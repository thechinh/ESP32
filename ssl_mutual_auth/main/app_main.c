/* MQTT Mutual Authentication Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
// #include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "mqtt_app.h"
#include "wifi_config.h"
#include "http_server_app.h"
#include "driver/gpio.h"
#include "esp_io.h"

#define LED GPIO_NUM_2
static const char *TAG = "MAIN";


static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == MQTT_DEV_EVENT)
    {
        if (event_id == MQTT_DEV_EVENT_CONNECTED) {
            ESP_LOGW(TAG, "Connected");
        }
        else if (event_id == MQTT_DEV_EVENT_DISCONNECTED) {
            ESP_LOGW(TAG, "Disconnected");
        }
        else if (event_id == MQTT_DEV_EVENT_SUBSCRIBED) {
            ESP_LOGW(TAG, "Subscribed");
        }
        else if (event_id == MQTT_DEV_EVENT_DATA) {
            ESP_LOGW(TAG, "Data");
            // mqtt_data_callback(data, len);
        }

    }
}

void mqtt_data_callback(char* data, int len) {
    printf("%s \n", data);
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(MQTT_DEV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    
    mqtt_app_set_data_callback(mqtt_data_callback);

    wifi_config();
    // start_webserver();

    esp_output_init(LED);

    mqtt_app_start();
    
}
