#ifndef  __APP_CONFIG_H
#define __APP_CONFIG_H

#define ESP_WIFIMAXIMUM_RETRY   10
#define WIFI_CONNECTED_BIT      BIT0
#define WIFI_FAIL_BIT           BIT1

#define DEFAULT_AP_WIFI_SSID    "esp32"
#define DEFAULT_AP_WIFI_PW      "thechinh"
#define DEFAULT_AP_WIFI_CHANNEL 1
#define DEFAULT_AP_MAX_STA_CONN 4

typedef enum {
    PROVISION_ACCESSPOINT = 0,
    PROVISION_SMARTCONFIG = 1
}   provision_type_t;

void wifi_config(void);

#endif