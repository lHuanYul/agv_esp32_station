#ifndef WIFI_PACKET_MOD_H
#define WIFI_PACKET_MOD_H

#include "vec_mod.h"
#include "esp_netif.h"
#include "lwip/ip4_addr.h"

typedef struct {
    ip4_addr_t ip;
    VecU8 data;
} WifiPacket;
WifiPacket wifi_packet_new(const ip4_addr_t *ip, const VecU8 *vec_u8);
VecU8 wifi_packet_get_data(const WifiPacket *packet);
void wifi_packet_add_data(WifiPacket *packet, const VecU8 *vec_u8);
void wifi_packet_unpack(const WifiPacket *packet, ip4_addr_t *ip, VecU8 *vec_u8);

#define WIFI_TRCV_BUF_CAP 5

typedef struct {
    WifiPacket  packet[WIFI_TRCV_BUF_CAP];
    uint8_t     head;
    uint8_t     length;
} WifiTrcvBuf;
extern WifiTrcvBuf wifi_tcp_transmit_buffer;
extern WifiTrcvBuf wifi_udp_transmit_buffer;
extern WifiTrcvBuf wifi_tcp_receive_buffer;
extern WifiTrcvBuf wifi_udp_receive_buffer;
WifiTrcvBuf wifi_trcv_buffer_new(void);
bool wifi_trcv_buffer_push(WifiTrcvBuf *buffer, const WifiPacket *packet);
bool wifi_trcv_buffer_pop(WifiTrcvBuf *buffer, WifiPacket *packet);
bool wifi_trcv_buffer_pop_firstHalf(const WifiTrcvBuf *buffer, WifiPacket *packet);
bool wifi_trcv_buffer_pop_secondHalf(WifiTrcvBuf *buffer);

#endif
