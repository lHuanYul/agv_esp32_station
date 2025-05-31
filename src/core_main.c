// #include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "freertos/event_groups.h"
// #include "esp_system.h"
// #include "esp_mac.h"
// #include "esp_wifi.h"
// #include "esp_event.h"
#include "esp_log.h"
// #include "nvs_flash.h"
// #include "esp_netif.h"
// #include "lwip/err.h"
// #include "lwip/sys.h"
// #include "lwip/sockets.h"
// #include "lwip/netdb.h"
#include "wifi/connect.h"
#include "wifi/tcp_transceive.h"
#include "uart/transceive.h"
#include "uart/packet.h"

static const char *TAG = "core main";

void core_main(void) {
    wifi_connect_setup();
    wifi_transceive_setup();
    uart_setup();

    VecU8 vec_u8 = vec_u8_new();
    vec_u8.push(&vec_u8, "testing", 7);
    UartPacket uart_packet = uart_packet_new();
    uart_packet.add_data(&uart_packet, &vec_u8);
    while (1) {
        // ESP_LOGI(TAG, "Running main loop...");
        // uart_trcv_buffer_push(&uart_trsm_buf, &uart_packet);
        // wifi_udp_write_task();
        // wifi_tcp_write_task();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
