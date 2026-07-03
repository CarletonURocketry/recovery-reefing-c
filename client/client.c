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
#include "pwm-tone.h"
#include "hardware/gpio.h"

//connection stuff
#define WIFI_SSID     "ROCKET_AP"
#define WIFI_PASSWORD "rocket123"
#define SERVER_IP     "192.168.4.1"
#define UDP_PORT      4242

//Client IP setup
#define CLIENT_IP     "192.168.4.2"
#define CLIENT_NM     "255.255.255.0"
#define CLIENT_GW     "192.168.4.1"

#define USB_WAIT_MS           3000
#define WIFI_ASSOC_TIMEOUT_MS 30000

#define HELLO_RETRY_INTERVAL_MS 1000
#define HELLO_MAX_RETRIES       8 
//amount of hellos before retry - min 7

//LEDs
#define LED1 11
#define LED2 12
#define LED3 13

//buzzer pin TBD
#define PIEZO_PIN   27

#define PACKET_TIMEOUT_MS 5000
#define FS_SIZE (256 * 1024)

#define EMATCHPIN 10

note_t VICTORY[] = {
    {NOTE_G4, 8},
    {NOTE_G4, 16},
    {NOTE_G4, 16},
    {NOTE_D5, 4},
    {REST, 8},
    {MELODY_END, 0},
};

note_t CONFIRM[] = {
    {NOTE_C7, 128},
    {REST, 128},
    {NOTE_C7, 128},
    {REST, 128},
    {NOTE_C7, 128},
    {REST, 128},
    {NOTE_C7, 128},
    {REST, 128},
    {REST, 8},
    {MELODY_END, 0},
};

volatile uint32_t last_packet_time_ms = 0;
static struct lfs_config * config;
static lfs_t lfs;
//struct mallinfo m = mallinfo();

//states
typedef enum {
    IDLE = 0,
    CONNECTED = 1,
    BLOW_UP = 2
} rocket_state_t;

rocket_state_t current_state;
bool server_acked = false;

struct udp_pcb *udp_client = NULL; 

tonegenerator_t generator;

void tone_gen(){
    tone_init(&generator, PIEZO_PIN);
    melody(&generator, CONFIRM, 0);
}

struct csv_struct {
    lfs_t *recv_lfs;
    lfs_file_t *recv_file;
};

void init_leds(){
    gpio_init(11);
    gpio_set_dir(11, GPIO_OUT);
    gpio_init(12);
    gpio_set_dir(12, GPIO_OUT);
    gpio_init(13);
    gpio_set_dir(13, GPIO_OUT);
}


void init_ematch_pin(){
    gpio_init(EMATCHPIN);
    gpio_set_dir(EMATCHPIN, GPIO_OUT);
}

//important intitializations **should be broken up later
void init(){
    stdio_init_all();

    init_ematch_pin();
    init_leds();
}

// Exactly what the function says
void blow_up(){
    gpio_put(EMATCHPIN, 1);
}

// memory function for testing purposes
void print_memory_usage() {
    struct mallinfo info = mallinfo();

    printf("Total allocated: %d bytes\n", info.uordblks);
    printf("Total free: %d bytes\n", info.fordblks);
    printf("Total heap size: %d bytes\n", info.arena);
    printf("Largest free block: %d bytes\n", info.ordblks);
}

// Write to CSV file we are using for documentation
void write_to_CSV(char str[], lfs_t *lfs, lfs_file_t *file){
    printf("%s", str);
    lfs_file_open(lfs, file, "client.csv", LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND); 
    lfs_file_write(lfs, file, str, strlen(str)); // Write to system
    lfs_file_close(lfs, file);
    return;
} 

// Reset CSV file to ensure that every test has a clean slate
void reset_csv(lfs_t *lfs, lfs_file_t *file){
    lfs_file_open(lfs, file, "client.csv", LFS_O_TRUNC); 
    lfs_file_close(lfs, file); 
    return;  
}
//gets packet ready to be sent to server
void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                       const ip_addr_t *addr, u16_t port) {
    if (p == NULL) return;
    last_packet_time_ms = to_ms_since_boot(get_absolute_time());

    struct csv_struct *s = (struct csv_struct *)arg; // takes arg csv info and transfers it to s
    
    char buffer[128];
    char text[32];
    u16_t copy_len = p->len < sizeof(buffer) - 1 ? p->len : sizeof(buffer) - 1;
    memcpy(buffer, p->payload, copy_len);
    buffer[copy_len] = '\0';
    pbuf_free(p);

    // printf("Received: %s\n", buffer);
    snprintf(text, sizeof(text), "Received: %s\n", buffer);
    //write_to_CSV(text, s->recv_lfs, s->recv_file);
    //lfs_file_open(s->recv_lfs, s->recv_file, "client.csv", LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND); 
   // lfs_file_write(s->recv_lfs, s->recv_file, text, strlen(text)); // Write to system
   // lfs_file_close(s->recv_lfs, s->recv_file);
    if (strncmp(buffer, "ACK", 3) == 0) {
        if (!server_acked) {
            server_acked = true;
            printf("Server acknowledged -- link established!\n");
            //write_to_CSV("Server acknowledged -- link established!\n", s->recv_lfs, s->recv_file);
        }
        return;
    }

    int state;
    if (sscanf(buffer, "STATE:%d", &state) != 1) return;

    current_state = (rocket_state_t)state;
    //state machine
    switch (current_state) {
        case IDLE:
            // printf("  -> Nothing deployed\n");

            break;
        case CONNECTED:
            // printf("  -> Main parachute deployed!\n");
            
            break;
        case BLOW_UP:

            while(true){
                blow_up();
            }

            break;
    }
}
//checks if wifi card is working
static bool wifi_init_and_connect(lfs_t *lfs, lfs_file_t *file) {
    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        //write_to_CSV("WiFi init failed\n", lfs, file);
        return false;
    }
    //wifi card into correct mode and type
    cyw43_arch_enable_sta_mode();
    cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);

    absolute_time_t deadline = make_timeout_time_ms(WIFI_ASSOC_TIMEOUT_MS);
    int link_status = CYW43_LINK_DOWN;
    char text[128];
    
    //case for when wifi doesnt work
    while (!time_reached(deadline)) {
        cyw43_arch_poll();
        link_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);

        if (link_status == CYW43_LINK_JOIN || link_status == CYW43_LINK_UP) break;

        if (link_status < 0) {
            // printf("WiFi error: %d", link_status);
            snprintf(text, sizeof(text), "WiFi error: %d", link_status);
            //write_to_CSV(text, lfs, file);

            if      (link_status == CYW43_LINK_BADAUTH) write_to_CSV(" (bad password)\n", lfs, file); //printf(" (bad password)\n"); // write_to_CSV(" (bad password)\n", &lfs, &file);
            else if (link_status == CYW43_LINK_NONET)   write_to_CSV(" (AP not found)\n", lfs, file); //printf(" (AP not found)\n"); // write_to_CSV(" (AP not found)\n", &lfs, &file);
            else write_to_CSV("\n", lfs, file); // printf("\n");  
            return false;
        }
        sleep_ms(100);
    }

    if (link_status != CYW43_LINK_JOIN && link_status != CYW43_LINK_UP) {
        printf("WiFi association timed out\n");
        //write_to_CSV("WiFi association timed out\n", lfs, file);
        return false;
    }

    printf("Associated with AP!\n");
    //write_to_CSV("Associated with AP!\n", lfs, file);
    //network setups
    struct netif *sta_if = &cyw43_state.netif[CYW43_ITF_STA];
    dhcp_stop(sta_if);
    ip4_addr_t ip, nm, gw;
    ip4addr_aton(CLIENT_IP, &ip);
    ip4addr_aton(CLIENT_NM, &nm);
    ip4addr_aton(CLIENT_GW, &gw);
    netif_set_addr(sta_if, &ip, &nm, &gw);
    netif_set_up(sta_if);
    printf("Static IP set: %s\n", ip4addr_ntoa(netif_ip4_addr(sta_if)));
    // char text[32];
    snprintf(text, sizeof(text), "Static IP set: %s\n", ip4addr_ntoa(netif_ip4_addr(sta_if)));
    //write_to_CSV(text, lfs, file);

    return true;
}

static bool setup_udp(lfs_t *lfs, lfs_file_t *file) {
    char text[32];
    if (udp_client != NULL) {
        udp_remove(udp_client);
        udp_client = NULL;
    }

    udp_client = udp_new();
    if (udp_client == NULL) {
        printf("Failed to create UDP socket\n");
        // write_to_CSV("Failed to create UDP socket\n", lfs, file);
        return false;
    }

    err_t err = udp_bind(udp_client, IP_ADDR_ANY, UDP_PORT);
    if (err != ERR_OK) {
        printf("UDP bind failed: %d\n", err);
        //snprintf(text, sizeof(text),"UDP bind failed: %d\n", err);
        //write_to_CSV(text, lfs, file);
        udp_remove(udp_client);
        udp_client = NULL;
        return false;
    } 

    static struct csv_struct csv;
    csv.recv_lfs = lfs;
    csv.recv_file = file;

    udp_recv(udp_client, udp_recv_callback, &csv);
    printf("UDP socket ready on port %d\n", UDP_PORT);
    //snprintf(text, sizeof(text), "UDP socket ready on port %d\n", UDP_PORT);
    //write_to_CSV(text, lfs, file);
    return true;
}
//recconection/ initial connection
static void do_hello_handshake(lfs_t *lfs, lfs_file_t *file) {
    char text[64];
    server_acked = false;
    ip_addr_t server_addr;
    ip4addr_aton(SERVER_IP, &server_addr);
    const char *hello = "HELLO_FROM_CLIENT";
    uint32_t last_hello_time = 0;
    int attempts = 0;

    printf("Sending HELLO to server (will retry every %d ms)...\n", HELLO_RETRY_INTERVAL_MS);
    //snprintf(text, sizeof(text), "Sending HELLO to server (will retry every %d ms)...\n", HELLO_RETRY_INTERVAL_MS);
    //write_to_CSV(text, lfs, file);
    sleep_ms(10);
    //hello attempts
    while (!server_acked && attempts < HELLO_MAX_RETRIES) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_hello_time >= (uint32_t)HELLO_RETRY_INTERVAL_MS) {
            struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, strlen(hello), PBUF_RAM);
            if (p != NULL) {
                memcpy(p->payload, hello, strlen(hello));
                udp_sendto(udp_client, p, &server_addr, UDP_PORT);
                pbuf_free(p);
                printf("HELLO attempt %d/%d\n", ++attempts, HELLO_MAX_RETRIES);
                //snprintf(text, sizeof(text), "HELLO attempt %d/%d\n", ++attempts, HELLO_MAX_RETRIES);
                //write_to_CSV(text, lfs, file);
            }
            last_hello_time = now;
        }
        cyw43_arch_poll();
        sleep_ms(10);
    }

    if (!server_acked) {
        printf("Server did not respond after %d attempts -- continuing anyway\n", HELLO_MAX_RETRIES);
        snprintf(text, sizeof(text),"Server did not respond after %d attempts -- continuing anyway\n", HELLO_MAX_RETRIES);
        write_to_CSV(text, lfs, file);        
    }
}

int main() {
    //initialize pico
    stdio_init_all();
    
    //declaration of FS + initialize file
    lfs_t lfs;
    lfs_file_t file; 
    char text[32];

    //config
    config = pico_lfs_init(PICO_FLASH_SIZE_BYTES - FS_SIZE, FS_SIZE);
    if(!config){
        printf("Out of memory\n");
    }

    //mount 
    //for every new pico 3 lines need to be commented out initially, loaded onto the pico, then add the lines back in
    int error = lfs_mount(&lfs, config); //this line
    if(error != LFS_ERR_OK){ //this line
        printf("error, did not work\n");
        lfs_format(&lfs, config);
        lfs_mount(&lfs, config);
    }//this line 
    
    reset_csv(&lfs, &file); // Reset CSV file to prepare for next test -> IF YOU ARE INITIALIZING A NEW CSV YOU NEED TO COMMENT THIS OUT THE FIRST TIME
    //printf("\n=== ROCKET UDP CLIENT ===\n");
    write_to_CSV("=== ROCKET UDP CLIENT ===\n", &lfs, &file);

    init(); 

    tone_gen();

    if (!wifi_init_and_connect(&lfs, &file)) {
        //printf("Initial WiFi connect failed\n");
        write_to_CSV("Initial WiFi connect failed\n", &lfs, &file);
        return -1;
    }

    if (!setup_udp(&lfs, &file)) {
        //printf("Initial UDP setup failed\n");
        write_to_CSV("Initial UDP setup failed\n", &lfs, &file);
        return -1;
    }
    //connecting
    do_hello_handshake(&lfs, &file);

    last_packet_time_ms = to_ms_since_boot(get_absolute_time());
     //just added for testing 

    // Main loop
    while (true) {
        cyw43_arch_poll(); // tends to wifi card NEVER REMOVE
        uint32_t now = to_ms_since_boot(get_absolute_time());


        //when blow up is initialized this state should just be continuously sent 
        if (current_state == BLOW_UP){
            while (true){
                blow_up();
            }
        }
        //if a certian amount of time has passed between packets received system assumes disconnection 
        //resets everything and do_hello_handshake is redone
        if (now - last_packet_time_ms >= PACKET_TIMEOUT_MS) {
            printf("\n[WATCHDOG] No packet for %d ms -- full WiFi reset...\n", PACKET_TIMEOUT_MS);
            //snprintf(text, sizeof(text), "\n[WATCHDOG] No packet for %d ms -- full WiFi reset...\n", PACKET_TIMEOUT_MS);
            //write_to_CSV(text, &lfs, &file);

            cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
            sleep_ms(500);
            cyw43_arch_deinit();
            sleep_ms(500);

            if (!wifi_init_and_connect(&lfs, &file)) {
                printf("Reconnect failed, will retry next watchdog tick\n");
                //write_to_CSV("Reconnect failed, will retry next watchdog tick\n", &lfs, &file);
                last_packet_time_ms = to_ms_since_boot(get_absolute_time());
                sleep_ms(10);
                continue;
            }

            if (!setup_udp(&lfs, &file)) {
                printf("UDP setup failed, will retry next watchdog tick\n");
                //write_to_CSV("UDP setup failed, will retry next watchdog tick\n", &lfs, &file);
                last_packet_time_ms = to_ms_since_boot(get_absolute_time());
                sleep_ms(10);
                continue;
            }

            do_hello_handshake(&lfs, &file);

            printf("Fully reconnected, resuming...\n");
            //write_to_CSV("Fully reconnected, resuming...\n", &lfs, &file);
            last_packet_time_ms = to_ms_since_boot(get_absolute_time());
        }

        sleep_ms(10);
    }
    // cyw43_arch_deinit();
    lfs_unmount(&lfs);
    
    return 0;
}
