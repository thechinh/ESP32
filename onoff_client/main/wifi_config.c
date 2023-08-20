#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi_config.h"
#include "smartconfig.h"
#include "http_server_app.h"

EventGroupHandle_t s_wifi_event_group;
provision_type_t provision_type = PROVISION_ACCESSPOINT;
static const char* TAG = "APP_WIFI_CONFIG";
httpd_handle_t server = NULL;

bool is_provisioned(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    if (conf.sta.ssid[0] == 0x00)
        return false;
    else
        return true;
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    static int s_retry_num = 0;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_WIFIMAXIMUM_RETRY) {
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
    // AP mode
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifiinfo_callback(char* data, int len) {
    // RX format ssid@pw, for e.g. chinhwifi@123456@
    char ssid[32] = "";        
    char pw[64] = "";
    data[len] = '\0';
    char* token = strtok(data, "@");
    if (token != NULL)
        strcpy(ssid, token);
    
    token = strtok(NULL, "@");
    if (token != NULL)
        strcpy(pw, token);

    esp_wifi_stop();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));    
    wifi_config_t wifi_config = {
        .sta = {
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK, // TODO: check
	     .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    strcpy((char*)wifi_config.sta.ssid, ssid);
    strcpy((char*)wifi_config.sta.password, pw);
    ESP_LOGI(TAG, "Wifi info rx: %s - %s", wifi_config.sta.ssid, wifi_config.sta.password);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start()); 

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s \n",
                 ssid, pw);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s \n",
                 ssid, pw);
    } else {
        ESP_LOGI(TAG, "ERROR: UNEXPECTED EVENT \n");
    }
    // stop_webserver(server);
}

void ap_config_start()
{
    wifiinfo_set_callback(wifiinfo_callback);
    
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = DEFAULT_AP_WIFI_SSID,
            .ssid_len = strlen(DEFAULT_AP_WIFI_SSID),
            .channel = DEFAULT_AP_WIFI_CHANNEL,
            .password = DEFAULT_AP_WIFI_PW,
            .max_connection = DEFAULT_AP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };
    if (strlen(DEFAULT_AP_WIFI_PW) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             DEFAULT_AP_WIFI_SSID, DEFAULT_AP_WIFI_PW, DEFAULT_AP_WIFI_CHANNEL);

    server = start_webserver();
}


void wifi_config(void)
{
    s_wifi_event_group = xEventGroupCreate();
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);
    
    // Common setup for all wifi modes
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta(); // for Station mode
    esp_netif_create_default_wifi_ap();  // for AP mode

    if (is_provisioned())
    {
        ESP_LOGI(TAG, "Wifi config existed");
        ESP_ERROR_CHECK(esp_wifi_start());
        xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        ESP_LOGI(TAG, "Wifi connected!");
    }
    else {
        if (provision_type == PROVISION_SMARTCONFIG)
        {
            ESP_LOGI(TAG, "Smartconfig starts...");
            smartconfig_start();
        }
        else if (provision_type == PROVISION_ACCESSPOINT)
        {
            ESP_LOGI(TAG, "AccessPoint starts...");
            ap_config_start();
        }
    }
}