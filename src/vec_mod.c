#include "vec_mod.h"
#include <string.h>

VecU8 vec_u8_new(void) {
    VecU8 new = {0};
    return new;
}

void vec_u8_push(VecU8 *vec_u8, const void *src, uint16_t src_len) {
    if (vec_u8->length + src_len > VECU8_MAX_CAPACITY) {
        return;
    }
    memcpy(vec_u8->data + vec_u8->length, src, src_len);
    vec_u8->length += src_len;
    return;
}
void vec_u8_push_u8(VecU8 *vec_u8, uint8_t value) {
    vec_u8_push(vec_u8, &value, 1);
}

static inline uint32_t swap32(uint32_t value) {
    return ((value & 0x000000FFU) << 24)
         | ((value & 0x0000FF00U) <<  8)
         | ((value & 0x00FF0000U) >>  8)
         | ((value & 0xFF000000U) >> 24);
}
void vec_u8_push_float(VecU8 *vec_u8, float value) {
    uint32_t u32;
    memcpy(&u32, &value, sizeof(u32));
    u32 = swap32(u32);
    vec_u8_push(vec_u8, &u32, sizeof(u32));
}

static inline uint16_t swap16(const uint16_t value) {
    return ((value & 0x00FFU) << 8)
         | ((value & 0xFF00U) >> 8);
}
void vec_u8_push_u16(VecU8 *vec_u8, uint16_t value) {
    uint16_t u16 = swap16(value);
    vec_u8_push(vec_u8, &u16, sizeof(u16));
}

bool vec_u8_starts_with(const VecU8 *src, const uint8_t *com, uint16_t com_len) {
    if (src->length < com_len) {
        return false;
    }
    return memcmp(src->data+src->head, com, com_len) == 0;
}

void vec_u8_rm_front(VecU8 *vec_u8, uint16_t size) {
    vec_u8->head += size;
    vec_u8->length -= size;
}

// void vec_u8_drain(VecU8 *vec_u8, uint16_t start, uint16_t end) {
//     if (start > end || end > vec_u8->length) {
//         return;
//     }
//     VecU8 new_vec = vec_u8_new();
//     if (start > 0) {
//         vec_u8_push(&new_vec, vec_u8->data, start);
//     }
//     uint16_t tail_len = vec_u8->length - end;
//     if (tail_len > 0) {
//         vec_u8_push(&new_vec, vec_u8->data + end, tail_len);
//     }
//     *vec_u8 = new_vec;
// }
