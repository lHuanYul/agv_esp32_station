#ifndef USER_PACKET_H
#define USER_PACKET_H

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

typedef struct {
    UartPacket  packet[UART_TRCV_BUF_CAP];
    uint8_t     head;
    uint8_t     length;
} UartTrcvBuf;
extern UartTrcvBuf transfer_buffer;
extern UartTrcvBuf receive_buffer;
UartTrcvBuf transceive_buffer_new(void);
bool transceive_buffer_push(UartTrcvBuf *transceive_buffer, const UartPacket *packet);
bool transceive_buffer_pop(UartTrcvBuf *buffer, UartPacket *packet);
bool transceive_buffer_pop_firstHalf(const UartTrcvBuf *buffer, UartPacket *packet);
bool transceive_buffer_pop_secondHalf(UartTrcvBuf *transceive_buffer);

#endif
