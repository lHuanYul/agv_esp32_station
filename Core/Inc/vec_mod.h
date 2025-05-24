#ifndef USER_VEC_U8_H
#define USER_VEC_U8_H

#include <stdint.h>
#include <stdbool.h>

#define VECU8_MAX_CAPACITY  255

typedef struct {
    uint8_t data[VECU8_MAX_CAPACITY];
    uint16_t head;
    uint16_t length;
} VecU8;

#define vec_u8_starts_with_s(src, com) \
    vec_u8_starts_with((src), (com), sizeof(com))

void vec_u8_push(VecU8 *vec_u8, const void *src, uint16_t src_len);
void vec_u8_push_float(VecU8 *vec_u8, float value);
void vec_u8_push_u16(VecU8 *vec_u8, uint16_t value);
bool vec_u8_starts_with(const VecU8 *src, const uint8_t *com, uint16_t com_len);
void vec_u8_rm_front(VecU8 *vec_u8, uint16_t size);
void vec_u8_drain(VecU8 *vec_u8, uint16_t start, uint16_t end);

#endif
