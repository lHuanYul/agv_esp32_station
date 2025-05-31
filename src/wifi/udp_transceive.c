#include "wifi/udp_transceive.h"
#include "prioritites_sequ.h"
#include "mcu_const.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#define TARGET_IP   "192.168.0.11"
#define UDP_PORT    60001

static const char *TAG = "wifi_udp_trcv";

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
    vec_u8.push(&vec_u8, rx_buffer, len);
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
        sock, vec_u8->data, vec_u8->len, 0,
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
    vec_u8.push(&vec_u8, CMD_RIGHT_ADC_STORE, sizeof(CMD_RIGHT_ADC_STORE));
    vec_u8.push_u16(&vec_u8, u16_test);
    u16_test++;

    int sent = wifi_udp_write(TARGET_IP, UDP_PORT, &vec_u8);
    if (sent < 0) {
        ESP_LOGE(TAG, "UDP send failed: %d", sent);
    }
}
