// SERVER (Main Pico) - Sends UDP packets
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico_lfs.h"
#include "lwip/udp.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/ip4_addr.h"
#include "pwm-tone.h"
#include "hardware/gpio.h"

//wifi infomration
#define WIFI_SSID     "ROCKET_AP"
#define WIFI_PASSWORD "rocket123"
#define UDP_PORT      4242

//static IP setup information
#define AP_IP   "192.168.4.1"
#define AP_NM   "255.255.255.0"
#define AP_GW   "192.168.4.1"

//LEDs
#define LED1 11
#define LED2 12
#define LED3 13

//buzzer pin TBD
#define PIEZO_PIN   27

#define ALTIMETERPIN 28

// Packet types to send
typedef enum {
    IDLE = 0,
    CONNECTED = 1,
    BLOW_UP = 2
} rocket_state_t;

#define FS_SIZE (256 * 1024)
#define FS_OFFSET (1024 * 1024)

// Global UDP control block (PCB = Protocol Control Block, lwIP's internal socket structure)
struct udp_pcb *udp_sender;
ip_addr_t client_ip;
bool client_connected = false;

note_t VICTORY[] = {
    {NOTE_G4, 8},
    {NOTE_G4, 16},
    {NOTE_G4, 16},
    {NOTE_D5, 4},
    {REST, 8},
    {MELODY_END, 0},
};

note_t COIN[] = {
    {NOTE_C6, 16},
    {NOTE_C7, 4},
    {REST, 8},
    {MELODY_END, 0},
};

// extern struct lfs_config pico_cfg;
static lfs_t lfs;
struct lfs_config *config;
tonegenerator_t generator;

rocket_state_t current_state;

void tone_gen(){
    tone_init(&generator, PIEZO_PIN);
    melody(&generator, VICTORY, 0);
}

void tone_gen_connected(){
    tone_init(&generator, PIEZO_PIN);
    melody(&generator, COIN, 0);
}

struct csv_struct {
    lfs_t *recv_lfs;
    lfs_file_t *recv_file;
};



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

void init_leds(){
    gpio_init(11);
    gpio_set_dir(11, GPIO_OUT);
    gpio_init(12);
    gpio_set_dir(12, GPIO_OUT);
    gpio_init(13);
    gpio_set_dir(13, GPIO_OUT);
}

void init_altimeter_pin(){
    gpio_init(ALTIMETERPIN);
    gpio_set_dir(ALTIMETERPIN, GPIO_IN);

    // Altimeter is pull up; 1 when IN == 0, 0 when IN == 1
    gpio_pull_up(ALTIMETERPIN);
}

void init(){
    stdio_init_all();

    init_altimeter_pin();
    init_leds();
}

// Send a state packet to the registered client
void send_state_packet(rocket_state_t state, lfs_t *lfs, lfs_file_t *file) {
    char text[32]; // I'm making this to make sure my stuff stays out of your way, but theres a good chance if we just put packet here we could use that instead
    if (!client_connected) {
        printf("No client connected yet\n");
        // write_to_CSV("No client connected yet\n", lfs, file);
        // print_memory_usage();
        return;
    }

    //creat packet snprintf prevents buffer overflow. Could switch to binary packets if really need to
    char packet[32];
    snprintf(packet, sizeof(packet), "STATE:%d", state);
    write_to_CSV(packet, lfs, file);

    // Send via UDP
    // LwIP uses pbufs for packet data, stands for packet buffers. Think of them as an envelope for our data
    // The parameters are: Transport layer packet, bytes we need, and where to allocate from
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, strlen(packet), PBUF_RAM);
    if (p != NULL) {
        memcpy(p->payload, packet, strlen(packet));
        // Sends the packet p to from UDP socket udp_sender, to the client IP and port
        err_t err = udp_sendto(udp_sender, p, &client_ip, UDP_PORT);
        if (err == ERR_OK) {
            printf("Sent: %s\n", packet);
            // snprintf(text, sizeof(text),"Sent: %s\n", packet);
            // write_to_CSV(text, lfs, file);
        } else {
            printf("Send failed: %d\n", err);
            // snprintf(text, sizeof(text), "Send failed: %s\n", err);
            // write_to_CSV(text, lfs, file);
        }
        pbuf_free(p);
    }
}

// The server gets a callback when a packet is received, this is how it knows the IP of the client and where to send packets
// arg: user argument (not used here)
// pcb: the UDP socket that received the packet
// p: the received packet buffer
// addr: the IP address of the sender
// port: the source port of the sender
void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                       const ip_addr_t *addr, u16_t port) {
    if (p == NULL) return;
    
    struct csv_struct *s = (struct csv_struct *)arg; // takes arg csv info and transfers it to s
    lfs_t *lfs = s->recv_lfs;
    lfs_file_t *file = s->recv_file;

    char buffer[128];
    char text[32];
    u16_t copy_len = p->len < sizeof(buffer) - 1 ? p->len : sizeof(buffer) - 1;
    memcpy(buffer, p->payload, copy_len);
    buffer[copy_len] = '\0';
    pbuf_free(p);

    printf("Received from client: %s\n", buffer);
    // snprintf(text, sizeof(text), "Received from client: %s\n", buffer);
    // write_to_CSV(text, lfs, file);

     // If the client IP is not saved yet, save it
    if (!client_connected) {
        ip_addr_copy(client_ip, *addr);
        client_connected = true;
        printf("Client registered at: %s\n", ipaddr_ntoa(&client_ip));
        // snprintf(text, sizeof(text), "Client registered at: %s\n", ipaddr_ntoa(&client_ip));
        // write_to_CSV(text, lfs, file);
    }

    // ACK so the client knows the server is alive and stops retrying HELLO
    const char *ack = "ACK";
    struct pbuf *ack_p = pbuf_alloc(PBUF_TRANSPORT, strlen(ack), PBUF_RAM);
    if (ack_p != NULL) {
        memcpy(ack_p->payload, ack, strlen(ack));
        udp_sendto(udp_sender, ack_p, addr, port);
        pbuf_free(ack_p);
    }
}
//main
int main() {
    stdio_init_all();

    bool buzzer_played = false;
    
    sleep_ms(3000); // Wait for USB serial to establish

    printf("\n--Initializing CSV File--\n");
    //declaration of FS + initialize file
    lfs_t lfs;
    lfs_file_t file;
    char text[64];
    //printf("read  = %p\n", config.read);
    //printf("prog  = %p\n", config.prog);
    //printf("erase = %p\n", config.erase);
    //printf("sync  = %p\n", config.sync);

    //mount 
    config = pico_lfs_init(FS_OFFSET, FS_SIZE);
    if(!config){
        printf("pico_lfs_init failed");
    }

    int error = lfs_mount(&lfs, config); 
    if(error != LFS_ERR_OK){
        printf("err did not work\n");
        lfs_format(&lfs, config);
        lfs_mount(&lfs, config);
    }

    // tone_init(&generator, PIEZO_PIN);

    reset_csv(&lfs, &file); 
    //printf("\n--UDP SERVER--\n"); 
    write_to_CSV("===ROCKET UDP SERVER===\n", &lfs, &file);

    init();

    tone_gen();

    // Initialize WiFi
    if (cyw43_arch_init()) {
        //printf("WiFi init failed\n");
        write_to_CSV("WiFi init failed\n", &lfs, &file);
        return -1;
    }

    //printf("Starting Access Point...\n");
    write_to_CSV("Starting Access Point...\n", &lfs, &file);
    cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);

    // Give the AP interface time to initialise before configuring it
    sleep_ms(500);
    tone_gen();

    // Configure the AP interface IP directly from cyw43_state.
    // The loopback interface (127.0.0.1) is ALWAYS the first entry in
    // netif_list and ALWAYS has netif_is_up() == true. Any search that
    // just grabs the first "up" interface silently configures loopback with
    // 192.168.4.1 and leaves the real AP netif at 0.0.0.0. Incoming UDP
    // packets destined for 192.168.4.1 are then dropped because no netif
    // owns that address. This was the root cause of nothing being received.
    struct netif *ap_if = &cyw43_state.netif[CYW43_ITF_AP];

    //sets static ip netmask and gateway
    ip4_addr_t ipaddr, nm, gw;
    ip4addr_aton(AP_IP, &ipaddr);
    ip4addr_aton(AP_NM, &nm);
    ip4addr_aton(AP_GW, &gw);
    netif_set_addr(ap_if, &ipaddr, &nm, &gw);

    // printf("AP IP configured: %s\n", ip4addr_ntoa(&ipaddr));
    write_to_CSV(("AP IP configured: %s\n", ip4addr_ntoa(&ipaddr)), &lfs, &file);
    // printf("AP started: %s\n", WIFI_SSID);
    write_to_CSV(("AP started: %s\n", WIFI_SSID), &lfs, &file);

    // Create UDP socket and bind AFTER the AP interface has its IP set
    udp_sender = udp_new();
    if (udp_sender == NULL) {
        // printf("Failed to create UDP socket\n");
        write_to_CSV("Failed to create UDP socket\n", &lfs, &file);
        return -1;
    }

    err_t err = udp_bind(udp_sender, IP_ADDR_ANY, UDP_PORT);
    if (err != ERR_OK) {
        // printf("UDP bind failed: %d\n", err);
        sprintf(text, "UDP bind failed: %d\n", err);
        write_to_CSV(text, &lfs, &file);
        return -1;
    }

    static struct csv_struct csv;
    csv.recv_lfs = &lfs;
    csv.recv_file = &file;

    // Set receive callback (aka, registers our udp_recv_callback when packet is received)
    // When a packet arrives on udp_sender, call the function udp_recv_callback (NULL means no user arg)
    udp_recv(udp_sender, udp_recv_callback, &csv);

    // printf("UDP server ready on port %d\n", UDP_PORT);
    sprintf(text, "UDP server ready on port %d\n", UDP_PORT);
    write_to_CSV(text, &lfs, &file);
    // printf("Waiting for client hello...\n\n");
    write_to_CSV("Waiting for client hello...\n\n", &lfs, &file);

    // Main loop - send different states
    
    uint32_t last_send_time    = 0;
    uint32_t state_change_time = 0;
    
    while (true) {
        uint32_t now = to_ms_since_boot(get_absolute_time());

        if(client_connected){
            if(!buzzer_played){
                tone_gen_connected();
                buzzer_played = true;
                current_state = CONNECTED;
                sprintf(text, "\n>>> STATE CHANGED TO: %d <<<\n\n", current_state);
                write_to_CSV(text, &lfs, &file);
                state_change_time = now;
            }
        }
        else if(!client_connected){
            current_state == IDLE;
        }

        if (!gpio_get(ALTIMETERPIN)){
            
            sleep_ms(70);
            
            if (gpio_get(ALTIMETERPIN)){
                current_state = BLOW_UP;
                sprintf(text, "\n>>> STATE CHANGED TO: %d <<<\n\n", current_state);
                write_to_CSV(text, &lfs, &file);
                state_change_time = now;
                }
            }
        

        // Send 2 packets per second once client is registered
        if (now - last_send_time >= 100) {
            send_state_packet(current_state, &lfs, &file);
            last_send_time = now;

        }
         if(current_state == CONNECTED){
            gpio_put(11, 1);
        }else{
            gpio_put(11, 0);
        }
            //current_state = (rocket_state_t)((current_state + 1) % 3);
            //printf("\n>>> STATE CHANGED TO: %d <<<\n\n", current_state);
            //sprintf(text, "\n>>> STATE CHANGED TO: %d <<<\n\n", current_state);
            // print_memory_usage();
            //write_to_CSV(text, &lfs, &file);
            //state_change_time = now;
        
        // Must be called regularly to pump WiFi/lwIP events
        cyw43_arch_poll();
        // Just so we don't use 100% CPU (for now?)
        sleep_ms(10);
    }

    cyw43_arch_deinit(); //cleanup
    return 0;
}
