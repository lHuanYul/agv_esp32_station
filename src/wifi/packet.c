#include "wifi/packet.h"

/**
 * @brief 全域傳輸緩衝區
 *        Global transmit ring buffer
 */
WifiTrcvBuf wifi_tcp_transmit_buffer = {0};
WifiTrcvBuf wifi_udp_transmit_buffer = {0};
/**
 * @brief 全域接收緩衝區
 *        Global receive ring buffer
 */
WifiTrcvBuf wifi_tcp_receive_buffer = {0};
WifiTrcvBuf wifi_udp_receive_buffer = {0};

/**
 * @brief 生成一個新的 Wifi 封包，包含起始碼與結束碼
 *        Create a new Wifi packet including start and end codes
 *
 * @param data 指向要封裝的原始資料向量 (input data vector)
 * @return WifiPacket 已封裝的 Wifi 封包 (packed Wifi packet)
 */
WifiPacket wifi_packet_new(const ip4_addr_t *ip, const VecU8 *vec_u8) {
    WifiPacket packet;
    packet.ip = *ip;
    packet.data = vec_u8_new();
    vec_u8_push(&packet.data, vec_u8->data, vec_u8->len);
    return packet;
}

/**
 * @brief 根據原始資料向量打包成 Wifi 封包，並移除起始與結束碼後重新封裝
 *        Pack raw data vector into Wifi packet, stripping start and end codes before repacking
 *
 * @param vec_u8 包含封包起始碼與結束碼的資料向量 (input byte vector with start/end codes)
 * @param packet 輸出參數，接收封裝後的 Wifi 封包 (output packed Wifi packet)
 * @return bool 是否封包成功 (true if pack successful, false otherwise)
 */
VecU8 wifi_packet_get_data(const WifiPacket *packet) {
    VecU8 vec_u8 = vec_u8_new();
    vec_u8_push(&vec_u8, packet->data.data, packet->data.len);
    return vec_u8;
}

/**
 * @brief 向現有 Wifi 封包中新增資料
 *        Add data to existing Wifi packet
 *
 * @param packet 指向要新增資料的 Wifi 封包 (input packet)
 * @param vec_u8 要新增的資料向量 (input data vector)
 */
void wifi_packet_add_data(WifiPacket *packet, const VecU8 *vec_u8) {
    vec_u8_push(&packet->data, vec_u8->data, vec_u8->len);
}

/**
 * @brief 解包 Wifi 封包，將起始碼、資料與結束碼合併為一個資料向量
 *        Unpack Wifi packet into a byte vector including start, data, and end codes
 *
 * @param packet 指向要解包的 Wifi 封包 (input packet)
 * @return VecU8 包含完整封包的資料向量 (vector containing full packet bytes)
 */
void wifi_packet_unpack(const WifiPacket *packet, ip4_addr_t *ip, VecU8 *vec_u8) {
    *ip = packet->ip;
    *vec_u8 = vec_u8_new();
    vec_u8_push(vec_u8, packet->data.data,packet->data.len);
}

/**
 * @brief 建立傳輸/接收環形緩衝區，初始化頭指標與計數
 *        Create a transmit/receive ring buffer, initialize head index and length
 *
 * @return WifiTrcvBuf 初始化後的環形緩衝區 (initialized ring buffer)
 */
WifiTrcvBuf wifi_trcv_buffer_new(void) {
    WifiTrcvBuf transceive_buffer;
    transceive_buffer.head = 0;
    transceive_buffer.length = 0;
    return transceive_buffer;
}

bool wifi_trcv_buffer_get_front(WifiTrcvBuf *buffer, WifiPacket *packet) {
    if (buffer->length == 0) return 0;
    *packet = buffer->packet[buffer->head];
    return 1;
}

/**
 * @brief 將封包推入環形緩衝區，若已滿則返回 false
 *        Push a packet into the ring buffer; return false if buffer is full
 *
 * @param buffer 指向環形緩衝區的指標 (input/output ring buffer)
 * @param packet 要推入緩衝區的 UART 封包 (input UART packet)
 * @return bool 是否推入成功 (true if push successful, false if buffer full)
 */
bool wifi_trcv_buffer_push(WifiTrcvBuf *buffer, const WifiPacket *packet) {
    if (buffer->length >= WIFI_TRCV_BUF_CAP) return false;
    uint8_t tail = (buffer->head + buffer->length) % WIFI_TRCV_BUF_CAP;
    buffer->packet[tail] = *packet;
    buffer->length++;
    return true;
}

/**
 * @brief 從環形緩衝區彈出一個封包資料
 *        Pop a packet from the ring buffer
 *
 * @param buffer 指向環形緩衝區的指標 (input/output ring buffer)
 * @param packet 輸出參數，接收彈出的 UART 封包 (output popped UART packet)
 * @return bool 是否彈出成功 (true if pop successful, false if buffer empty)
 */
bool wifi_trcv_buffer_pop(WifiTrcvBuf *buffer, WifiPacket *packet) {
    if (buffer->length == 0) return 0;
    *packet = buffer->packet[buffer->head];
    if (--buffer->length == 0) {
        buffer->head = 0;
    } else {
        buffer->head = (buffer->head + 1) % WIFI_TRCV_BUF_CAP;
    }
    return 1;
}
