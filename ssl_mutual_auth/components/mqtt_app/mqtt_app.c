/* MQTT simple example

Setup
- Broker: hivemq
- Client: esp32 (this one) and MQTT fx

Usage
- Dung MQTT fx as a client dieu khien trang thai den led tren esp32. Mesage format JSON
- Dung MQTT fx as a client request trang thai hien tai cua led

*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "json_app.h"
#include "esp_io.h"
#include "driver/gpio.h"
#include "mqtt_app.h"

static const char *TAG = "MQTTS_EXAMPLE";
ESP_EVENT_DEFINE_BASE(MQTT_DEV_EVENT);

extern const uint8_t client_cert_pem_start[] asm("_binary_client_crt_start");
extern const uint8_t client_cert_pem_end[] asm("_binary_client_crt_end");
extern const uint8_t client_key_pem_start[] asm("_binary_client_key_start");
extern const uint8_t client_key_pem_end[] asm("_binary_client_key_end");
extern const uint8_t server_cert_pem_start[] asm("_binary_mosquitto_org_crt_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_mosquitto_org_crt_end");

static mqtt_data_handle_t mqtt_data_handle = NULL;
static esp_mqtt_client_handle_t client;

#define LED GPIO_NUM_2
int led_state = 0;
int prev_led_state = 0;
int update = 0;


void mqtt_update_board_status(esp_mqtt_client_handle_t client) {
    // Collect status and send json     
    json_gen_test_result_t result;
    int current_state = gpio_get_level(LED);
    json_generator_board_status(&result, 1, "esp32", current_state);
    printf("Current status: %s \n", result.buf);

    if (esp_mqtt_client_publish(client, "/chinh/status", result.buf, 0, 0, 0) != ESP_OK)  // update status
        ESP_LOGE(TAG, "Update board status failed");
}

static void mqtt_execute_command(esp_mqtt_event_handle_t event) 
{
    // Read MQTT JSON and control LED
    event->data[event->data_len] = '\0';    // Add end-of-string so that parser can read correctly
    json_parser_cmd(event->data, &led_state, &update);
    
    // Should call a callback here
    if (led_state != prev_led_state)
    {
        gpio_set_level(2, led_state);
        printf("TURN %s LED \n", (led_state==1) ? "ON" : "OFF");
        prev_led_state = led_state;
    }
    if (update == 1)
        mqtt_update_board_status(event->client);
}


static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:  // happen once when connect succesfully
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, "/chinh/cmd", 0);            // waiting for command
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            mqtt_update_board_status(client);
            esp_event_post(MQTT_DEV_EVENT, MQTT_DEV_EVENT_CONNECTED, NULL, 0, portMAX_DELAY);

            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            esp_event_post(MQTT_DEV_EVENT, MQTT_DEV_EVENT_DISCONNECTED, NULL, 0, portMAX_DELAY);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            esp_event_post(MQTT_DEV_EVENT, MQTT_DEV_EVENT_SUBSCRIBED, NULL, 0, portMAX_DELAY);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:           // happen every time RX data
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);

            mqtt_data_handle(event->data, event->data_len);
            mqtt_execute_command(event);
            esp_event_post(MQTT_DEV_EVENT, MQTT_DEV_EVENT_DATA, NULL, 0, portMAX_DELAY);

            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

void mqtt_app_start(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
    
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://broker.hivemq.com:1883",
        // .uri = "mqtts://test.mosquitto.org:8884",
        // .client_cert_pem = (const char *)client_cert_pem_start,
        // .client_key_pem = (const char *)client_key_pem_start,
        // .cert_pem = (const char *)server_cert_pem_start,
    };

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}


// Should test json online first
void test_json()
{
    json_gen_test_result_t result;
    json_generator_example(&result, "Chinh", true, 20, 8, 86);
    printf("%s \n", result.buf);

    char* json = "{\"name\":\"Chinh\", \"male\":true, \"dob\":{\"d\":20, \"m\":8, \"y\":86}}";
    char name[20];
    bool male;
    int d,m,y;

    json_parser(json, name, &male, &d, &m, &y);

    printf("%s\n", name);
    printf("%d\n", male);
    printf("%d\n", d);
    printf("%d\n", m);
    printf("%d\n", y);
}

void mqtt_app_set_data_callback(void *cb) {
    if (cb)
        mqtt_data_handle = cb;
}

void mqtt_app_publish(char* data, int len) {
    return esp_mqtt_client_publish(client, topic, data, len, 1, 0);
}

void mqtt_app_subscribe(char* topic) {
    return esp_mqtt_client_subscribe(client, topic, 1);
}