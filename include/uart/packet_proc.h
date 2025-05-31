#ifndef PACKET_PROC_MOD_H
#define PACKET_PROC_MOD_H

#include <stdint.h>
#include <stdbool.h>
#include "packet.h"

void uart_transmit_pkt_proc(void);
void uart_receive_pkt_proc(uint8_t count);

#endif
