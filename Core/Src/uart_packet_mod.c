#include "uart_packet_mod.h"

/**
 * @brief 生成一個新的 UART 封包，包含起始碼與結束碼
 *        Create a new UART packet including start and end codes
 *
 * @param data 指向要封裝的原始資料向量 (input data vector)
 * @return UartPacket 已封裝的 UART 封包 (packed UART packet)
 */
UartPacket uart_packet_new(const VecU8 *data) {
    UartPacket packet;
    packet.start = PACKET_START_CODE;
    packet.data_vec_u8 = vec_u8_new();
    vec_u8_push(&packet.data_vec_u8, data->data, data->length);
    packet.end = PACKET_END_CODE;
    return packet;
}

/**
 * @brief 根據原始資料向量打包成 UART 封包，並移除起始與結束碼後重新封裝
 *        Pack raw data vector into UART packet, stripping start and end codes before repacking
 *
 * @param vec_u8 包含封包起始碼與結束碼的資料向量 (input byte vector with start/end codes)
 * @param packet 輸出參數，接收封裝後的 UART 封包 (output packed UART packet)
 * @return bool 是否封包成功 (true if pack successful, false otherwise)
 */
VecU8 uart_packet_get_data(const UartPacket *packet) {
    VecU8 vec_u8 = vec_u8_new();
    vec_u8_push(&vec_u8, packet->data_vec_u8.data, packet->data_vec_u8.length);
    return vec_u8;
}

/**
 * @brief 向現有 UART 封包中新增資料
 *        Add data to existing UART packet
 *
 * @param packet 指向要新增資料的 UART 封包 (input packet)
 * @param vec_u8 要新增的資料向量 (input data vector)
 */
void uart_packet_add_data(UartPacket *packet, const VecU8 *vec_u8) {
    vec_u8_push(&packet->data_vec_u8, vec_u8->data, vec_u8->length);
}

/**
 * @brief 根據原始資料向量打包成 UART 封包，並移除起始與結束碼後重新封裝
 *        Pack raw data vector into UART packet, stripping start and end codes before repacking
 *
 * @param vec_u8 包含封包起始碼與結束碼的資料向量 (input byte vector with start/end codes)
 * @param packet 輸出參數，接收封裝後的 UART 封包 (output packed UART packet)
 * @return bool 是否封包成功 (true if pack successful, false otherwise)
 */
bool uart_packet_pack(const VecU8 *vec_u8, UartPacket *packet) {
    if (
        (vec_u8->length < 2 || vec_u8->data[0] != PACKET_START_CODE) ||
        (vec_u8->data[vec_u8->length - 1] != PACKET_END_CODE)
    ) {
        return 0;
    }
    VecU8 data_vec = vec_u8_new();
    vec_u8_push(&data_vec, vec_u8->data + 1, vec_u8->length - 2);
    *packet = uart_packet_new(&data_vec);
    return 1;
}

/**
 * @brief 解包 UART 封包，將起始碼、資料與結束碼合併為一個資料向量
 *        Unpack UART packet into a byte vector including start, data, and end codes
 *
 * @param packet 指向要解包的 UART 封包 (input packet)
 * @return VecU8 包含完整封包的資料向量 (vector containing full packet bytes)
 */
VecU8 uart_packet_unpack(const UartPacket *packet) {
    VecU8 vec_u8 = vec_u8_new();
    vec_u8_push(&vec_u8, &packet->start, 1);
    vec_u8_push(&vec_u8, packet->data_vec_u8.data, packet->data_vec_u8.length);
    vec_u8_push(&vec_u8, &packet->end, 1);
    return vec_u8;
}

/**
 * @brief 全域傳輸緩衝區
 *        Global transmit ring buffer
 */
UartTrcvBuf uart_transmit_buffer = {0};

/**
 * @brief 全域接收緩衝區
 *        Global receive ring buffer
 */
UartTrcvBuf uart_receive_buffer = {0};

/**
 * @brief 建立傳輸/接收環形緩衝區，初始化頭指標與計數
 *        Create a transmit/receive ring buffer, initialize head index and length
 *
 * @return UartTrcvBuf 初始化後的環形緩衝區 (initialized ring buffer)
 */
UartTrcvBuf uart_trcv_buffer_new(void) {
    UartTrcvBuf transceive_buffer;
    transceive_buffer.head = 0;
    transceive_buffer.length = 0;
    return transceive_buffer;
}

/**
 * @brief 將封包推入環形緩衝區，若已滿則返回 false
 *        Push a packet into the ring buffer; return false if buffer is full
 *
 * @param buffer 指向環形緩衝區的指標 (input/output ring buffer)
 * @param packet 要推入緩衝區的 UART 封包 (input UART packet)
 * @return bool 是否推入成功 (true if push successful, false if buffer full)
 */
bool uart_trcv_buffer_push(UartTrcvBuf *buffer, const UartPacket *packet) {
    if (buffer->length >= UART_TRCV_BUF_CAP) return false;
    uint8_t tail = (buffer->head + buffer->length) % UART_TRCV_BUF_CAP;
    buffer->packet[tail] = *packet;
    buffer->length++;
    return true;
}

bool uart_trcv_buffer_get_front(UartTrcvBuf *buffer, UartPacket *packet) {
    if (buffer->length == 0) return 0;
    if (packet != NULL) *packet = buffer->packet[buffer->head];
    return 1;
}

/**
 * @brief 從環形緩衝區彈出一個封包資料
 *        Pop a packet from the ring buffer
 *
 * @param buffer 指向環形緩衝區的指標 (input/output ring buffer)
 * @param packet 輸出參數，接收彈出的 UART 封包 (output popped UART packet)
 * @return bool 是否彈出成功 (true if pop successful, false if buffer empty)
 */
bool uart_trcv_buffer_pop(UartTrcvBuf *buffer, UartPacket *packet) {
    if (buffer->length == 0) return 0;
    if (packet != NULL) *packet = buffer->packet[buffer->head];
    if (--buffer->length == 0) {
        buffer->head = 0;
    } else {
        buffer->head = (buffer->head + 1) % UART_TRCV_BUF_CAP;
    }
    return 1;
}
