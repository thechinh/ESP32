/* Simple HTTP Server Example

A simple example that demonstrates how to create GET and POST
handlers for the web server.
*/

#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>
#include "esp_netif.h"
#include <esp_event.h>
// #include "protocol_examples_common.h"

#include <esp_http_server.h>
#include "http_server_app.h"
#include "ledc_app.h"
#include <ctype.h>
#include "freertos/event_groups.h"
#include <string.h>
#include "esp_wifi.h"
#include "sdkconfig.h"

#include "dht11.h"
#include "ws2812b.h"
#include "smartconfig.h"
#include "driver/gpio.h"
#define LED         GPIO_NUM_2
#define LED_R       GPIO_NUM_14
#define LED_Y       GPIO_NUM_25
#define LED_G       GPIO_NUM_26
#define DHT11       GPIO_NUM_27
#define LIGHT_SS    GPIO_NUM_13
#define WS2812     GPIO_NUM_33

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define DEFAULT_SSID      "Nguyen The Chinh_2.4G"
#define DEFAULT_PASS      "0902819916"
#define WIFI_MAXIMUM_RETRY  5
static const char *TAG = "wifi station";

EventGroupHandle_t xEventGroup;
#define BIT_SLIDER	( 1 << 0 )
char* slider_data;
int slider_len;

struct dht11_reading dht_data;
bool light;

// ############## HANDLER ##################

void task_read_sensor(void* pvParam) 
{
    struct dht11_reading dht_reading;
    while(1) {
        // DHT
        dht_reading = DHT11_read();
        printf("%.1f, %.0f, %d \n", dht_reading.temperature, dht_reading.humidity, dht_reading.status);
        if (dht_reading.status == DHT11_OK && dht_reading.temperature > 0 && dht_reading.humidity > 0)
            dht_data = dht_reading;

        // Light sensor
        light = gpio_get_level(LIGHT_SS);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void get_dht11_callback(httpd_req_t *req)
{
    char res[30];
    sprintf(res, "{\"temp\":%.1f, \"hum\":%.0f}", dht_data.temperature, dht_data.humidity); // humidity has integer value
    httpd_resp_send(req, res, strlen(res));
}

void led_switch_callback(char* data, int len)
{
    if (strcmp(data, "ON") == 0)
        app_ledc_update(LEDC_CHANNEL_0, 100);
    else if (strcmp(data, "OFF") == 0)
        app_ledc_update(LEDC_CHANNEL_0, 0);
}

void slider_callback(char* data, int len)
{
    /* Since slider data stream could be a lot, we need to be fast here. 
    *  Just save it and process later by event handler.
    *  Even in case we miss processing some data, the lastest one should be captured
    */
    slider_data = data;
    slider_len = len;
    xEventGroupSetBits(xEventGroup, BIT_SLIDER);                             
}

void change_led_brightness(void* pvParam)
{
    EventBits_t uxBits;
    const TickType_t xTicksToWait = 100 / portTICK_PERIOD_MS;

    while (1) {
        uxBits = xEventGroupWaitBits(
                xEventGroup,   /* The event group being tested. */
                BIT_SLIDER,    /* The bits within the event group to wait for. */
                pdTRUE,        /* BIT should be cleared before returning. */
                pdFALSE,       /* Don't wait for both bits, either bit will do. */
                xTicksToWait );/* Wait a maximum of 100ms for either bit to be set. */

        if (uxBits & BIT_SLIDER) {
            char cleanedStr[4];             // brightness is from 0-100 
            memcpy(cleanedStr, slider_data, slider_len);
            cleanedStr[slider_len] = '\0';  // Remove unwanted characters at the end
            
            int brightness = atoi(cleanedStr);
            printf("brightness = %d \n", brightness);
            app_ledc_update(LEDC_CHANNEL_0, brightness);
        }
    }
}

// void rgb_callback(char* data, int len) {
//     char rgb[7];        // RGB data is always 6 character, e.g. A5FF60
//     memcpy(rgb, data, len);
//     rgb[len] = '\0';    // Remove unwanted characters at the end
//     printf("RGB: %s\n", rgb);
// }

void rgb_callback(httpd_req_t* req) {
    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    char*  buf;
    size_t buf_len;
    
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[7]; // rgb string len is 6 + end string char '\0'
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "color", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => color=%s", param);
            }
        }
        free(buf);
    }

    ws2812_update(40, 255, 255, 0);
}

void wifiinfo_callback(char* data, int len) {
    // RX format ssid@pw, for e.g. chinhwifi@123456@
    char ssid[32] = "";        
    char pw[64] = "";
    data[len] = '\0';
    printf("data: %s\n", data);
    char* token = strtok(data, "@");
    if (token != NULL)
        strcpy(ssid, token);
    
    token = strtok(NULL, "@");
    if (token != NULL)
        strcpy(pw, token);

    printf("Wifi: %s:%s\n", ssid, pw);

    esp_wifi_stop(); // disconnect_handler() will call webserver_stop()
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));    
    wifi_config_t wifi_config = {
        .sta = {
            // .ssid = *(uint8_t*) ssid, // error: missing braces around initializer [-Werror=missing-braces]
            // .password = *(uint8_t*) pw,
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK, // TODO: check
	     .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    strcpy((char*)wifi_config.sta.ssid, ssid);
    strcpy((char*)wifi_config.sta.password, pw);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() ); 
    // CHINH: event WIFI_EVENT_STA_START will happen -> call esp_wifi_start()

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        printf("connected to ap SSID:%s password:%s \n",
                 ssid, pw);
    } else if (bits & WIFI_FAIL_BIT) {
        printf("Failed to connect to SSID:%s, password:%s \n",
                 ssid, pw);
    } else {
        printf("ERROR: UNEXPECTED EVENT \n");
    }
    // start_webserver(); // connect_handler() will call
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = DEFAULT_SSID, 
            .password = DEFAULT_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,
	     .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 DEFAULT_SSID, DEFAULT_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 DEFAULT_SSID, DEFAULT_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

// ########## MAIN ##########
void app_main(void)
{
    static httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(nvs_flash_init());
    // wifi_init_sta();
    smartconfig_start();

    /* Register event handlers to stop the server when Wi-Fi is disconnected,
     * and re-start it upon connection.
     */
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

    get_dht11_set_callback(get_dht11_callback);
    led_switch_set_callback(led_switch_callback);
    slider_set_callback(slider_callback);
    rgb_set_callback(rgb_callback);
    wifiinfo_set_callback(wifiinfo_callback);

    app_ledc_init(LEDC_CHANNEL_0, LED);
    xEventGroup = xEventGroupCreate();
    xTaskCreate(change_led_brightness, "led brightness", 4096, NULL, 4, NULL);
    xTaskCreate(task_read_sensor, "dht", 4096, NULL, 4, NULL);

    DHT11_init(GPIO_NUM_27);
    gpio_pad_select_gpio(LIGHT_SS);
    gpio_sleep_set_direction(LIGHT_SS, GPIO_MODE_INPUT);

    ws2812_init(WS2812, 40);

    /* Start the server for the first time */
    server = start_webserver();

}
