---
updated: 2026-03-10 19:49:53Z
created: 2026-03-06 21:41:37Z
---

&nbsp;

* * *

# SETUP

```C
struct udp_pcb *udp_sender;
```

- pointer that will later be bound to the socket used for sending UDP signals 
- `udp_pcb` structure from lwip used to store the socket number is a way lwip can understand
- will be connected to `udp_new()` → creates a new UDP PCB

```C
ip_addr_t client_ip;
```

- `ip_addr_t` is a structure from lwip that stores IP addresses in a format lwip can work with

* * *

```C
void send_state_packet(rocket_state_t state)
```

## Param

- Takes in the state number the rocket is in

sends a packet of data to the client 

- ```C
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, strlen(packet), PBUF_RAM);
    ```
    
    - ## Param
        
        - `PBUF_TRANSPORT` tells lwip to reserve space for a header to tell the program if we are using UDP or TCP 
        - `PBUF_RAM` allocate memory from RAM
    - `pbuf` is a structure from lwip used to arrange data packets in a way lwip can transport and understand
    - `pbuf_alloc()`is a lwip function that allocates memory 
    - creates the packet ready for transport

&nbsp;

- ```C
    err_t err = udp_sendto(udp_sender, p, &client_ip, UDP_PORT);
    ```
    

- - ## Param
        
        - `udp_sender` pointer to the socket number the server is using for udp 
        - `p` pointer to pbuf packet previously made
        - `client_ip` address of the structure holding the client IP
        - `UDP_PORT` udp port number that's being used
    - sends the packet created from `udp_sender` to the client IP and port

* * *

```C
void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                       const ip_addr_t *addr, u16_t port)
```

## Param

- `arg` data that can be attached to the specific callback - useful if we are using multiple servers or clients but for our case not needed so set to NULL
- `pcb` pointer to the structure holding the socket number of the server that received the packet
- `p` pointer to the structure of the data packet arranged in pbuf that is received from client
- `addr` pointer to the structure of the ip address of the client 
- `port` the port number of the client

When a packet is received from the client the callback function is called. This stores all the needed info about the client and what was sent. Then it sends back an acknowledgment that it received the packet to the client. 

- ```C
    struct pbuf *ack_p = pbuf_alloc(PBUF_TRANSPORT, strlen(ack), PBUF_RAM);
    ```
    
- ## Param
    
    - `PBUF_TRANSPORT` tells lwip if we are using UDP or TCP
    - `PBUF_RAM` allocate memory from RAM
- making a structure for our ACK being sent to client

* * *

# MAIN

&nbsp;

```C
stdio_init_all()
```

Turns on the output/input systems on the pico so the program can use standard I/O

```C
cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
```

From the `pico_cyw43_arch` library.

Puts the pico into AP mode

&nbsp;`CYW43_AUTH_WPA2_AES_PSK` tells it what type of network, authentication and padding is being used

```C
struct netif *ap_if = &cyw43_state.netif[CYW43_ITF_AP];
```

When assigning an IP address `netif` automatically assigns it to the loopback interface this is for sending information through internal programs. We need it to be sent externally through the AP interface. This structure forces the `netif` function to assign the IP address to the AP interface. 

`struct netif` is from the `lwip/netif` library 

`cyw43_state` and `CYW43_ITF_AP` are from the `pico/cyw43_arch` library

```C
ip4_addr_t ipaddr, nm, gw;
```

Structure from the lwip library to save the IP, netmask, and gateway of the server.

netmask `nw` separates the network portion of an IP address from the host portion. It is used so a device can determine whether another IP address is on the same local network or must be reached through a router.

gateway `gw` is used if we are sending data to an IP outside of the network. We are not doing this so its not needed in our program and is set to the default number. 

```C
err_t err = udp_bind(udp_sender, IP_ADDR_ANY, UDP_PORT);
```

Binds the UDP socket of the server to the IP address and port designated for the server.

`IP_ADDR_ANY` means the server accepts packets sent the this port from any local interface. If another system on the rocket is also sending UDP packets, we could restrict the program to only process packets from a specific client IP to prevent other packets from interfering. 

```C
udp_recv(udp_sender, udp_recv_callback, NULL);
```

`udp_recv` is an lwip function that is called when a UDP packet arrives on the socket

## Param

- `udp_sender` the socket on the server the packet arrived at
- `udp_recv_callback` our callback function written earlier is executed 
- `NULL` normally arg goes here to assign specific data to this packet but we are not using that here so it is set to NULL

```C
uint32_t now = to_ms_since_boot(get_absolute_time());
```

gets the time since the device booted in ms

is part of the `pico/stdlib` library

```C
cyw43_arch_poll();
```

Very important function

This tells the program to execute all of the network functions. Basically "updates" all of our network functions. 

This can cause `udp_recv_callback()` to run if a UDP packet arrived, and it lets previously requested `udp_sendto()` network work continue.

```C
cyw43_arch_deinit()
```

This function:

1.  Stops the Wi-Fi driver
    
2.  Stops the lwIP networking stack
    
3.  Frees networking resources
    
4.  turns off the Wi-Fi chip
    

So it basically undoes what `cyw43_arch_init()` started.
