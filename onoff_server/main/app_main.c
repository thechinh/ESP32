/* main.c - Application main entry point */

/*
 * Copyright (c) 2017 Intel Corporation
 * Additional Copyright (c) 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "ble_mesh_example_init.h"
#include "ble_mesh_app.h"
#include "board_app.h"
#include "esp_event.h"

#define TAG "SERVER"

ESP_EVENT_DECLARE_BASE(IO_DEV_EVENT); // declare whereever we want to handle

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == IO_DEV_EVENT)
    {
        if (event_id == IO_DEV_EVENT_BUTTON_PUSHED) {
            ESP_LOGW(TAG, "BUTTON PUSHED!");
            // Announce built-in led status to client.
            server_send_to_client(0x000c);
        }
    }
}

void app_main(void)
{
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing...");

    board_init();

    gpio_pad_select_gpio(LED);
    gpio_set_direction(LED, GPIO_MODE_INPUT_OUTPUT);

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = bluetooth_init();
    if (err) {
        ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
        return;
    }

    ble_mesh_get_dev_uuid(dev_uuid);

    /* Initialize the Bluetooth Mesh Subsystem */
    err = ble_mesh_init();
    if (err) {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
    }

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(IO_DEV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

}
