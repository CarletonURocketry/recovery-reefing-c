// CLIENT (Secondary Pico) - Receives UDP packets
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/udp.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/dhcp.h"

#define WIFI_SSID     "ROCKET_AP"
#define WIFI_PASSWORD "rocket123"
#define SERVER_IP     "192.168.4.1"
#define UDP_PORT      4242

#define CLIENT_IP     "192.168.4.2"
#define CLIENT_NM     "255.255.255.0"
#define CLIENT_GW     "192.168.4.1"

// Wait up to 3s for USB serial — then boot anyway (no USB in flight)
#define USB_WAIT_MS          3000
#define WIFI_ASSOC_TIMEOUT_MS 30000

// How often to retry HELLO, and how many times before giving up
#define HELLO_RETRY_INTERVAL_MS 1000
#define HELLO_MAX_RETRIES       30

typedef enum {
    STATE_NOTHING_DEPLOYED = 0,
    STATE_MAIN_DEPLOYED    = 1,
    STATE_CUT_REEFING      = 2
} rocket_state_t;

rocket_state_t current_state = STATE_NOTHING_DEPLOYED;
bool server_acked = false;  // Set true when server replies with ACK

// UDP receive callback
void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                       const ip_addr_t *addr, u16_t port) {
    if (p == NULL) return;

    char buffer[128];
    u16_t copy_len = p->len < sizeof(buffer) - 1 ? p->len : sizeof(buffer) - 1;
    memcpy(buffer, p->payload, copy_len);
    buffer[copy_len] = '\0';
    pbuf_free(p);

    printf("Received: %s\n", buffer);

    // Server ACK to our HELLO -- handshake complete
    if (strncmp(buffer, "ACK", 3) == 0) {
        if (!server_acked) {
            server_acked = true;
            printf("Server acknowledged -- link established!\n");
        }
        return;
    }

    // State packet from server
    int state;
    if (sscanf(buffer, "STATE:%d", &state) != 1) return;

    current_state = (rocket_state_t)state;

    switch (current_state) {
        case STATE_NOTHING_DEPLOYED:
            printf("  -> Nothing deployed\n");
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            break;

        case STATE_MAIN_DEPLOYED:
            printf("  -> Main parachute deployed!\n");
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            break;

        case STATE_CUT_REEFING:
            printf("  -> CUT REEFING LINE!!!\n");
            for (int i = 0; i < 10; i++) {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                sleep_ms(50);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                sleep_ms(50);
            }
            break;
    }
}


int main() {
    stdio_init_all();

    // Waits for USB serial connection before proceeding, will need to change before in flight
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
        
    printf("\n=== ROCKET UDP CLIENT ===\n");

    // Initialize WiFi (must be before any lwIP calls)
    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        return -1;
    }

    // Enable STA mode and start connecting to AP
    cyw43_arch_enable_sta_mode();
    printf("Connecting to AP: %s\n", WIFI_SSID);

    // Connect async and wait for layer-2 association only (CYW43_LINK_JOIN).
    // cyw43_arch_wifi_connect_timeout_ms waits for LINK_UP which requires
    // DHCP. The AP has no DHCP server so that would always time out.
    // Async connection, does not block -- we will poll for completion below. This allows the program to remain responsive and handle timeouts properly.
    cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);

    absolute_time_t deadline = make_timeout_time_ms(WIFI_ASSOC_TIMEOUT_MS);
    int link_status = CYW43_LINK_DOWN;

    while (!time_reached(deadline)) {
        cyw43_arch_poll();
        link_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);

        if (link_status == CYW43_LINK_JOIN || link_status == CYW43_LINK_UP) break;

        if (link_status < 0) {
            printf("WiFi error: %d", link_status);
            if      (link_status == CYW43_LINK_BADAUTH) printf(" (bad password)\n");
            else if (link_status == CYW43_LINK_NONET)   printf(" (AP not found)\n");
            else printf("\n");
            return -1;
        }
        sleep_ms(100);
    }

    if (link_status != CYW43_LINK_JOIN && link_status != CYW43_LINK_UP) {
        printf("WiFi association timed out\n");
        return -1;
    }

    printf("Associated with AP!\n");

    // Set static IP directly on the STA netif -- no netif_list search.
    // dhcp_stop() prevents the DHCP client from later overwriting our IP.
    struct netif *sta_if = &cyw43_state.netif[CYW43_ITF_STA];

    // Could also change lwipopts.h to disable DHCP client entirely (LWIP_DHCP=0) since we won't be using it for anything
    dhcp_stop(sta_if);

    // Set static IP, netmask, gateway
    ip4_addr_t ip, nm, gw;
    ip4addr_aton(CLIENT_IP, &ip);
    ip4addr_aton(CLIENT_NM, &nm);
    ip4addr_aton(CLIENT_GW, &gw);
    netif_set_addr(sta_if, &ip, &nm, &gw);

    printf("Static IP set: %s\n", ip4addr_ntoa(&ip));

    // Create and bind UDP socket
    struct udp_pcb *udp_client = udp_new();
    if (udp_client == NULL) {
        printf("Failed to create UDP socket\n");
        return -1;
    }

    err_t err = udp_bind(udp_client, IP_ADDR_ANY, UDP_PORT);
    if (err != ERR_OK) {
        printf("UDP bind failed: %d\n", err);
        return -1;
    }

    udp_recv(udp_client, udp_recv_callback, NULL);

    printf("UDP client ready on port %d\n", UDP_PORT);
    printf("Listening for packets...\n\n");

    // Send HELLO repeatedly until server ACKs.
    ip_addr_t server_addr;
    ip4addr_aton(SERVER_IP, &server_addr);

    const char *hello = "HELLO_FROM_CLIENT";
    uint32_t last_hello_time = 0;
    int hello_attempts = 0;

    // Interval can be a lot quicker, current one is for testing
    printf("Sending HELLO to server (will retry every %d ms)...\n", HELLO_RETRY_INTERVAL_MS);

    while (!server_acked && hello_attempts < HELLO_MAX_RETRIES) {
        uint32_t now = to_ms_since_boot(get_absolute_time());

        if (now - last_hello_time >= (uint32_t)HELLO_RETRY_INTERVAL_MS) {
            struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, strlen(hello), PBUF_RAM);
            if (p != NULL) {
                memcpy(p->payload, hello, strlen(hello));
                udp_sendto(udp_client, p, &server_addr, UDP_PORT);
                pbuf_free(p);
                printf("HELLO attempt %d/%d\n", ++hello_attempts, HELLO_MAX_RETRIES);
            }
            last_hello_time = now;
        }

        cyw43_arch_poll();
        sleep_ms(10);
    }

    if (!server_acked) {
        printf("Server did not respond after %d attempts\n", HELLO_MAX_RETRIES);
        // Continue anyway -- server may still be sending state packets
    }

    // Main loop -- lwIP callbacks fire from cyw43_arch_poll()
    while (true) {
        cyw43_arch_poll();
        sleep_ms(10);
    }

    return 0;
}