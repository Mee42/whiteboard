#include "http.h"
#include "main.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include <stdlib.h>
#include "ccode.h"

extern uint8_t index_html_start[] asm("_binary_index_html_start"); 
extern uint8_t index_html_end[]   asm("_binary_index_html_end"); 

static esp_err_t get_handler(httpd_req_t* req) {
    httpd_resp_send(req, (char*)index_html_start, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static size_t min(size_t a, size_t b) {
    return a < b ? a : b;
}

static esp_err_t run_ccode_http(httpd_req_t* req) {
    

   /* content length would give length of string */
    char content[100] = { 0 };

    /* Truncate if content length larger than the buffer */
    size_t recv_size = min(req->content_len, sizeof(content));

    httpd_req_recv(req, content, recv_size);

    ESP_LOGI("http", "got ccode %s", content);

    return run_ccode(content, req);
}


static const httpd_uri_t uri_get_root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_post_ccode = {
    .uri = "/ccode",
    .method = HTTP_POST,
    .handler = run_ccode_http,
};
httpd_uri_t uri_get_status_info = {
    .uri = "/status",
    .method = HTTP_GET,
    .handler = get_status_info,
};



httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_LOGI("http", "Starting server on port: %d", config.server_port); 
    httpd_handle_t server = NULL;
    if(httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get_root);
        httpd_register_uri_handler(server, &uri_post_ccode);
        httpd_register_uri_handler(server, &uri_get_status_info);
    }
    return server; // can be null
}

