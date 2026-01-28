// CLIENT (Secondary Pico) - Receives UDP packets
// Check client file to see red squiggly issue

// A lot of the code is similar to the server, so comments are sparse here. Refer to server.c for detailed comments.
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/udp.h"

// WiFi credentials
#define WIFI_SSID "ROCKET_AP"
#define WIFI_PASSWORD "rocket123"
#define SERVER_IP "192.168.4.1"
#define UDP_PORT 4242

// Packet types
typedef enum {
    STATE_NOTHING_DEPLOYED = 0,
    STATE_MAIN_DEPLOYED = 1,
    STATE_CUT_REEFING = 2
} rocket_state_t;

rocket_state_t current_state = STATE_NOTHING_DEPLOYED;

// UDP receive callback
void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    if (p != NULL) {
        char buffer[128];
        memcpy(buffer, p->payload, p->len);
        buffer[p->len] = '\0';
        
        printf("Received: %s\n", buffer);
        
        // Parse state, sscanf reads from string buffer, parses state and puts value into state variable
        // Returns number of successfully parsed items
        int state;
        if (sscanf(buffer, "STATE:%d", &state) == 1) {
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
                    // Blink rapidly
                    for (int i = 0; i < 10; i++) {
                        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                        sleep_ms(50);
                        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                        sleep_ms(50);
                    }
                    break;
            }
        }
        
        pbuf_free(p);
    }
}

int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
        
    printf("\n=== ROCKET UDP CLIENT ===\n");
    
    // Initialize WiFi
    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        return -1;
    }
    
    // Enable station mode, will allow to connect to AP
    cyw43_arch_enable_sta_mode();
    
    printf("Connecting to: %s\n", WIFI_SSID);
    
    // Connects to AP, 30s timeout because establishing connection might take a little bit, 
    // We will find out more during testing
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("WiFi connection failed test\n");
        return -1;
    }
    
    printf("Connected!\n");
    // Prints assigned IP
    printf("Client IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    
    // Create UDP socket
    struct udp_pcb *udp_client = udp_new();
    if (udp_client == NULL) {
        printf("Failed to create UDP socket\n");
        return -1;
    }
    
    // Bind to port
    err_t err = udp_bind(udp_client, IP_ADDR_ANY, UDP_PORT);
    if (err != ERR_OK) {
        printf("UDP bind failed: %d\n", err);
        return -1;
    }
    
    // Set receive callback
    udp_recv(udp_client, udp_recv_callback, NULL);
    
    printf("UDP client ready on port %d\n", UDP_PORT);
    printf("Listening for packets...\n\n");
    
    // Send hello message to server
    ip_addr_t server_addr;
    // Convert SERVER_IP string to ip_addr_t
    ip4addr_aton(SERVER_IP, &server_addr);
    
    const char *hello = "HELLO_FROM_CLIENT";
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, strlen(hello), PBUF_RAM);
    if (p != NULL) {
        memcpy(p->payload, hello, strlen(hello));
        udp_sendto(udp_client, p, &server_addr, UDP_PORT);
        pbuf_free(p);
        printf("Sent hello to server\n");
    }
    
    // Main loop
    while (true) {
        cyw43_arch_poll();
        sleep_ms(100);
    }
    
    return 0;
}