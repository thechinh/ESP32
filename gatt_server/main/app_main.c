#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "sdkconfig.h"

#include "ble_gatt_server.h"
#include "driver/gpio.h"

const char* TAG = "MAIN";

void app_ble_data_rx_callback(uint8_t* data, uint16_t len)
{
    printf("Data short:\n");
    if (strstr((char *)data, "led off")) {
        app_ble_send_data((uint8_t *)"OK", 2);
        gpio_set_level(2, 0);
    } else if (strstr((char *)data, "led on")) {
        gpio_set_level(2, 1);
    }
}

void app_main(void)
{
    esp_err_t ret;

    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    gpio_pad_select_gpio(2);
    gpio_set_direction(2, GPIO_MODE_OUTPUT);

    app_ble_set_data_rx_callback(app_ble_data_rx_callback);
    app_ble_start();
    
    return;
}