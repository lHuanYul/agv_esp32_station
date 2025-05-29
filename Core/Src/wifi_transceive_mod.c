#include "wifi_transceive_mod.h"
#include "wifi_packet_proc_mod.h"
#include "prioritites_sequ.h"
#include "mcu_const.h"
#include <errno.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#define TARGET_IP   "192.168.0.11"
#define TCP_PORT    60000
#define UDP_PORT    60001

static const char *TAG = "wifi trcv";

// ip4_addr_t ip;
// IP4_ADDR(&ip, 0, 0, 0, 0);

static bool wifi_tcp_read(WifiPacket *packet, int sock) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sock = accept(sock, (struct sockaddr *)&client_addr, &client_len);
    if (client_sock < 0) {
        ESP_LOGE(TAG, "TCP accept() failed: errno %d", errno);
        return 0;
    }
    ESP_LOGI(TAG, "TCP connection from %s:%d",
                inet_ntoa(client_addr.sin_addr),
                ntohs(client_addr.sin_port));
    VecU8 vec_u8 = vec_u8_new();
    uint8_t rx_buffer[VECU8_MAX_CAPACITY];
    int len;
    // 3. 持續 recv 數據，直到對方關閉
    while ((len = recv(client_sock, rx_buffer, sizeof(rx_buffer), 0)) > 0) {
        ESP_LOGI(TAG, "TCP Rx %d bytes → %s", len, (char *)rx_buffer);
        vec_u8_push(&vec_u8, rx_buffer, len);
    }
    close(client_sock);
    ESP_LOGI(TAG, "TCP client disconnected");
    if (len < 0) {
        ESP_LOGE(TAG, "TCP recv() failed: errno %d", errno);
        return 0;
    }
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

    // 2. 接受並處理每個 client
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

static bool wifi_udp_read(WifiPacket *packet, int sock) {
    uint8_t rx_buffer[VECU8_MAX_CAPACITY];
    struct sockaddr_in client_addr;
    socklen_t socklen = sizeof(client_addr);
    int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr*)&client_addr, &socklen);
    if (len <= 0) {
        return 0;
    }
    ip4_addr_t ip;
    ip.addr = client_addr.sin_addr.s_addr;
    VecU8 vec_u8 = vec_u8_new();
    vec_u8_push(&vec_u8, rx_buffer, len);
    *packet = wifi_packet_new(&ip, &vec_u8);
    ESP_LOGI(TAG, "Received %d bytes from %s:%d → %.*s",
        len,
        inet_ntoa(client_addr.sin_addr),
        ntohs(client_addr.sin_port),
        len,
        rx_buffer);
    return 1;
}

/**
 * @brief UDP 伺服器任務
 *
 * 阻塞接收 UDP 封包並印出來源 IP:Port 與內容
 *
 * @param pvParameters 任務參數 (未使用)
 * @return 不會返回
 *
 * UDP server task: blocks to receive packets and logs source IP:port and data
 */
static void wifi_udp_read_task(void *pvParameters) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", sock);
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in addr = {
        .sin_family         = AF_INET,
        .sin_port           = htons(UDP_PORT),
        .sin_addr.s_addr    = htonl(INADDR_ANY),
    };

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "Socket bind failed: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Socket bound, listening on port %d", UDP_PORT);

    while (1) {
        WifiPacket packet;
        if (!wifi_udp_read(&packet, sock)) {
            continue;
        }
        wifi_trcv_buffer_push(&wifi_udp_receive_buffer, &packet);
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
    int ret = send(sock, vec_u8->data, vec_u8->length, 0);
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
    vec_u8_push_const(&vec_u8, CMD_RIGHT_ADC_STORE);
    int sent = wifi_tcp_write(TARGET_IP, TCP_PORT, &vec_u8);
    if (sent < 0) {
        ESP_LOGE(TAG, "TCP send failed: %d", sent);
    }
    ESP_LOGI(TAG, "TCP send success");
}

/**
 * @brief UDP 發送函式
 *
 * 將 data 經 UDP 發送到 remote_ip:remote_port，
 * 並回傳實際送出的 byte 數或負值 errno。
 *
 * Send data via UDP to remote_ip:remote_port;
 * returns number of bytes sent or negative errno on error.
 */
static int wifi_udp_write(const char *remote_ip, const uint16_t remote_port, const VecU8 *vec_u8) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return -errno;
    }
    struct sockaddr_in addr = {
        .sin_family         = AF_INET,
        .sin_port           = htons(remote_port),
        .sin_addr.s_addr    = inet_addr(remote_ip),
    };

    // ESP_LOG_BUFFER_HEXDUMP(TAG, vec_u8->data, vec_u8->length, ESP_LOG_INFO);
    int ret = sendto(
        sock, vec_u8->data, vec_u8->length, 0,
        (struct sockaddr *)&addr,
        sizeof(addr)
    );
    if (ret < 0) {
        ESP_LOGE(TAG, "sendto() failed: errno %d", errno);
        close(sock);
        return -errno;
    }
    // ESP_LOGI(TAG, "Sent %d bytes to %s:%d", ret, remote_ip, remote_port);
    close(sock);
    return ret;
}

float f32_test = 1;
uint16_t u16_test = 1;
void wifi_udp_write_task(void) {
    VecU8 vec_u8 = vec_u8_new();
    vec_u8_push_const(&vec_u8, CMD_RIGHT_ADC_STORE);
    vec_u8_push_u16(&vec_u8, u16_test);
    u16_test++;

    int sent = wifi_udp_write(TARGET_IP, UDP_PORT, &vec_u8);
    if (sent < 0) {
        ESP_LOGE(TAG, "UDP send failed: %d", sent);
    }
}

void wifi_transceive_setup(void) {
    xTaskCreate(wifi_udp_read_task, "udp_server", 4096, NULL, WIFI_UDP_READ_TASK_PRIO_SEQU, NULL);
    xTaskCreate(wifi_tcp_read_task, "tcp_recv", 4096, NULL, WIFI_TCP_READ_TASK_PRIO_SEQU, NULL);
    // BaseType_t ret = 
    // if (ret != pdPASS) {
    //     ESP_LOGE(TAG, "xTaskCreate(wifi_tcp_read_task) failed");
    // }
}
