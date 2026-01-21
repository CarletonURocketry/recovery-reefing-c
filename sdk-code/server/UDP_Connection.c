// SERVER (Main Pico) - Sends UDP packets
// Pico SDK generates some header files we need, so if you are seeing red squiggles in your IDE, its probably because of that
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/udp.h"

// WiFi credentials
#define WIFI_SSID "ROCKET_AP"
#define WIFI_PASSWORD "rocket123"
#define UDP_PORT 4242

// Packet types to send
typedef enum {
    STATE_NOTHING_DEPLOYED = 0,
    STATE_MAIN_DEPLOYED = 1,
    STATE_CUT_REEFING = 2
} rocket_state_t;

// Global UDP control block
// PCB = Protocol Control Block, LwIP structure for sockets basically
struct udp_pcb *udp_sender;
ip_addr_t client_ip;
bool client_connected = false;

// Function to send state update packet
void send_state_packet(rocket_state_t state) {
    if (!client_connected) {
        printf("No client connected yet\n");
        return;
    }
    
    // Create packet, snprintf prevents buffer overflow. Could switch to binary packets if really need to
    char packet[32];
    snprintf(packet, sizeof(packet), "STATE:%d", state);
    
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
        } else {
            printf("Send failed: %d\n", err);
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
void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    if (p != NULL) {
        char buffer[128];
        memcpy(buffer, p->payload, p->len);
        buffer[p->len] = '\0';
        
        printf("Received from client: %s\n", buffer);
        
        // If the client IP is not saved yet, save it
        if (!client_connected) {
            ip_addr_copy(client_ip, *addr);
            client_connected = true;
            printf("Client registered: %s\n", ipaddr_ntoa(addr));
        }
        
        pbuf_free(p);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(3000); // Wait for USB connection to establish
    
    printf("\n--UDP SERVER--\n");
    
    // Initialize WiFi
    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        return -1;
    }
    
    printf("Starting Access Point...\n");
    
    // Start AP mode with our credentials
    cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
    
    printf("AP Started: %s\n", WIFI_SSID);
    printf("IP: 192.168.4.1\n");
    
    // Create UDP socket
    udp_sender = udp_new();
    if (udp_sender == NULL) {
        printf("Failed to create UDP socket\n");
        return -1;
    }
    
    // Bind to receive on any local IP and port 4242
    err_t err = udp_bind(udp_sender, IP_ADDR_ANY, UDP_PORT);
    if (err != ERR_OK) {
        printf("UDP bind failed: %d\n", err);
        return -1;
    }
    
    // Set receive callback (aka, registers our udp_recv_callback when packet is received)
    // When a packet arrives on udp_sender, call the function udp_recv_callback (NULL means no user arg)
    udp_recv(udp_sender, udp_recv_callback, NULL);
    
    printf("UDP server ready on port %d\n", UDP_PORT);
    printf("Waiting for client...\n\n");
    
    // Main loop - send different states
    rocket_state_t current_state = STATE_NOTHING_DEPLOYED;
    uint32_t last_send_time = 0;
    uint32_t state_change_time = 0;
    
    while (true) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        
        // Send 2 packets every second
        if (now - last_send_time >= 500) {
            send_state_packet(current_state);
            last_send_time = now;
        }
        
        // Change state every 5 seconds for testing
        if (now - state_change_time >= 5000) {
            current_state = (current_state + 1) % 3;
            printf("\n>>> STATE CHANGED TO: %d <<<\n\n", current_state);
            state_change_time = now;
        }
        
        // cyw43_arch_poll needs to be called regularly to handle WiFi events
        cyw43_arch_poll();
        // Just so we don't use 100% CPU (for now?)
        sleep_ms(100);
    }
    cyw43_arch_deinit(); //cleans up 
    return 0;
}
