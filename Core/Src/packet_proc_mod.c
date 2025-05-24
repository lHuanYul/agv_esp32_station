#include "packet_proc_mod.h"
#include "packet_mod.h"

float f32_test = 1;
uint16_t u16_test = 1;

void rspdw(VecU8* vec_u8) {
    vec_u8_push(vec_u8, &(uint8_t){0x01}, 1);
    vec_u8_push(vec_u8, &(uint8_t){0x00}, 1);
    vec_u8_push_float(vec_u8, f32_test);
    f32_test++;
}
void radcw(VecU8* vec_u8) {
    vec_u8_push(vec_u8, &(uint8_t){0x01}, 1);
    vec_u8_push(vec_u8, &(uint8_t){0x05}, 1);
    vec_u8_push_u16(vec_u8, u16_test);
    u16_test++;
}

void uart_tr_packet_proccess(void) {

}

void uart_re_pkt_proc_data_store(VecU8 *vec_u8);
void uart_re_packet_proccess(uint8_t count) {

}

void uart_re_pkt_proc_data_store(VecU8 *vec_u8) {

}

void wifi_tr_packet_proccess(void) {

}

void wifi_re_pkt_proc_data_store(VecU8 *vec_u8);
void wifi_re_packet_proccess(uint8_t count) {

}

void wifi_re_pkt_proc_data_store(VecU8 *vec_u8) {
    
}
