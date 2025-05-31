#include "wifi/tcp_transceive.h"
#include "prioritites_sequ.h"
#include "mcu_const.h"
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "http_parser/http_parser.h"

#define TARGET_IP   "192.168.0.11"
#define TCP_PORT    60000

static const char *TAG = "wifi_tcp_trcv";

// ip4_addr_t ip;
// IP4_ADDR(&ip, 0, 0, 0, 0);

static void wifi_tasks_spawn(void);
void wifi_transceive_setup(void) {
    wifi_tasks_spawn();
}

#define BUFFER_SIZE 1024
#ifndef MIN
  #define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif
static bool wifi_tcp_read(WifiPacket *packet, int sock) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sock = accept(sock, (struct sockaddr *)&client_addr, &client_len);
    if (client_sock < 0) {
        ESP_LOGE(TAG, "TCP accept() failed: errno %d", errno);
        return 0;
    }
    ESP_LOGI(TAG, "TCP connection from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    char header_buf[BUFFER_SIZE];
    int total_header_len = 0;
    int header_end_index = -1;
    VecU8 vec_u8 = vec_u8_new();
    // uint8_t rx_buffer[1024];
    // int len;
    while (1) {
        // if ((len = recv(client_sock, rx_buffer, sizeof(rx_buffer), 0)) <= 0) break;
        // ESP_LOGI(TAG, "TCP Rx %d bytes → \n%s", len, (char *)rx_buffer);
        // vec_u8.push(&vec_u8, rx_buffer, len);
        int len = recv(client_sock,
                       header_buf + total_header_len,
                       BUFFER_SIZE - total_header_len,
                       0);
        if (len <= 0) {
            // 連線被關、或 recv 失敗
            close(client_sock);
            ESP_LOGE(TAG, "recv error or connection closed prematurely");
            return false;
        }
        total_header_len += len;
        // 在目前的 header_buf 裡找 "\r\n\r\n"
        char *pos = strstr(header_buf, "\r\n\r\n");
        if (pos != NULL) {
            header_end_index = (int)(pos - header_buf) + 4; // include "\r\n\r\n"
            break;
        }
    }

    int content_len = 0;
    {
        // 暫時把 header_buf 當成字串處理
        header_buf[header_end_index] = '\0';
        char *cl_key = "Content-Length:";
        char *cl_pos = strcasestr(header_buf, cl_key);
        if (cl_pos) {
            // 往後跳過 "Content-Length:"
            cl_pos += strlen(cl_key);
            // 跳過空白
            while (*cl_pos == ' ' || *cl_pos == '\t') {
                cl_pos++;
            }
            content_len = atoi(cl_pos);
        } else {
            content_len = 0;
        }
    }
    ESP_LOGI(TAG, "Parsed Content-Length = %d", content_len);
    vec_u8.push(&vec_u8, (uint8_t *)header_buf, header_end_index);

    int already_body = total_header_len - header_end_index;
    if (already_body > 0) {
        vec_u8.push(&vec_u8,
                    (uint8_t *)(header_buf + header_end_index),
                    already_body);
    }

    int remaining = content_len - already_body;
    while (remaining > 0) {
        uint8_t tmp_buf[BUFFER_SIZE];
        int len2 = recv(client_sock, tmp_buf, MIN(remaining, BUFFER_SIZE), 0);
        if (len2 <= 0) {
            ESP_LOGE(TAG, "recv body failed: errno %d", errno);
            close(client_sock);
            return false;
        }
        vec_u8.push(&vec_u8, tmp_buf, len2);
        remaining -= len2;
    }

    close(client_sock);
    ESP_LOGI(TAG, "TCP client disconnected");
    ESP_LOGI(TAG, "TCP client read complete, total body bytes = %d get\n%s", content_len, vec_u8.data);
    // if (len < 0) {
    //     ESP_LOGE(TAG, "TCP recv() failed: \n%s", vec_u8.data);
    //     ESP_LOGE(TAG, "TCP recv() failed: errno %d", errno);
    //     return 0;
    // }
    ip4_addr_t ip;
    ip.addr = client_addr.sin_addr.s_addr;
    *packet = wifi_packet_new(&ip, &vec_u8);
    return 1;
}

/**
 * @brief 啟動 TCP 接收任務
 *
 * 在背景啟動一個 FreeRTOS Task 持續監聽 TCP_PORT，
 * 收到封包後用 ESP_LOGI 列印來源與資料。
 *
 * Start a background FreeRTOS task listening on TCP_PORT;
 * upon packet arrival it logs peer and payload.
 */
static void wifi_tcp_read_task(void *pvParameters) {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "TCP socket() failed: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    struct sockaddr_in addr = {
        .sin_family         = AF_INET,
        .sin_port           = htons(TCP_PORT),
        .sin_addr.s_addr    = htonl(INADDR_ANY),
    };
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "TCP bind failed: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }
    if (listen(sock, 1) < 0) {
        ESP_LOGE(TAG, "TCP listen() failed: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "TCP listening on port %d", TCP_PORT);
    while(1) {
        WifiPacket packet;
        if (!wifi_tcp_read(&packet, sock)) {
            continue;
        }
        wifi_trcv_buffer_push(&wifi_tcp_receive_buffer, &packet);
    }
    close(sock);
    vTaskDelete(NULL);
}

/**
 * @brief TCP 發送函式
 *
 * 將 data 經 TCP 發送到 remote_ip:remote_port，
 * 並回傳實際送出的 byte 數或負值 errno。
 *
 * Send data via TCP to remote_ip:remote_port;
 * returns number of bytes sent or negative errno on error.
 */
static int wifi_tcp_write(const char *remote_ip, const uint16_t remote_port, const VecU8 *vec_u8) {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "TCP socket() failed: errno %d", errno);
        return -errno;
    }

    struct sockaddr_in addr = {
        .sin_family         = AF_INET,
        .sin_port           = htons(remote_port),
        .sin_addr.s_addr    = inet_addr(remote_ip),
    };

    // 建立 TCP 連線（三次握手）
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "TCP connect() failed: errno %d", errno);
        close(sock);
        return -errno;
    }

    // 發送資料
    int ret = send(sock, vec_u8->data, vec_u8->len, 0);
    if (ret < 0) {
        ESP_LOGE(TAG, "TCP send() failed: errno %d", errno);
        close(sock);
        return -errno;
    }

    ESP_LOGI(TAG, "TCP sent %d bytes to %s:%d", ret, remote_ip, remote_port);
    close(sock);
    return ret;
}

void wifi_tcp_write_task(void) {
    VecU8 vec_u8 = vec_u8_new();
    vec_u8.push(&vec_u8, CMD_RIGHT_ADC_STORE, sizeof(CMD_RIGHT_ADC_STORE));
    int sent = wifi_tcp_write(TARGET_IP, TCP_PORT, &vec_u8);
    if (sent < 0) {
        ESP_LOGE(TAG, "TCP send failed: %d", sent);
    }
    ESP_LOGI(TAG, "TCP send success");
}

static void wifi_tasks_spawn(void) {
    // xTaskCreate(wifi_udp_read_task, "udp_server", 4096, NULL, WIFI_UDP_READ_TASK_PRIO_SEQU, NULL);
    xTaskCreate(wifi_tcp_read_task, "tcp_recv", 8192, NULL, WIFI_TCP_READ_TASK_PRIO_SEQU, NULL);
    // BaseType_t ret = 
    // if (ret != pdPASS) {
    //     ESP_LOGE(TAG, "xTaskCreate(wifi_tcp_read_task) failed");
    // }
}
