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

// ----------------------------------------------------------------------------------------------------

typedef struct UartPacket UartPacket;
typedef struct UartPacket {
    uint8_t     start;
    VecU8       datas;
    uint8_t     end;
} UartPacket;
bool uart_pkt_add_data(UartPacket *self, VecU8 *vec_u8);
bool uart_pkt_get_data(const UartPacket *self, VecU8 *vec_u8);
bool uart_pkt_pack(UartPacket *self, VecU8 *vec_u8);
bool uart_pkt_unpack(const UartPacket *self, VecU8 *vec_u8);
UartPacket uart_packet_new(void);

// ----------------------------------------------------------------------------------------------------

#define UART_TRCV_BUF_CAP 5
typedef struct UartTrcvBuf UartTrcvBuf;
typedef struct UartTrcvBuf {
    UartPacket  packets[UART_TRCV_BUF_CAP];
    uint8_t     head;
    uint8_t     len;
} UartTrcvBuf;
bool uart_trcv_buf_push(UartTrcvBuf *self, const UartPacket *pkt);
bool uart_trcv_buf_get_front(const UartTrcvBuf *self, UartPacket *pkt);
bool uart_trcv_buf_pop_front(UartTrcvBuf *self, UartPacket *pkt);
UartTrcvBuf uart_trcv_buf_new(void);
extern UartTrcvBuf uart_trsm_pkt_buf;
extern UartTrcvBuf uart_recv_pkt_buf;
void uart_trcv_buf_init(void);

#endif
