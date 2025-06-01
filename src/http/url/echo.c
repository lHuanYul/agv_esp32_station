#include "http/base.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "http_server_example";

/* ----- 2. 回呼函式：處理 POST /echo ----- */
static esp_err_t echo_post_handler(httpd_req_t *req)
{
    // 取得 Content-Length
    int total_len = req->content_len;
    if (total_len <= 0) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    // 分配緩衝區讀取 POST body
    char *buf = calloc(1, total_len + 1);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate buffer for POST data");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int received = 0;
    int ret;
    // 可能要分多次讀取
    while (received < total_len) {
        ret = httpd_req_recv(req, buf + received, total_len - received);
        if (ret <= 0) {
            free(buf);
            ESP_LOGE(TAG, "Failed to receive POST data");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        received += ret;
    }

    // 將收到的資料 echo 回去
    httpd_resp_send(req, buf, total_len);
    free(buf);
    return ESP_OK;
}

/* 定義 POST /echo 的 URI 結構 */
const httpd_uri_t echo_uri = {
    .uri       = "/echo",
    .method    = HTTP_POST,
    .handler   = echo_post_handler,
    .user_ctx  = NULL
};
