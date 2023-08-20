#ifndef __MQTT_APP_H
#define __MQTT_APP_H

ESP_EVENT_DECLARE_BASE(MQTT_DEV_EVENT);

typedef enum {
    MQTT_DEV_EVENT_CONNECTED,
    MQTT_DEV_EVENT_DISCONNECTED,
    MQTT_DEV_EVENT_DATA,
    MQTT_DEV_EVENT_SUBSCRIBED,
    MQTT_DEV_EVENT_UNSUBSCRIBED,
    MQTT_DEV_EVENT_PUBLISHED,
} mqtt_dev_event;

typedef void (*mqtt_data_handle_t) (char* data, int len);

void mqtt_app_start(void);
void mqtt_app_set_data_callback(void *cb);
void mqtt_app_publish(char* data, int len);
void mqtt_app_subscribe(char* topic);
#endif