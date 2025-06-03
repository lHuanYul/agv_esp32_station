#include "vec_mod.h"
// ----------------------------------------------------------------------------------------------------
#include <string.h>

/**
 * @brief   把 VecU8 裡的資料「搬到索引 0 開始」(head = 0)，並保留原本的儲存順序
 *
 * @param   self   指向要重新對齊 (realign) 的 VecU8
 * @return  true   重新對齊成功（或本來就不需動作）
 */
void vec_u8_realign(VecU8 *self) {
    if (self->len == 0 || self->head == 0) return;
    uint16_t first_part = VECU8_MAX_CAPACITY - self->head;
    if (first_part >= self->len) {
        memmove(self->data, self->data + self->head, self->len);
    } else {
        uint16_t second_part = self->len - first_part;
        memmove(self->data, self->data + self->head, first_part);
        memmove(self->data + first_part, self->data, second_part);
    }
    self->head = 0;
}

/**
 * @brief   從 VecU8（環狀緩衝區）中，讀取相對於 head 的第 num 個位元組
 *
 * @param   self  指向要讀取的 VecU8 物件（只讀）
 * @param   u8    用來存放讀出位元組的位址參考
 * @param   num   欲讀取的偏移量（相對 head 的索引，範圍須在 0 ~ len-1 之間）
 *
 * @return  true 表示成功，u8 已被填入對應值  
 *          false 表示失敗，通常是因為緩衝區為空或 num 超出範圍  
 */
bool vec_u8_get_byte(const VecU8 *self, uint8_t *u8, uint16_t id) {
    if(self->len == 0) return 0;
    if (id >= self->len) return 0;
    uint16_t idx = (self->head + id) % VECU8_MAX_CAPACITY;
    *u8 = self->data[idx];
    return 1;
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
bool vec_u8_starts_with(const VecU8 *self, const uint8_t *pre, uint16_t pre_len) {
    if (self->len < pre_len) {
        return 0;
    }
    if (self->head + pre_len <= VECU8_MAX_CAPACITY) {
        return memcmp(self->data + self->head, pre, pre_len) == 0;
    }
    uint16_t first_part  = VECU8_MAX_CAPACITY - self->head;
    uint16_t remaining = pre_len - first_part;
    if (memcmp(self->data + self->head, pre, first_part) != 0) {
        return 0;
    }
    return memcmp(self->data, pre + first_part, remaining) == 0;
}

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
bool vec_u8_push(VecU8 *self, const void *src, uint16_t src_len) {
    if (self->len + src_len > VECU8_MAX_CAPACITY) return 0;
    uint16_t tail = self->head + self->len;
    if (
        (tail >= VECU8_MAX_CAPACITY) ||
        (tail + src_len >= VECU8_MAX_CAPACITY)
    ) {
        vec_u8_realign(self);
        tail = self->len;
    }
    memcpy(self->data + tail, src, src_len);
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
inline bool vec_u8_push_byte(VecU8 *self, uint8_t value) {
    return vec_u8_push(self, &value, 1);
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
bool vec_u8_push_u16(VecU8 *self, uint16_t value) {
    uint16_t u16 = swap16(value);
    return vec_u8_push(self, &u16, sizeof(u16));
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
bool vec_u8_push_f32(VecU8 *self, float value) {
    uint32_t u32;
    uint8_t u32_len = sizeof(u32);
    memcpy(&u32, &value, u32_len);
    u32 = swap32(u32);
    return vec_u8_push(self, &u32, u32_len);
}

/**
 * @brief 從 VecU8 中移除指定範圍的資料
 *        Remove a range of bytes from VecU8
 * 
 * @param self   指向 VecU8 實例的指標 (pointer to VecU8 instance)
 * @param offset 要移除區段在目前資料（以 head 為起點）的起始位移 (start index, relative to head)
 * @param size   要移除的 byte 長度 (number of bytes to remove)
 * 
 * @return true  成功移除 (successfully removed)
 * @return false offset 超過目前資料長度或 realign 失敗 (offset out of range or realign failed)
 */
bool vec_u8_rm_range(VecU8 *self, uint16_t offset, uint16_t size) {
    if (offset >= self->len) return 0;
    if (size == 0) return 1;
    if (size >= self->len) {
        self->head = 0;
        self->len  = 0;
        return 1;
    }
    if (offset == 0) {
        self->head = (self->head + size) % VECU8_MAX_CAPACITY;
        self->len -= size;
        return 1;
    }
    if (offset + size >= self->len) {
        self->len = offset;
        return 1;
    }
    vec_u8_realign(self);
    memmove(self->data + offset, self->data + (offset + size), self->len - (offset + size));
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
    return vec;
}
