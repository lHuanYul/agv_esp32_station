#include "vec_mod.h"
#include <string.h>

static bool bytes_push(VecU8 *v_u8, const void *src, uint16_t src_len) {
    if (v_u8->len + src_len > VECU8_MAX_CAPACITY) {
        return 0;
    }
    memcpy(v_u8->data + v_u8->len, src, src_len);
    v_u8->len += src_len;
    return 1;
}
static inline bool bytes_push_byte(VecU8 *v_u8, uint8_t value) {
    return bytes_push(v_u8, &value, 1);
}

static inline uint16_t swap16(const uint16_t value) {
    return ((value & 0x00FFU) << 8)
         | ((value & 0xFF00U) >> 8);
}
static bool bytes_push_u16(VecU8 *v_u8, uint16_t value) {
    uint16_t u16 = swap16(value);
    return bytes_push(v_u8, &u16, sizeof(u16));
}

static inline uint32_t swap32(uint32_t value) {
    return ((value & 0x000000FFU) << 24)
         | ((value & 0x0000FF00U) <<  8)
         | ((value & 0x00FF0000U) >>  8)
         | ((value & 0xFF000000U) >> 24);
}
static bool bytes_push_float(VecU8 *v_u8, float value) {
    uint32_t u32;
    uint8_t u32_len = sizeof(u32);
    memcpy(&u32, &value, u32_len);
    u32 = swap32(u32);
    return bytes_push(v_u8, &u32, u32_len);
}

static bool bytes_starts_with(const VecU8 *v_u8, const uint8_t *com, uint16_t com_len) {
    if (v_u8->len < com_len) {
        return false;
    }
    return memcmp(v_u8->data+v_u8->head, com, com_len) == 0;
}

static bool bytes_rm_front(VecU8 *v_u8, uint16_t size) {
    v_u8->head += size;
    v_u8->len -= size;
    return 1;
}

VecU8 vec_u8_new(void) {
    VecU8 vec = {0};
    vec.push         = bytes_push;
    vec.push_byte    = bytes_push_byte;
    vec.push_u16     = bytes_push_u16;
    vec.push_f32     = bytes_push_float;
    vec.start_with   = bytes_starts_with;
    vec.rm_front     = bytes_rm_front;
    return vec;
}
