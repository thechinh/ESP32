/* board.c - Board-specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 * Additional Copyright (c) 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "board_app.h"
#include "esp_io.h"
#include "esp_event.h"

// #include "iot_button.h"  optional reference

#define TAG "BOARD"
ESP_EVENT_DEFINE_BASE(IO_DEV_EVENT); // Define once in the event source file


struct _led_state led_state[3] = {
    { LED_OFF, LED_OFF, LED_G, "green" },
    { LED_OFF, LED_OFF, LED_Y, "yellow"  },
    { LED_OFF, LED_OFF, LED_R, "red"   },
};

void board_led_operation(uint8_t pin, uint8_t onoff)
{
    for (int i = 0; i < 3; i++) {
        if (led_state[i].pin != pin) {
            continue;
        }
        if (onoff == led_state[i].previous) {
            ESP_LOGW(TAG, "led %s is already %s",
                     led_state[i].name, (onoff ? "on" : "off"));
            return;
        }
        gpio_set_level(pin, onoff);
        led_state[i].previous = onoff;
        return;
    }

    ESP_LOGE(TAG, "LED is not found!");
}

static void button_tap_cb(uint32_t gpio_num)
{
    // all button callbacks will go here first    
    esp_event_post(IO_DEV_EVENT, IO_DEV_EVENT_BUTTON_PUSHED, &gpio_num, sizeof(gpio_num), portMAX_DELAY);
}


static void board_button_init(void)
{
    esp_input_init(BUTTON, GPIO_INTR_NEGEDGE);
    esp_input_set_callback(button_tap_cb);
}

static void board_led_init(void)
{
    for (int i = 0; i < 3; i++) {
        gpio_reset_pin(led_state[i].pin);
        gpio_set_direction(led_state[i].pin, GPIO_MODE_OUTPUT);
        gpio_set_level(led_state[i].pin, LED_OFF);
        led_state[i].previous = LED_OFF;
    }
}

void board_init(void)
{
    board_led_init();
    board_button_init();
}
