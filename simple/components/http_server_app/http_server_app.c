#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_system.h>
#include "esp_eth.h"
#include <esp_http_server.h>
#include "http_server_app.h"

http_get_cb_t get_dht11_cb = NULL;
http_get_cb_t rgb_cb = NULL;
http_post_cb_t led_switch_cb = NULL;
http_post_cb_t slider_cb = NULL;
// http_post_cb_t rgb_cb = NULL;
http_post_cb_t wifiinfo_cb = NULL;

extern const uint8_t image_start[] asm("_binary_image_jpg_start");
extern const uint8_t image_end[]   asm("_binary_image_jpg_end");
extern const uint8_t html_start[] asm("_binary_index_html_start");
extern const uint8_t html_end[]   asm("_binary_index_html_end");

static const char *TAG = "http_server";

void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

/* This handler allows the custom error handling functionality to be
   tested from client side. */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/hello", req->uri) == 0)
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
    else if (strcmp("/image", req->uri) == 0)
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/image URI is not available");
    else
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    
    return ESP_OK;
}

void stop_webserver(httpd_handle_t server)
{
    httpd_stop(server);
}

// ##########################
// ##########################

// ********** HELLO **********
static esp_err_t hello_get_handler(httpd_req_t *req)
{
    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*) "Hello Chinh. This is the response from ESP32!";
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
    return ESP_OK;
}

static const httpd_uri_t hello = {
    .uri       = "/hello",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "Hello World!"
};

// ********* SHOW IMAGE ***********
static esp_err_t show_image_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/jpeg");
    esp_err_t err = httpd_resp_send(req, (char*)image_start, image_end - image_start);

    return err;
}

static const httpd_uri_t show_image = {
    .uri       = "/image",
    .method    = HTTP_GET,
    .handler   = show_image_handler,
    .user_ctx  = "Show image!"
};

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

// ********** GET DTH11 ***********

static esp_err_t get_dht11_handler(httpd_req_t *req)
{
    get_dht11_cb(req);
    return ESP_OK;
}

static const httpd_uri_t get_dht11 = {
    .uri       = "/dht11",
    .method    = HTTP_GET,
    .handler   = get_dht11_handler,
    .user_ctx  = "Get DHT11"
};

void get_dht11_set_callback(void* cb)
{
    get_dht11_cb = cb;
}

// ################## HTTP POST #########################
//  ********** Control LED switch from web *************
static esp_err_t http_post_handler(httpd_req_t *req)
{
    char buf[64];

    /* Read the data for the request */
    httpd_req_recv(req, buf, req->content_len);
    // End response
    httpd_resp_send_chunk(req, NULL, 0);

    if (strcmp(req->uri, "/led") == 0)
        led_switch_cb(buf, req->content_len);
    else if (strcmp(req->uri, "/slider") == 0)
        slider_cb(buf, req->content_len);
    // else if (strcmp(req->uri, "/rgb") == 0)
    //     rgb_cb(buf, req->content_len);
    else if (strcmp(req->uri, "/wifiinfo") == 0)
        wifiinfo_cb(buf, req->content_len);

    return ESP_OK;
}

static const httpd_uri_t led_switch = {
    .uri       = "/led",
    .method    = HTTP_POST,
    .handler   = http_post_handler,
    .user_ctx  = NULL
};

void led_switch_set_callback(void* cb)
{
    led_switch_cb = cb;
}

// ****** Slider *******
static const httpd_uri_t slider = {
    .uri       = "/slider",
    .method    = HTTP_POST,
    .handler   = http_post_handler,
    .user_ctx  = NULL
};

void slider_set_callback(void* cb)
{
    slider_cb = cb;
}

// ****** RGB *******
static esp_err_t http_get_rgb_handler(httpd_req_t *req)
{
    rgb_cb(req);
    return ESP_OK;
}

static const httpd_uri_t rgb = {
    .uri       = "/rgb",
    .method    = HTTP_GET, // experiment. We should use POST instead.
    .handler   = http_get_rgb_handler,
    .user_ctx  = NULL
};

void rgb_set_callback(void* cb)
{
    rgb_cb = cb;
}

// ****** Wifi Info *******
static const httpd_uri_t wifiinfo = {
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
        httpd_register_uri_handler(server, &hello);
        httpd_register_uri_handler(server, &show_image);
        httpd_register_uri_handler(server, &show_homepage);
        httpd_register_uri_handler(server, &get_dht11);
        httpd_register_uri_handler(server, &led_switch);
        httpd_register_uri_handler(server, &slider);
        httpd_register_uri_handler(server, &rgb);
        httpd_register_uri_handler(server, &wifiinfo);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler); // CHINH: custom error handler
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}
