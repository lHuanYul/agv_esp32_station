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

typedef struct {
    char *method_str;       // HTTP method (e.g., "GET", "POST")
    char *url;              // Request URI (e.g., "/hello")
    // 簡單把 header 存成陣列，實務上可能希望用 map 或 Hash table
    struct {
        char *field;        // header name
        char *value;        // header value
    } headers[16];          // 假設最多一筆 request 不超過 16 個 header
    size_t num_headers;
    char *body;             // Body (NULL if no body)
    size_t body_len;
} http_request_t;
/* 為了在 callback 裡存取，先宣告全域（或 static）http_request_t 及暫存變數  */
static http_request_t current_req;
/* 暫存上一次解析到的 header field/value 片段 */
static char *tmp_field    = NULL;
static size_t tmp_f_len   = 0;
static char *tmp_value    = NULL;
static size_t tmp_v_len   = 0;

// helper：把完整的一對 Header field+value 加入 current_req.headers[]
static void add_header_entry() {
    if (current_req.num_headers < 16) {
        current_req.headers[current_req.num_headers].field = tmp_field;
        current_req.headers[current_req.num_headers].value = tmp_value;
        current_req.num_headers++;
    } else {
        // 若超過上限，可選擇忽略或 free tmp_field/tmp_value
        free(tmp_field);
        free(tmp_value);
    }
    tmp_field = NULL; tmp_f_len = 0;
    tmp_value = NULL; tmp_v_len = 0;
}

// ---------- 2. callback 函式實作 ----------
//   回呼幾乎都會傳來「片段」（slice），必須累積成完整字串才能使用

// on_message_begin：每次收到新 request 時，會在最一開始呼叫
static int on_message_begin_cb(http_parser *parser) {
    // 先清空 current_req 裡面之前殘留的資料
    if (current_req.url) { free(current_req.url); }
    if (current_req.method_str) { free(current_req.method_str); }
    for (size_t i = 0; i < current_req.num_headers; i++) {
        free(current_req.headers[i].field);
        free(current_req.headers[i].value);
    }
    if (current_req.body) { free(current_req.body); }

    memset(&current_req, 0, sizeof(current_req));
    tmp_field = NULL; tmp_f_len = 0;
    tmp_value = NULL; tmp_v_len = 0;
    return 0;  // 回傳 0 表示繼續解析
}

// on_url：解析到 URL 時被呼叫 (只對 HTTP_REQUEST 有意義)
static int on_url_cb(http_parser *parser, const char *at, size_t length) {
    if (current_req.url == NULL) {
        current_req.url = (char *)malloc(length + 1);
        memcpy(current_req.url, at, length);
        current_req.url[length] = '\0';
    } else {
        size_t old_len = strlen(current_req.url);
        current_req.url = (char *)realloc(current_req.url, old_len + length + 1);
        memcpy(current_req.url + old_len, at, length);
        current_req.url[old_len + length] = '\0';
    }
    return 0;
}

// on_header_field：解析到 Header 名稱時被呼叫
static int on_header_field_cb(http_parser *parser, const char *at, size_t length) {
    // 若上一次 tmp_value_len > 0，表示上一個 header field+value 已經完整，可以先存下
    if (tmp_v_len > 0) {
        add_header_entry();
    }
    // 開始累積新的 field 片段
    if (tmp_field == NULL) {
        tmp_field = (char *)malloc(length + 1);
        memcpy(tmp_field, at, length);
        tmp_field[length] = '\0';
        tmp_f_len = length;
    } else {
        tmp_field = (char *)realloc(tmp_field, tmp_f_len + length + 1);
        memcpy(tmp_field + tmp_f_len, at, length);
        tmp_f_len += length;
        tmp_field[tmp_f_len] = '\0';
    }
    return 0;
}

// on_header_value：解析到 Header 值時被呼叫
static int on_header_value_cb(http_parser *parser, const char *at, size_t length) {
    if (tmp_value == NULL) {
        tmp_value = (char *)malloc(length + 1);
        memcpy(tmp_value, at, length);
        tmp_value[length] = '\0';
        tmp_v_len = length;
    } else {
        tmp_value = (char *)realloc(tmp_value, tmp_v_len + length + 1);
        memcpy(tmp_value + tmp_v_len, at, length);
        tmp_v_len += length;
        tmp_value[tmp_v_len] = '\0';
    }
    return 0;
}

// on_headers_complete：所有 header 解析完成時呼叫
static int on_headers_complete_cb(http_parser *parser) {
    // 如果最後一組 header field+value 還沒加入，就在此補上
    if (tmp_f_len > 0 && tmp_v_len > 0) {
        add_header_entry();
    }
    // 把 Method 存起來
    const char *m = http_method_str(parser->method);
    current_req.method_str = strdup(m);
    // 若有 Content-Length，就先 allocate 一塊 body buffer
    if (parser->content_length > 0) {
        current_req.body = (char *)malloc(parser->content_length + 1);
        current_req.body_len = 0;
    }
    return 0;
}

// on_body：讀到 body 片段時呼叫，多次呼叫直到 body 讀完
static int on_body_cb(http_parser *parser, const char *at, size_t length) {
    ESP_LOGI(TAG, "on_body_cb called, segment length = %d", (int)length);
    if (current_req.body) {
        memcpy(current_req.body + current_req.body_len, at, length);
        current_req.body_len += length;
        current_req.body[current_req.body_len] = '\0';
    }
    return 0;
}

// on_message_complete：整筆 request（header + body）都讀完時呼叫
static int on_message_complete_cb(http_parser *parser) {
    // 到這裡代表 current_req 已經有完備的 method, url, headers[], body
    ESP_LOGI(TAG, ">>> HTTP 解析完成 <<<");
    ESP_LOGI(TAG, "Method: %s", current_req.method_str ? current_req.method_str : "(none)");
    ESP_LOGI(TAG, "URL   : %s", current_req.url ? current_req.url : "(none)");
    for (size_t i = 0; i < current_req.num_headers; i++) {
        ESP_LOGI(TAG, "Header[%d]: %s = %s",
                 (int)i,
                 current_req.headers[i].field,
                 current_req.headers[i].value);
    }
    if (current_req.body) {
        ESP_LOGI(TAG, "Body (%d bytes): %.*s", (int)current_req.body_len, (int)current_req.body_len, current_req.body);
    }
    ESP_LOGI(TAG, "=======================");
    return 0;
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
        return false;
    }
    ESP_LOGI(TAG, "TCP connection from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // 1. 先讀 header（直到 "\r\n\r\n"）
    char header_buf[BUFFER_SIZE];
    int total_header_len   = 0;
    int header_end_index   = -1;
    VecU8 vec_u8          = vec_u8_new();

    while (1) {
        int len = recv(client_sock,
                       header_buf + total_header_len,
                       20,
                       // BUFFER_SIZE - total_header_len,
                       0);
        if (len < 0) {
            // recv 出錯
            ESP_LOGE(TAG, "recv() failed: errno %d", errno);
            close(client_sock);
            return false;
        } else if (len == 0) {
            // 對端已經關閉連線（EOF）
            ESP_LOGW(TAG, "Client closed connection before sending full header");
            close(client_sock);
            return false;
        }
        total_header_len += len;
        ESP_LOGI(TAG, "recv get \n%.*s", (int)total_header_len, header_buf);
        char *pos = strstr(header_buf, "\r\n\r\n");
        if (pos != NULL) {
            header_end_index = (int)(pos - header_buf) + 4; // 包含 "\r\n\r\n"
            break;
        }
        // 如果 buffer 滿了但還沒找到 "\r\n\r\n"，可考量擴大 buffer 或回報錯誤
        if (total_header_len >= BUFFER_SIZE) {
            ESP_LOGE(TAG, "Header too large or missing CRLFCRLF");
            close(client_sock);
            return false;
        }
    }

    // 2. 解析 Content-Length
    int content_len = 0;
    {
        header_buf[header_end_index] = '\0';
        char *cl_key = "Content-Length:";
        char *cl_pos = strcasestr(header_buf, cl_key);
        if (cl_pos) {
            cl_pos += strlen(cl_key);
            while (*cl_pos == ' ' || *cl_pos == '\t') { cl_pos++; }
            content_len = atoi(cl_pos);
        } else {
            content_len = 0;
        }
    }
    ESP_LOGI(TAG, "Parsed Content-Length = %d", content_len);

    // 3. 把 header 部分先 push 到 vec_u8
    vec_u8.push(&vec_u8, (uint8_t *)header_buf, header_end_index);

    // 4. 如果 header_buf 中已經含有部分 body，就也 push
    int already_body = total_header_len - header_end_index;
    if (already_body > 0) {
        vec_u8.push(&vec_u8,
                    (uint8_t *)(header_buf + header_end_index),
                    already_body);
    }

    // 5. 若 body 還沒接收完，就持續 recv，直到讀滿 content_len 字節
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
    ESP_LOGI(TAG, "Body (%d bytes): %.*s", (int)current_req.body_len, (int)current_req.body_len, current_req.body);

    // -------- 6. 使用 http_parser- 解析剛剛收到的完整 raw bytes --------
    {
        // 6.1 初始化 parser & settings
        http_parser parser;
        http_parser_settings settings;
        http_parser_init(&parser, HTTP_REQUEST);
        memset(&settings, 0, sizeof(settings));

        // 6.2 設定 callback
        settings.on_message_begin    = on_message_begin_cb;
        settings.on_url              = on_url_cb;
        settings.on_header_field     = on_header_field_cb;
        settings.on_header_value     = on_header_value_cb;
        settings.on_headers_complete = on_headers_complete_cb;
        settings.on_body             = on_body_cb;
        settings.on_message_complete = on_message_complete_cb;

        // 6.3 呼叫 execute()，把 vec_u8.data、vec_u8.len 全塞進去
        size_t nparsed = http_parser_execute(&parser,
                                             &settings,
                                             (const char *)vec_u8.data,
                                             vec_u8.len);
        if (nparsed != vec_u8.len) {
            enum http_errno err = HTTP_PARSER_ERRNO(&parser);
            ESP_LOGE(TAG, "http_parser_execute error: %s (%s)",
                     http_errno_description(err),
                     http_errno_name(err));
            // 可以選擇在此丟棄這筆 request 或做錯誤處理
        }
        // 到這裡，on_message_complete_cb 裡面的 ESP_LOGI 會印出解析後的內容
    }

    return true;
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
