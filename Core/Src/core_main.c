#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "wifi_connect.h"
#include "wifi_transceive.h"
#include "uart_async.h"

static const char *TAG = "core main";

void core_main(void)
{
    wifi_connect_setup();
    wifi_transceive_setup();
    uart_main();

    while (1) {
        ESP_LOGI(TAG, "Running main loop...");
        wifi_udp_transmit();
        // wifi_tcp_transmit();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
