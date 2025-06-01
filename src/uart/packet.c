#include "uart/packet.h"

/**
 * @brief 向現有 UART 封包中新增資料
 *        Add data to existing UART packet
 *
 * @param self 指向要新增資料的 UART 封包 (input packet)
 * @param vec_u8 要新增的資料向量 (input data vector)
 */
static void pkt_add_data(UartPacket *self, const VecU8 *vec_u8) {
    self->data_vec_u8.push(&self->data_vec_u8, vec_u8->data, vec_u8->len);
}

/**
 * @brief 從 UART 封包中取出資料向量 (Extract payload data from UART packet)
 *
 * 從輸入的 UartPacket 取得其內部儲存的資料向量 (data_vec_u8)，
 * 並回傳該 VecU8 實例。並不包含起始與結束碼 (start/end codes)。
 *
 * @param self  來源 UART 封包指標 (input UART packet pointer)
 * @return     VecU8 由封包提取出的資料向量 (the data vector extracted from the packet)
 */
static VecU8 pkt_get_data(const UartPacket *self) {
    VecU8 vec_u8 = vec_u8_new();
    vec_u8.push(&vec_u8, self->data_vec_u8.data, self->data_vec_u8.len);
    return vec_u8;
}

/**
 * @brief 根據原始資料向量打包成 UART 封包，並移除起始與結束碼後重新封裝
 *        Pack raw data vector into UART packet, stripping start and end codes before repacking
 *
 * @param self 輸出參數，接收封裝後的 UART 封包 (output packed UART packet)
 * @param vec_u8 包含封包起始碼與結束碼的資料向量 (input byte vector with start/end codes)
 * @return bool 是否封包成功 (true if pack successful, false otherwise)
 */
static bool pkt_pack(UartPacket *self, const VecU8 *vec_u8) {
    if (
        (vec_u8->len < 2 || vec_u8->data[0] != PACKET_START_CODE) ||
        (vec_u8->data[vec_u8->len - 1] != PACKET_END_CODE)
    ) {
        return 0;
    }
    VecU8 vec = vec_u8_new();
    vec.push(&vec, vec_u8->data + 1, vec_u8->len - 2);
    self->add_data(self, &vec);
    return 1;
}

/**
 * @brief 解包 UART 封包，將起始碼、資料與結束碼合併為一個資料向量
 *        Unpack UART packet into a byte vector including start, data, and end codes
 *
 * @param self 指向要解包的 UART 封包 (input packet)
 * @return VecU8 包含完整封包的資料向量 (vector containing full packet bytes)
 */
static VecU8 pkt_unpack(const UartPacket *self) {
    VecU8 vec = vec_u8_new();
    vec.push(&vec, &self->start, 1);
    vec.push(&vec, self->data_vec_u8.data, self->data_vec_u8.len);
    vec.push(&vec, &self->end, 1);
    return vec;
}

/**
 * @brief 生成一個新的 UART 封包，包含起始碼與結束碼
 *        Create a new UART packet including start and end codes
 *
 * @return UartPacket 已封裝的 UART 封包 (packed UART packet)
 */
UartPacket uart_packet_new(void) {
    UartPacket pkt = {0};
    pkt.start        = PACKET_START_CODE;
    pkt.data_vec_u8  = vec_u8_new();
    pkt.end          = PACKET_END_CODE;
    pkt.add_data     = pkt_add_data;
    pkt.get_data     = pkt_get_data;
    pkt.pack         = pkt_pack;
    pkt.unpack       = pkt_unpack;
    return pkt;
}

// ----------------------------------------------------------------------------------------------------

/**
 * @brief 將封包推入環形緩衝區，若已滿則返回 false
 *        Push a packet into the ring buffer; return false if buffer is full
 *
 * @param self 指向環形緩衝區的指標 (input/output ring buffer)
 * @param pkt 要推入緩衝區的 UART 封包 (input UART packet)
 * @return bool 是否推入成功 (true if push successful, false if buffer full)
 */
static bool trcv_buffer_push(UartTrcvBuf *self, const UartPacket *pkt) {
    if (self->length >= UART_TRCV_BUF_CAP) return false;
    uint8_t tail = (self->head + self->length) % UART_TRCV_BUF_CAP;
    self->packet[tail] = *pkt;
    self->length++;
    return true;
}

static bool trcv_buffer_get_front(const UartTrcvBuf *self, UartPacket *pkt) {
    if (self->length == 0) return 0;
    if (self != NULL) *pkt = self->packet[self->head];
    return 1;
}

/**
 * @brief 從環形緩衝區彈出一個封包資料
 *        Pop a packet from the ring buffer
 *
 * @param self 指向環形緩衝區的指標 (input/output ring buffer)
 * @param pkt 輸出參數，接收彈出的 UART 封包 (output popped UART packet)
 * @return bool 是否彈出成功 (true if pop successful, false if buffer empty)
 */
static bool trcv_buffer_pop(UartTrcvBuf *self, UartPacket *pkt) {
    if (self->length == 0) return 0;
    if (self != NULL) *pkt = self->packet[self->head];
    if (--self->length == 0) {
        self->head = 0;
    } else {
        self->head = (self->head + 1) % UART_TRCV_BUF_CAP;
    }
    return 1;
}

/**
 * @brief 建立傳輸/接收環形緩衝區，初始化頭指標與計數
 *        Create a transmit/receive ring buffer, initialize head index and length
 *
 * @return UartTrcvBuf 初始化後的環形緩衝區 (initialized ring buffer)
 */
UartTrcvBuf uart_trcv_buf_new(void) {
    UartTrcvBuf buf = {0};
    buf.head    = 0;
    buf.length  = 0;
    buf.push        = trcv_buffer_push;
    buf.get_front   = trcv_buffer_get_front;
    buf.pop         = trcv_buffer_pop;
    return buf;
}

/**
 * @brief 全域傳輸緩衝區
 *        Global transmit ring buffer
 */
UartTrcvBuf uart_trsm_buf = {0};

/**
 * @brief 全域接收緩衝區
 *        Global receive ring buffer
 */
UartTrcvBuf uart_recv_buf = {0};

void uart_trcv_buf_init(void) {
    uart_trsm_buf   = uart_trcv_buf_new();
    uart_recv_buf   = uart_trcv_buf_new();
}
