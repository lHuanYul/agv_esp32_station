#ifndef VEC_MOD_H
#define VEC_MOD_H
// ----------------------------------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
// ----------------------------------------------------------------------------------------------------

#define VECU8_MAX_CAPACITY  255

typedef struct VecU8 {
    uint8_t         data[VECU8_MAX_CAPACITY];
    uint16_t        head;
    uint16_t        len;
} VecU8;
void vec_u8_realign(VecU8 *self);
bool vec_u8_get_byte(const VecU8 *self, uint8_t *u8, uint16_t id);
bool vec_u8_starts_with(const VecU8 *self, const uint8_t *pre, uint16_t pre_len);
bool vec_u8_push(VecU8 *self, const void *src, uint16_t src_len);
bool vec_u8_push_byte(VecU8 *self, uint8_t value);
bool vec_u8_push_u16(VecU8 *self, uint16_t value);
bool vec_u8_push_f32(VecU8 *self, float value);
bool vec_u8_rm_range(VecU8 *self, uint16_t offset, uint16_t size);
VecU8 vec_u8_new(void);

#endif
