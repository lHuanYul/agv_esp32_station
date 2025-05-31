#ifndef USER_PACKET_H
#define USER_PACKET_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "vec_mod.h"

#define PACKET_START_CODE  ((uint8_t) '{')
#define PACKET_END_CODE    ((uint8_t) '}')

#define PACKET_MAX_SIZE VECU8_MAX_CAPACITY
#define PACKET_DATA_MAX_SIZE (VECU8_MAX_CAPACITY - 2)

// ----------------------------------------------------------------------------------------------------

typedef struct UartPacket UartPacket;
typedef VecU8 (*PktGetDataFn)   (const UartPacket *pkt);
typedef bool (*PktPackFn)       (      UartPacket *pkt, const VecU8 *vec_u8);
typedef void (*PktAddDataFn)    (      UartPacket *pkt, const VecU8 *vec_u8);
typedef VecU8 (*PktUnpackFn)    (const UartPacket *pkt);
typedef struct UartPacket {
    uint8_t     start;
    VecU8       data_vec_u8;
    uint8_t     end;
    /**
     * @brief 向現有 UART 封包中新增資料
     *        Add data to existing UART packet
     *
     * @param pkt 指向呼叫者自己 (input packet)
     * @param vec_u8 要新增的資料向量 (input data vector)
     */
    PktAddDataFn    add_data;
    /**
     * @brief 根據原始資料向量打包成 UART 封包，並移除起始與結束碼後重新封裝
     *        Pack raw data vector into UART packet, stripping start and end codes before repacking
     *
     * @param pkt 輸出參數，接收封裝後的 UART 封包 (output packed UART packet)
     * @return bool 是否封包成功 (true if pack successful, false otherwise)
     */
    PktGetDataFn    get_data;
    /**
     * @brief 根據原始資料向量打包成 UART 封包，並移除起始與結束碼後重新封裝
     *        Pack raw data vector into UART packet, stripping start and end codes before repacking
     *
     * @param pkt 輸出參數，接收封裝後的 UART 封包 (output packed UART packet)
     * @param vec_u8 包含封包起始碼與結束碼的資料向量 (input byte vector with start/end codes)
     * @return bool 是否封包成功 (true if pack successful, false otherwise)
     */
    PktPackFn       pack;
    /**
     * @brief 解包 UART 封包，將起始碼、資料與結束碼合併為一個資料向量
     *        Unpack UART packet into a byte vector including start, data, and end codes
     *
     * @param pkt 指向要解包的 UART 封包 (input packet)
     * @return VecU8 包含完整封包的資料向量 (vector containing full packet bytes)
     */
    PktUnpackFn     unpack;
} UartPacket;

UartPacket uart_packet_new(void);

// ----------------------------------------------------------------------------------------------------

#define UART_TRCV_BUF_CAP 5
typedef struct UartTrcvBuf UartTrcvBuf;
typedef bool (*BufPushFn)       (      UartTrcvBuf *buf, const UartPacket *pkt);
typedef bool (*BufGetFrontFn)   (const UartTrcvBuf *buf,       UartPacket *pkt);
typedef bool (*BufPopFn)        (      UartTrcvBuf *buf,       UartPacket *pkt);
typedef struct UartTrcvBuf {
    UartPacket  packet[UART_TRCV_BUF_CAP];
    uint8_t     head;
    uint8_t     length;
    /**
     * @brief 將封包推入環形緩衝區，若已滿則返回 false
     *        Push a packet into the ring buffer; return false if buffer is full
     *
     * @param buf 指向環形緩衝區的指標 (input/output ring buffer)
     * @param pkt 要推入緩衝區的 UART 封包 (input UART packet)
     * @return bool 是否推入成功 (true if push successful, false if buffer full)
     */
    BufPushFn     push;
    BufGetFrontFn get_front;
    /**
     * @brief 從環形緩衝區彈出一個封包資料
     *        Pop a packet from the ring buffer
     *
     * @param buf 指向環形緩衝區的指標 (input/output ring buffer)
     * @param pkt 輸出參數，接收彈出的 UART 封包 (output popped UART packet)
     * @return bool 是否彈出成功 (true if pop successful, false if buffer empty)
     */
    BufPopFn      pop;
} UartTrcvBuf;
extern UartTrcvBuf uart_trsm_buf;
extern UartTrcvBuf uart_recv_buf;
UartTrcvBuf uart_trcv_buf_new(void);
void uart_trcv_buf_init(void);

#endif
