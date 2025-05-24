#ifndef USER_PACKET_H
#define USER_PACKET_H

#include <stdint.h>
#include <stdbool.h>
#include "vec_mod.h"

#define PACKET_START_CODE  ((uint8_t) '{')
#define PACKET_END_CODE    ((uint8_t) '}')
// 50
#define PACKET_MAX_SIZE VECU8_MAX_CAPACITY
#define PACKET_DATA_MAX_SIZE (VECU8_MAX_CAPACITY - 2)

#define TR_RE_PKT_BUFFER_CAP 10

typedef struct {
    uint8_t     start;
    VecU8       data_vec_u8;
    uint8_t     end;
} UartPacket;
UartPacket uart_packet_new(VecU8 *data);
bool packet_error(const UartPacket *packet);
VecU8 uart_packet_get_vec(const UartPacket *packet);
UartPacket uart_packet_pack(const VecU8 *vec_u8);
void uart_packet_add_data(UartPacket *packet, const VecU8 *vec_u8);
VecU8 uart_packet_unpack(const UartPacket *packet);

typedef struct {
    UartPacket  packet[TR_RE_PKT_BUFFER_CAP];
    uint8_t     head;
    uint8_t     length;
} TrReBuffer;
extern TrReBuffer transfer_buffer;
extern TrReBuffer receive_buffer;
TrReBuffer trRe_buffer_new(void);
bool trRe_buffer_push(TrReBuffer *tr_re_buffer, const UartPacket *packet);
UartPacket trRe_buffer_pop_firstHalf(const TrReBuffer *tr_re_buffer);
void trRe_buffer_pop_secondHalf(TrReBuffer *tr_re_buffer);

#endif
