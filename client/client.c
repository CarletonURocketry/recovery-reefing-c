// CLIENT (Secondary Pico) - Receives UDP packets
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico_lfs.h"
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

#define USB_WAIT_MS           3000
#define WIFI_ASSOC_TIMEOUT_MS 30000

#define HELLO_RETRY_INTERVAL_MS 1000
#define HELLO_MAX_RETRIES       40

#define PACKET_TIMEOUT_MS 5000
#define FS_SIZE (256 * 1024)

volatile uint32_t last_packet_time_ms = 0;
static struct lfs_config * config;
static lfs_t lfs;
//struct mallinfo m = mallinfo();

typedef enum {
    STATE_NOTHING_DEPLOYED = 0,
    STATE_MAIN_DEPLOYED    = 1,
    STATE_CUT_REEFING      = 2
} rocket_state_t;

rocket_state_t current_state = STATE_NOTHING_DEPLOYED;
bool server_acked = false;

struct udp_pcb *udp_client = NULL;

// Write to CSV file we are using for documentation
void write_to_CSV(char str[], lfs_t *lfs, lfs_file_t *file){
    printf("written: %s\n", str);
    lfs_file_open(lfs, file, "client.csv", LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND); 
    lfs_file_write(lfs, file, str, strlen(str)); // Write to system
    lfs_file_close(lfs, file);
    return;
} 

// Read from CSV file we are using for documentation
void read_CSV(lfs_t *lfs, lfs_file_t *file){
    char read_text[5000] ="";
    lfs_file_open(lfs, file, "client.csv", LFS_O_RDONLY | LFS_O_CREAT); 
    lfs_file_read(lfs, file, &read_text, sizeof(read_text)-1); 
    lfs_file_close(lfs, file);

    printf("%s\n", read_text);
    return;
}

// Reset CSV file to ensure that every test has a clean slate
void reset_csv(lfs_t *lfs, lfs_file_t *file){
    lfs_file_open(lfs, file, "client.csv", LFS_O_TRUNC); 
    lfs_file_close(lfs, file); 
    return;  
}

void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                       const ip_addr_t *addr, u16_t port) {
    if (p == NULL) return;
    last_packet_time_ms = to_ms_since_boot(get_absolute_time());

    char buffer[128];
    u16_t copy_len = p->len < sizeof(buffer) - 1 ? p->len : sizeof(buffer) - 1;
    memcpy(buffer, p->payload, copy_len);
    buffer[copy_len] = '\0';
    pbuf_free(p);

    printf("Received: %s\n", buffer);

    if (strncmp(buffer, "ACK", 3) == 0) {
        if (!server_acked) {
            server_acked = true;
            printf("Server acknowledged -- link established!\n");
        }
        return;
    }

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

static bool wifi_init_and_connect() {
    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        return false;
    }

    cyw43_arch_enable_sta_mode();
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
            return false;
        }
        sleep_ms(100);
    }

    if (link_status != CYW43_LINK_JOIN && link_status != CYW43_LINK_UP) {
        printf("WiFi association timed out\n");
        return false;
    }

    printf("Associated with AP!\n");

    struct netif *sta_if = &cyw43_state.netif[CYW43_ITF_STA];
    dhcp_stop(sta_if);
    ip4_addr_t ip, nm, gw;
    ip4addr_aton(CLIENT_IP, &ip);
    ip4addr_aton(CLIENT_NM, &nm);
    ip4addr_aton(CLIENT_GW, &gw);
    netif_set_addr(sta_if, &ip, &nm, &gw);
    netif_set_up(sta_if);
    printf("Static IP set: %s\n", ip4addr_ntoa(netif_ip4_addr(sta_if)));

    return true;
}

static bool setup_udp() {
    if (udp_client != NULL) {
        udp_remove(udp_client);
        udp_client = NULL;
    }

    udp_client = udp_new();
    if (udp_client == NULL) {
        printf("Failed to create UDP socket\n");
        return false;
    }

    err_t err = udp_bind(udp_client, IP_ADDR_ANY, UDP_PORT);
    if (err != ERR_OK) {
        printf("UDP bind failed: %d\n", err);
        udp_remove(udp_client);
        udp_client = NULL;
        return false;
    }

    udp_recv(udp_client, udp_recv_callback, NULL);
    printf("UDP socket ready on port %d\n", UDP_PORT);
    return true;
}

static void do_hello_handshake() {
    server_acked = false;
    ip_addr_t server_addr;
    ip4addr_aton(SERVER_IP, &server_addr);
    const char *hello = "HELLO_FROM_CLIENT";
    uint32_t last_hello_time = 0;
    int attempts = 0;

    printf("Sending HELLO to server (will retry every %d ms)...\n", HELLO_RETRY_INTERVAL_MS);

    while (!server_acked && attempts < HELLO_MAX_RETRIES) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_hello_time >= (uint32_t)HELLO_RETRY_INTERVAL_MS) {
            struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, strlen(hello), PBUF_RAM);
            if (p != NULL) {
                memcpy(p->payload, hello, strlen(hello));
                udp_sendto(udp_client, p, &server_addr, UDP_PORT);
                pbuf_free(p);
                printf("HELLO attempt %d/%d\n", ++attempts, HELLO_MAX_RETRIES);
            }
            last_hello_time = now;
        }
        cyw43_arch_poll();
        sleep_ms(10);
    }

    if (!server_acked) {
        printf("Server did not respond after %d attempts -- continuing anyway\n", HELLO_MAX_RETRIES);
    }
}

int main() {
    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    } 
    
    //declaration of FS + initialize file
    lfs_t lfs;
    lfs_file_t file; 

    //config
    config = pico_lfs_init(PICO_FLASH_SIZE_BYTES - FS_SIZE, FS_SIZE);
    if(!config){
        printf("Out of memory\n");
    }

    //mount 
    int error = lfs_mount(&lfs, config); 
    if(error != LFS_ERR_OK){
        printf("error, did not work\n");
        lfs_format(&lfs, config);
        lfs_mount(&lfs, config);
    }

    printf("\n=== ROCKET UDP CLIENT ===\n");
    reset_csv(&lfs, &file);
    
    write_to_CSV("Test no. 1\n", &lfs, &file);
    write_to_CSV("Test no. 2\n", &lfs, &file);
    write_to_CSV("Test no. 3]+++\n", &lfs, &file);
    write_to_CSV("Test no. 4 asdjfkajdskfj;klajsdjfkl;asdjfkl;adjfklajlk;sdflkajksdjfajksdf;asdjf\n", &lfs, &file);

    if (!wifi_init_and_connect()) {
        printf("Initial WiFi connect failed\n");
        return -1;
    }

    if (!setup_udp()) {
        printf("Initial UDP setup failed\n");
        return -1;
    }

    do_hello_handshake();

    last_packet_time_ms = to_ms_since_boot(get_absolute_time());

    // Main loop
    while (true) {
        cyw43_arch_poll();
        uint32_t now = to_ms_since_boot(get_absolute_time());

        config = pico_lfs_init(PICO_FLASH_SIZE_BYTES - FS_SIZE, FS_SIZE);
        if(!config){
            printf("Out of memory\n");
    }

        if (now - last_packet_time_ms >= PACKET_TIMEOUT_MS) {
            printf("\n[WATCHDOG] No packet for %d ms -- full WiFi reset...\n", PACKET_TIMEOUT_MS);

            cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
            sleep_ms(500);
            cyw43_arch_deinit();
            sleep_ms(500);

            if (!wifi_init_and_connect()) {
                printf("Reconnect failed, will retry next watchdog tick\n");
                last_packet_time_ms = to_ms_since_boot(get_absolute_time());
                sleep_ms(10);
                continue;
            }

            if (!setup_udp()) {
                printf("UDP setup failed, will retry next watchdog tick\n");
                last_packet_time_ms = to_ms_since_boot(get_absolute_time());
                sleep_ms(10);
                continue;
            }

            do_hello_handshake();

            printf("Fully reconnected, resuming...\n");
            last_packet_time_ms = to_ms_since_boot(get_absolute_time());
        }

        sleep_ms(10);
    }
    cyw43_arch_deinit();
    lfs_unmount(&lfs);
    
    return 0;
}