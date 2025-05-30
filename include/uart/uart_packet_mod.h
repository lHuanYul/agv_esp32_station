#ifndef USER_PACKET_H
#define USER_PACKET_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "vec_mod.h"

#define PACKET_START_CODE  ((uint8_t) '{')
#define PACKET_END_CODE    ((uint8_t) '}')

#define PACKET_MAX_SIZE VECU8_MAX_CAPACITY
#define PACKET_DATA_MAX_SIZE (VECU8_MAX_CAPACITY - 2)

typedef struct {
    uint8_t     start;
    VecU8       data_vec_u8;
    uint8_t     end;
} UartPacket;
UartPacket uart_packet_new(const VecU8 *data);
VecU8 uart_packet_get_data(const UartPacket *packet);
bool uart_packet_pack(const VecU8 *vec_u8, UartPacket *packet);
void uart_packet_add_data(UartPacket *packet, const VecU8 *vec_u8);
VecU8 uart_packet_unpack(const UartPacket *packet);

#define UART_TRCV_BUF_CAP 5
typedef struct UartTrcvBuf UartTrcvBuf;
typedef bool (*UartBufPushFn)       (struct UartTrcvBuf *buf, const UartPacket *pkt);
typedef bool (*UartBufGetFrontFn)   (struct UartTrcvBuf *buf,       UartPacket *pkt);
typedef bool (*UartBufPopFn)        (struct UartTrcvBuf *buf,       UartPacket *pkt);
typedef struct UartTrcvBuf {
    UartPacket  packet[UART_TRCV_BUF_CAP];
    uint8_t     head;
    uint8_t     length;
    UartBufPushFn     push;
    UartBufGetFrontFn get_front;
    UartBufPopFn      pop;
} UartTrcvBuf;
extern UartTrcvBuf uart_trsm_buf;
extern UartTrcvBuf uart_recv_buf;
UartTrcvBuf uart_trcv_buffer_new(void);
void uart_trcv_buf_init(void);
// bool        uart_trcv_buffer_push(UartTrcvBuf *transceive_buffer, const UartPacket *packet);
// bool        uart_trcv_buffer_get_front(UartTrcvBuf *buffer, UartPacket *packet);
// bool        uart_trcv_buffer_pop(UartTrcvBuf *buffer, UartPacket *packet);

#endif
