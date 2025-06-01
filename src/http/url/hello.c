#include "http/base.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "http_server_example";

typedef struct {
    int count;    // 記錄 echo 被呼叫的次數
} hello_ctx_t;

static hello_ctx_t echo_ctx;

static esp_err_t hello_get_handler(httpd_req_t *req) {
    hello_ctx_t *ctx = (hello_ctx_t *)req->user_ctx;
    if (ctx == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No ctx");
        return ESP_FAIL;
    }

    char resp_str[64];
    snprintf(resp_str, sizeof(resp_str), "Hello, World! %d", ctx->count);
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    ctx->count++;

    return ESP_OK;
}

const httpd_uri_t hello_get_uri = {
    .uri       = "/hello",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    .user_ctx  = &echo_ctx,
};

static esp_err_t hello_post_handler(httpd_req_t *req) {
    int content_len = req->content_len;
    char resp_str[64];
    snprintf(resp_str, sizeof(resp_str), "Hello, World! %d", content_len);
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

const httpd_uri_t hello_post_uri = {
    .uri       = "/hello",
    .method    = HTTP_POST,
    .handler   = hello_post_handler,
    .user_ctx  = NULL
};
