#ifndef __HTTP_SERVER_APP_H
#define __HTTP_SERVER_APP_H

#include <esp_http_server.h>

typedef void (*http_post_cb_t) (char* data, int len);
void wifiinfo_set_callback(void* cb);

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err);

void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);

void disconnect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);

void stop_webserver(httpd_handle_t server);
httpd_handle_t start_webserver(void);

#endif