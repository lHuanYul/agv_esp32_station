#include "packet_mod.h"

/**
  * 傳輸和接收循環緩衝區
  * 
  * Transmission and reception ring buffers
  */
TrReBuffer transfer_buffer = {0};
TrReBuffer receive_buffer = {0};

/**
  * 生成一個新的UART封包，包含起始碼與結束碼
  * 
  * Create a new UART packet with start and end codes
  */
UartPacket uart_packet_new(VecU8 *data) {
    UartPacket packet;
    packet.start = PACKET_START_CODE;
    packet.data_vec_u8 = (VecU8){0};
    vec_u8_push(&packet.data_vec_u8, data->data, data->length);
    packet.end = PACKET_END_CODE;
    return packet;
}

/**
  * 生成一個錯誤封包，用於表示封包解析失敗
  * 
  * Create an error packet to indicate packet parsing failure
  */
UartPacket uart_packet_new_error(void) {
    UartPacket packet;
    packet.start = 0;
    return packet;
}

/**
  * 檢查封包是否為錯誤封包
  * 
  * Check if the packet is an error packet
  */
bool packet_error(const UartPacket *packet) {
    if (packet->start == 0) {
        return true;
    }
    return false;
}

VecU8 uart_packet_get_vec(const UartPacket *packet) {
    VecU8 vec_u8 = {0};
    vec_u8_push(&vec_u8, packet->data_vec_u8.data, packet->data_vec_u8.length);
    return vec_u8;
}

/**
  * 根據原始資料向量打包成UART封包，並移除起始與結束碼後重新封裝
  * 
  * Pack raw data vector into UART packet, stripping start and end codes before repacking
  */
UartPacket uart_packet_pack(const VecU8 *vec_u8) {
    if (
        (vec_u8->length < 2 || vec_u8->data[0] != PACKET_START_CODE) ||
        (vec_u8->data[vec_u8->length - 1] != PACKET_END_CODE)
    ) {
        return uart_packet_new_error();
    }
    VecU8 data_vec = {0};
    vec_u8_push(&data_vec, vec_u8->data + 1, vec_u8->length - 2);
    return uart_packet_new(&data_vec);
}

void uart_packet_add_data(UartPacket *packet, const VecU8 *vec_u8) {
    vec_u8_push(&packet->data_vec_u8, vec_u8->data, vec_u8->length);
}

/**
  * 解包UART封包，將封包前後碼與資料合併為一個字節向量
  * 
  * Unpack UART packet into a byte vector including start, data, and end codes
  */
VecU8 uart_packet_unpack(const UartPacket *packet) {
    VecU8 vec_u8 = {0};
    vec_u8_push(&vec_u8, &packet->start, 1);
    vec_u8_push(&vec_u8, packet->data_vec_u8.data, packet->data_vec_u8.length);
    vec_u8_push(&vec_u8, &packet->end, 1);
    return vec_u8;
}

/**
  * 建立傳輸/接收環形緩衝區，初始化頭指標與計數
  * 
  * Create a transmit/receive ring buffer, initialize head and count
  */
TrReBuffer trRe_buffer_new(void) {
    TrReBuffer tr_re_buffer;
    tr_re_buffer.head = 0;
    tr_re_buffer.length = 0;
    return tr_re_buffer;
}

/**
  * 將封包推入環形緩衝區，若已滿則返回false
  * 
  * Push a packet into the ring buffer; return false if buffer is full
  */
bool trRe_buffer_push(TrReBuffer *tr_re_buffer, const UartPacket *packet) {
    if (tr_re_buffer->length >= TR_RE_PKT_BUFFER_CAP) return false;
    uint8_t tail = (tr_re_buffer->head + tr_re_buffer->length) % TR_RE_PKT_BUFFER_CAP;
    tr_re_buffer->packet[tail] = *packet;
    tr_re_buffer->length++;
    return true;
}

/**
  * 從環形緩衝區彈出一個封包
  * 
  * Pop a packet from the ring buffer
  */
UartPacket trRe_buffer_pop_firstHalf(const TrReBuffer *tr_re_buffer) {
    return tr_re_buffer->packet[tr_re_buffer->head];
}
void trRe_buffer_pop_secondHalf(TrReBuffer *tr_re_buffer) {
    if (tr_re_buffer->length == 0) return;
    tr_re_buffer->head = (tr_re_buffer->head + 1) % TR_RE_PKT_BUFFER_CAP;
    tr_re_buffer->length--;
}
