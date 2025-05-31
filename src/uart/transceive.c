#include "uart/transceive.h"
#include "uart/packet.h"
#include "prioritites_sequ.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

static const int RX_BUF_SIZE = VECU8_MAX_CAPACITY;

#define TXD_PIN (GPIO_NUM_4)
#define RXD_PIN (GPIO_NUM_5)

#define UART_READ_TIMEOUT_MS 10

/**
 * @brief 傳輸/接收操作旗標
 *        Transmit/receive operation flags
 *
 * @details 控制資料處理流程 (Control data processing flow)
 */
TransceiveFlags transceive_flags = {0};

static void uart_tasks_spawn(void);
void uart_setup(void) {
    uart_trcv_buf_init();
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_tasks_spawn();
}

bool uart_write_t(const char* logName, UartPacket *packet) {
    VecU8 vec_u8 = packet->unpack(packet);
    int len = uart_write_bytes(UART_NUM_1, vec_u8.data, vec_u8.len);
    if (len <= 0) {
        return 0;
    }
    ESP_LOGI(logName, "Wrote %d bytes", len);
    return 1;
}

static void uart_write_task(void *arg) {
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        UartPacket packet = uart_packet_new();
        if (!uart_trsm_buf.get_front(&uart_trsm_buf, &packet)) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (!uart_write_t(TX_TASK_TAG, &packet)) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        uart_trsm_buf.pop(&uart_trsm_buf, NULL);
    }
    vTaskDelete(NULL);
}

static bool uart_read_t(const char* logName, UartPacket *packet) {
    uint8_t data[VECU8_MAX_CAPACITY] = {0};
    int len = uart_read_bytes(UART_NUM_1, data, VECU8_MAX_CAPACITY, pdMS_TO_TICKS(UART_READ_TIMEOUT_MS));
    if (len <= 0) {
        return 0;
    }
    ESP_LOGI(logName, "Read %d bytes: '%s'", len, data);
    ESP_LOG_BUFFER_HEXDUMP(logName, data, len, ESP_LOG_INFO);
    VecU8 vec_u8 = vec_u8_new();
    vec_u8.push(&vec_u8, &data, len);
    UartPacket new = uart_packet_new();
    new.pack(&new, &vec_u8);
    *packet = new;
    return 1;
}

static void uart_read_task(void *arg) {
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        UartPacket packet = uart_packet_new();
        if (!uart_read_t(RX_TASK_TAG, &packet)) {
            continue;
        }
        uart_recv_buf.push(&uart_recv_buf, &packet);
    }
    vTaskDelete(NULL);
}

static void uart_tasks_spawn(void) {
    xTaskCreate(uart_read_task, "uart_rx_task", 4096, NULL, UART_READ_TASK_PRIO_SEQU, NULL);
    xTaskCreate(uart_write_task, "uart_tx_task", 4096, NULL, UART_WRITE_TASK_PRIO_SEQU, NULL);
}
