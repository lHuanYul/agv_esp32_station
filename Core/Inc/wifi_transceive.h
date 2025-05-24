#ifndef WIFI_TR_RE_H
#define WIFI_TR_RE_H

#include <stdint.h>
#include "vec_mod.h"

void wifi_transceive_setup(void);
void wifi_udp_transmit(void);
void wifi_tcp_transmit(void);

#endif