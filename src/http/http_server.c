#include "http/server.h"
#include "http/base.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "http_server_example";

/* ----- 3. 啟動 HTTP Server，註冊 URI ----- */
httpd_handle_t http_start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // 啟動伺服器
    if (httpd_start(&server, &config) == ESP_OK) {
        // 成功後註冊 URI
        httpd_register_uri_handler(server, &hello_get_uri);
        httpd_register_uri_handler(server, &hello_post_uri);
        httpd_register_uri_handler(server, &echo_uri);
        ESP_LOGI(TAG, "HTTP Server Started");
        return server;
    }

    ESP_LOGE(TAG, "Failed to start HTTP Server");
    return NULL;
}

/* 可選：停止 HTTP Server */
static void stop_webserver(httpd_handle_t server)
{
    if (server) {
        httpd_stop(server);
        ESP_LOGI(TAG, "HTTP Server Stopped");
    }
}
