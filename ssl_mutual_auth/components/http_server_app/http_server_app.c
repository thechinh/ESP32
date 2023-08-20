#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_system.h>
#include "esp_eth.h"
#include <esp_http_server.h>
#include "http_server_app.h"

http_post_cb_t wifiinfo_cb = NULL;

extern const uint8_t html_start[]             asm("_binary_index_html_start");
extern const uint8_t html_end[]               asm("_binary_index_html_end");
extern const uint8_t html_wifi_config_start[] asm("_binary_wifi_config_html_start");
extern const uint8_t html_wifi_config_end[]   asm("_binary_wifi_config_html_end");

static const char *TAG = "http_server";


void stop_webserver(httpd_handle_t server)
{
    if (server != NULL) {
        httpd_stop(server);
        server = NULL;
    }
    ESP_LOGI(TAG, "Webserver stopped");
}

// ##########################
// ##########################

static esp_err_t http_post_handler(httpd_req_t *req)
{
    char buf[64];

    /* Read the data for the request */
    httpd_req_recv(req, buf, req->content_len);
    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    wifiinfo_cb(buf, req->content_len);
    
    return ESP_OK;
}

// ********* SHOW HTML ***********
static esp_err_t show_homepage_handler(httpd_req_t *req)
{
    return httpd_resp_send(req, (char*)html_start, html_end - html_start);
}

static const httpd_uri_t show_homepage = {
    .uri       = "/index.html",
    .method    = HTTP_GET,
    .handler   = show_homepage_handler,
    .user_ctx  = "Show html!"
};

static esp_err_t show_wifi_config_handler(httpd_req_t *req)
{
    return httpd_resp_send(req, (char*)html_wifi_config_start, html_wifi_config_end - html_wifi_config_start);
}

static const httpd_uri_t show_wifi_config = {
    .uri       = "/wifi_config.html",
    .method    = HTTP_GET,
    .handler   = show_wifi_config_handler,
    .user_ctx  = "Show html!"
};

// ################## HTTP POST #########################

// ****** Wifi Info *******
static const httpd_uri_t send_wifi_info = {
    .uri       = "/wifiinfo",
    .method    = HTTP_POST,
    .handler   = http_post_handler,
    .user_ctx  = NULL
};

void wifiinfo_set_callback(void* cb)
{
    wifiinfo_cb = cb;
}
// ##########################
// ##########################

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &show_homepage);
        httpd_register_uri_handler(server, &show_wifi_config);
        httpd_register_uri_handler(server, &send_wifi_info);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}
