#ifndef USER_VEC_U8_H
#define USER_VEC_U8_H

#include <stdint.h>
#include <stdbool.h>

#define VECU8_MAX_CAPACITY  255

typedef struct VecU8 VecU8;
typedef bool (*U8PushFn)        (      VecU8 *v_u8, const void     *src,  uint16_t src_len);
typedef bool (*U8PushU8Fn)      (      VecU8 *v_u8,       uint8_t  value);
typedef bool (*U8PushU16Fn)     (      VecU8 *v_u8,       uint16_t value);
typedef bool (*U8PushFloatFn)   (      VecU8 *v_u8,       float    value);
typedef bool (*U8StartWithFn)   (const VecU8 *v_u8, const uint8_t  *com,  uint16_t com_len);
typedef bool (*U8RmFrontFn)     (      VecU8 *v_u8,       uint16_t size);
typedef bool (*U8DrainFn)       (      VecU8 *v_u8,       uint16_t start, uint16_t end);
typedef struct VecU8 {
    uint8_t         data[VECU8_MAX_CAPACITY];
    uint16_t        head;
    uint16_t        len;
    U8PushFn        push;
    U8PushU8Fn      push_byte;
    U8PushU16Fn     push_u16;
    U8PushFloatFn   push_f32;
    U8StartWithFn   start_with;
    U8RmFrontFn     rm_front;
} VecU8;
VecU8 vec_u8_new(void);

#endif
