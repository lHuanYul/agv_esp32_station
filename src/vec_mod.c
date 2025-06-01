#include "vec_mod.h"
#include <string.h>

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
static bool bytes_push(VecU8 *self, const void *src, uint16_t src_len) {
    if (self->len + src_len > VECU8_MAX_CAPACITY) {
        return 0;
    }
    memcpy(self->data + self->len, src, src_len);
    self->len += src_len;
    return 1;
}
/**
 * @brief 將單一位元組推入 VecU8
 *        Pushes a single byte into VecU8
 *
 * @param self 指向 VecU8 實例的指標 (pointer to VecU8 instance)
 * @param value 要推入的單一位元組 (value of byte to push)
 * @return true 成功推入 (successfully pushed)
 * @return false 推入失敗（超過容量） (failed to push, exceeds capacity)
 */
static inline bool bytes_push_byte(VecU8 *self, uint8_t value) {
    return bytes_push(self, &value, 1);
}

/**
 * @brief 交換 16-bit 整數的大小端 (endianness swap)
 *        Swaps byte order of a 16-bit unsigned integer
 *
 * @param value 要交換大小端的 16-bit 值 (16-bit value to swap)
 * @return uint16_t 交換後的 16-bit 值 (byte-swapped 16-bit value)
 */
static inline uint16_t swap16(const uint16_t value) {
    return ((value & 0x00FFU) << 8)
         | ((value & 0xFF00U) >> 8);
}
/**
 * @brief 將 uint16_t 轉換為大端序並推入 VecU8
 *        Converts a 16-bit unsigned integer to big-endian and pushes into VecU8
 *
 * @param self 指向 VecU8 實例的指標 (pointer to VecU8 instance)
 * @param value 要推入的 16-bit 原始值 (original 16-bit value)
 * @return true 成功推入 (successfully pushed)
 * @return false 推入失敗（超過容量） (failed to push, exceeds capacity)
 */
static inline bool bytes_push_u16(VecU8 *self, uint16_t value) {
    uint16_t u16 = swap16(value);
    return bytes_push(self, &u16, sizeof(u16));
}

/**
 * @brief 交換 32-bit 整數的大小端 (endianness swap)
 *        Swaps byte order of a 32-bit unsigned integer
 *
 * @param value 要交換大小端的 32-bit 值 (32-bit value to swap)
 * @return uint32_t 交換後的 32-bit 值 (byte-swapped 32-bit value)
 */
static inline uint32_t swap32(uint32_t value) {
    return ((value & 0x000000FFU) << 24)
         | ((value & 0x0000FF00U) <<  8)
         | ((value & 0x00FF0000U) >>  8)
         | ((value & 0xFF000000U) >> 24);
}
/**
 * @brief 將 float 轉換為 IEEE-754 大端序並推入 VecU8
 *        Converts a float to IEEE-754 big-endian representation and pushes into VecU8
 *
 * @param self 指向 VecU8 實例的指標 (pointer to VecU8 instance)
 * @param value 要推入的 float 原始值 (original float value)
 * @return true 成功推入 (successfully pushed)
 * @return false 推入失敗（超過容量） (failed to push, exceeds capacity)
 */
static bool bytes_push_f32(VecU8 *self, float value) {
    uint32_t u32;
    uint8_t u32_len = sizeof(u32);
    memcpy(&u32, &value, u32_len);
    u32 = swap32(u32);
    return bytes_push(self, &u32, u32_len);
}
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
static bool bytes_starts_with(const VecU8 *self, const uint8_t *pre, uint16_t pre_len) {
    if (self->len < pre_len) {
        return false;
    }
    return memcmp(self->data+self->head, pre, pre_len) == 0;
}

/**
 * @brief 將 VecU8 起始處前移指定大小 (移除前方資料)
 *        Advances the start of VecU8 by specified size (removes front data)
 *
 * @param self 指向 VecU8 實例的指標 (pointer to VecU8 instance)
 * @param size 要移除的位元組數 (number of bytes to remove)
 * @return true 始終回傳 true (always returns true)
 */
static bool bytes_rm_front(VecU8 *self, uint16_t size) {
    self->head += size;
    self->len -= size;
    return 1;
}

/**
 * @brief 初始化並回傳新的 VecU8 實例
 *        Initializes and returns a new VecU8 instance
 *
 * @return VecU8 新的 VecU8 結構 (the initialized VecU8 structure)
 */
VecU8 vec_u8_new(void) {
    VecU8 vec = {0};
    vec.push         = bytes_push;
    vec.push_byte    = bytes_push_byte;
    vec.push_u16     = bytes_push_u16;
    vec.push_f32     = bytes_push_f32;
    vec.start_with   = bytes_starts_with;
    vec.rm_front     = bytes_rm_front;
    return vec;
}
