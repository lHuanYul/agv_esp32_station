#ifndef WIFI_TR_RE_H
#define WIFI_TR_RE_H

#include <stdint.h>
#include "vec_mod.h"

void wifi_tcp_write_task(void);
void wifi_udp_write_task(void);
void wifi_transceive_setup(void);

#endif