#ifndef VEC_MOD_H
#define VEC_MOD_H

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
    /**
     * @brief 將 src 指向的位元組組合並推入 VecU8 末端
     *        Pushes bytes from "src" into the end of VecU8
     *
     * @param self 指向 VecU8 實例的指標 (pointer to VecU8 instance)
     * @param src 指向要推入的資料緩衝區 (pointer to source data buffer)
     * @param src_len 要推入的資料長度 (length of source data)
     * @return true 成功推入 (successfully pushed)
     * @return false 推入失敗（超過容量） (failed to push, exceeds capacity)
     */
    U8PushFn        push;
    /**
     * @brief 將單一位元組推入 VecU8
     *        Pushes a single byte into VecU8
     *
     * @param self 指向 VecU8 實例的指標 (pointer to VecU8 instance)
     * @param value 要推入的單一位元組 (value of byte to push)
     * @return true 成功推入 (successfully pushed)
     * @return false 推入失敗（超過容量） (failed to push, exceeds capacity)
     */
    U8PushU8Fn      push_byte;
    /**
     * @brief 將 uint16_t 轉換為大端序並推入 VecU8
     *        Converts a 16-bit unsigned integer to big-endian and pushes into VecU8
     *
     * @param self 指向 VecU8 實例的指標 (pointer to VecU8 instance)
     * @param value 要推入的 16-bit 原始值 (original 16-bit value)
     * @return true 成功推入 (successfully pushed)
     * @return false 推入失敗（超過容量） (failed to push, exceeds capacity)
     */
    U8PushU16Fn     push_u16;
    /**
     * @brief 將 float 轉換為 IEEE-754 大端序並推入 VecU8
     *        Converts a float to IEEE-754 big-endian representation and pushes into VecU8
     *
     * @param self 指向 VecU8 實例的指標 (pointer to VecU8 instance)
     * @param value 要推入的 float 原始值 (original float value)
     * @return true 成功推入 (successfully pushed)
     * @return false 推入失敗（超過容量） (failed to push, exceeds capacity)
     */
    U8PushFloatFn   push_f32;
    /**
     * @brief 檢查 VecU8 起始位置是否以指定序列開頭
     *        Checks if VecU8 starts with a specified sequence of bytes
     *
     * @param self 指向 VecU8 實例的指標 (pointer to VecU8 instance)
     * @param pre 指向要比對的序列 (pointer to comparison sequence)
     * @param pre_len 序列長度 (length of comparison sequence)
     * @return true 若開頭吻合 (true if starts with sequence)
     * @return false 否則 (false otherwise)
     */
    U8StartWithFn   start_with;
    /**
     * @brief 將 VecU8 起始處前移指定大小 (移除前方資料)
     *        Advances the start of VecU8 by specified size (removes front data)
     *
     * @param self 指向 VecU8 實例的指標 (pointer to VecU8 instance)
     * @param size 要移除的位元組數 (number of bytes to remove)
     * @return true 始終回傳 true (always returns true)
     */
    U8RmFrontFn     rm_front;
} VecU8;
VecU8 vec_u8_new(void);

#endif
