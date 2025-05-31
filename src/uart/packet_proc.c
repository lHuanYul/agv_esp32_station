#include "uart/packet_proc.h"
#include "uart/transceive.h"
#include "mcu_const.h"

float f32_test = 1;
uint16_t u16_test = 1;

/**
 * @brief 組合並傳輸封包至傳輸緩衝區
 *        Assemble and transmit packet into transfer buffer
 *
 * @note 根據 transceive_flags 決定回應內容
 *
 * @return void
 */
void uart_transmit_pkt_proc(void) {
    VecU8 new_vec = vec_u8_new();
    new_vec.push(&new_vec, &(uint8_t){0x10}, 1);
    bool new_vec_wri_flag = false;
    if (new_vec_wri_flag) {
        UartPacket new_packet = uart_packet_new();
        new_packet.add_data(&new_packet, &new_vec);
        uart_trsm_buf.push(&uart_trsm_buf, &new_packet);
    };
}

void uart_re_pkt_proc_data_store(VecU8 *vec_u8);

/**
 * @brief 從接收緩衝區反覆讀取封包並處理
 *        Pop packets from receive buffer and process them
 *
 * @param count 單次最大處理封包數量 (input maximum number of packets to process per time)
 * @return void
 */
void uart_receive_pkt_proc(uint8_t count) {
    uint8_t i;
    for (i = 0; i < 5; i++){
        UartPacket packet = uart_packet_new();
        if (!uart_recv_buf.pop(&uart_recv_buf, &packet)) {
            break;
        }
        VecU8 re_vec_u8 = packet.get_data(&packet);
        uint8_t code = re_vec_u8.data[0];
        re_vec_u8.rm_front(&re_vec_u8, 1);
        switch (code) {
            case CMD_CODE_DATA_TRRE:
                uart_re_pkt_proc_data_store(&re_vec_u8);
                break;
            default:
                break;
        }
    }
}

/**
 * @brief 處理接收命令並存儲/回應資料
 *        Process received commands and store or respond data
 *
 * @param vec_u8 指向去除命令碼後的資料向量 (input vector without command code)
 * @return void
 */
void uart_re_pkt_proc_data_store(VecU8 *vec_u8) {
    VecU8 new_vec = vec_u8_new();
    new_vec.push(&new_vec, &(uint8_t){0x10}, 1);
    bool data_proc_flag;
    bool new_vec_wri_flag = false;
    while (1) {
        data_proc_flag = false;
        if (vec_u8->start_with(vec_u8, CMD_RIGHT_SPEED_STOP, sizeof(CMD_RIGHT_SPEED_STOP))) {
            vec_u8->rm_front(vec_u8, sizeof(CMD_RIGHT_SPEED_STOP));
            data_proc_flag = true;
            transceive_flags.right_speed = false;
        }
        if (vec_u8->start_with(vec_u8, CMD_RIGHT_SPEED_ONCE, sizeof(CMD_RIGHT_SPEED_ONCE))) {
            vec_u8->rm_front(vec_u8, sizeof(CMD_RIGHT_SPEED_ONCE));
            data_proc_flag = true;
            new_vec_wri_flag = true;
        }
        if (vec_u8->start_with(vec_u8, CMD_RIGHT_SPEED_START, sizeof(CMD_RIGHT_SPEED_START))) {
            vec_u8->rm_front(vec_u8, sizeof(CMD_RIGHT_SPEED_START));
            data_proc_flag = true;
            transceive_flags.right_speed = true;
        }
        if (vec_u8->start_with(vec_u8, CMD_RIGHT_ADC_STOP, sizeof(CMD_RIGHT_ADC_STOP))) {
            vec_u8->rm_front(vec_u8, sizeof(CMD_RIGHT_ADC_STOP));
            data_proc_flag = true;
            transceive_flags.right_adc = false;
        }
        if (vec_u8->start_with(vec_u8, CMD_RIGHT_ADC_ONCE, sizeof(CMD_RIGHT_ADC_ONCE))) {
            vec_u8->rm_front(vec_u8, sizeof(CMD_RIGHT_ADC_ONCE));
            data_proc_flag = true;
            new_vec_wri_flag = true;
        }
        if (vec_u8->start_with(vec_u8, CMD_RIGHT_ADC_START, sizeof(CMD_RIGHT_ADC_START))) {
            vec_u8->rm_front(vec_u8, sizeof(CMD_RIGHT_ADC_START));
            data_proc_flag = true;
            transceive_flags.right_adc = true;
        }
        if (!data_proc_flag) break;
    }
    if (new_vec_wri_flag) {
        UartPacket new_packet = uart_packet_new();
        new_packet.add_data(&new_packet, &new_vec);
        uart_trsm_buf.push(&uart_trsm_buf, &new_packet);
    }
}

/**
 * @brief 填充狀態至資料向量
 *        Populate status into byte vector
 *
 * @param vec_u8 指向要寫入資料的 VecU8 (input/output vector to receive motor data)
 * @return void
 */
void transmit_buf_set(VecU8* vec_u8) {
}
