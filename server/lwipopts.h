#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

/* NO_SYS=1 for bare metal */
#define NO_SYS                         1
#define LWIP_SOCKET                    0
#define LWIP_NETCONN                   0

/* Memory options */
#define MEM_ALIGNMENT                  4
#define MEM_SIZE                       (16 * 1024)
#define MEMP_NUM_PBUF                  16
#define MEMP_NUM_UDP_PCB               6
#define PBUF_POOL_SIZE                 24
#define PBUF_POOL_BUFSIZE              1700

/* IP options */
#define LWIP_IPV4                      1
#define LWIP_IPV6                      0
#define IP_REASSEMBLY                  0
#define IP_FRAG                        0
#define LWIP_NETIF_API                 0


/* UDP */
#define LWIP_UDP                       1
#define LWIP_TCP                       0

/* DHCP */
#define LWIP_DHCP                      1

/* Checksums */
#define CHECKSUM_GEN_IP                1
#define CHECKSUM_GEN_UDP               1
#define CHECKSUM_CHECK_IP              1
#define CHECKSUM_CHECK_UDP             1

/* Debug (optional) */
#define LWIP_DEBUG                     0

#endif /* _LWIPOPTS_H */
