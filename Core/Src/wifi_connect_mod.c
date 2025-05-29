#include <arpa/inet.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include "wifi_connect_mod.h"

#define CONNECT_MAXIMUM_RETRY   5
#define WIFI_CONNECTED_BIT      BIT0
#define WIFI_FAIL_BIT           BIT1

static const char *TAG = "wifi connect";

char connect_wifi_ssid[] = "HY-TPL-BF94";
char connect_wifi_pswd[] = "23603356";
char connect_wifi_DHCP[] = "192.168.0.20";

static EventGroupHandle_t s_wifi_event_group;
static int wifi_connect_retry_count = 0;

/**
 * @brief Wi-Fi 事件處理函式
 *
 * 處理 Wi-Fi 相關事件，如啟動後連線、斷線重連與取得 IP
 *
 * @param arg        使用者參數 (未使用)
 * @param event_base 事件來源
 * @param event_id   事件 ID
 * @param event_data 事件資料
 *
 * Handle Wi-Fi events: start connect, retry on disconnect, and got IP
 */
static void wifi_connect_event_handler(
    void* arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data
) {
    // Called when Wi-Fi starts, initiate connection
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    // On disconnection, retry or signal failure
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (wifi_connect_retry_count < CONNECT_MAXIMUM_RETRY) {
            esp_wifi_connect();
            wifi_connect_retry_count++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            // Exceeded max retries, set failure bit
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    }
    // Obtained IP, reset retry count and set success bit
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connect_retry_count = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * @brief 初始化並啟動 Wi-Fi STA 模式
 *
 * 設定 SSID、密碼，啟動 Wi-Fi 並等待連接結果
 *
 * Initialize Wi-Fi in Station mode, configure SSID/password and wait for connection
 */
void wifi_init_sta(void) {
    // Create wifi event group
    s_wifi_event_group = xEventGroupCreate();
    // Initialize NVS for storing Wi-Fi parameters
    ESP_ERROR_CHECK(nvs_flash_init());
    // Initialize TCP/IP network interface
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default Wi-Fi station
    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(sta_netif));
    esp_netif_ip_info_t ip_info;
    // 預設閘道
    inet_pton(AF_INET, "192.168.0.1", &ip_info.gw);
    // 本機 IP
    inet_pton(AF_INET, connect_wifi_DHCP, &ip_info.ip);
    // 子網遮罩
    inet_pton(AF_INET, "255.255.255.0", &ip_info.netmask);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(sta_netif, &ip_info));


    // Initialize Wi-Fi driver
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Register Wi-Fi and IP event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_connect_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_connect_event_handler, NULL, NULL));
    
    // Configure Wi-Fi connection parameters
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    strcpy((char *)wifi_config.sta.ssid,     connect_wifi_ssid);
    strcpy((char *)wifi_config.sta.password, connect_wifi_pswd);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    // Set Wi-Fi mode to Station
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    // Set Station configuration
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    // Start Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_start());

    // Wait for connection result: success or fail
    ESP_LOGI(TAG, "Connecting to SSID:%s...", connect_wifi_ssid);
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE, pdFALSE, portMAX_DELAY);
    
    // Connected successfully
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 connect_wifi_ssid, connect_wifi_pswd);
    }
    // Failed to connect
    else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "failed to connect to SSID:%s, password:%s",
                 connect_wifi_ssid, connect_wifi_pswd);
    }

    // Delete event group
    vEventGroupDelete(s_wifi_event_group);
}

void wifi_connect_setup(void) {
    wifi_init_sta();
}

