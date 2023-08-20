/* board.h - Board-specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 * Additional Copyright (c) 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _BOARD_H_
#define _BOARD_H_

#include "driver/gpio.h"
#include "esp_event.h"

// MY BOARD
#define BUTTON  GPIO_NUM_0

#define LED     GPIO_NUM_2
#define LED_G   GPIO_NUM_25
#define LED_Y   GPIO_NUM_26   
#define LED_R   GPIO_NUM_27

#define DHT11       GPIO_NUM_13
#define LIGHT_SS    GPIO_NUM_34
#define BUZZER      GPIO_NUM_32
#define WS2812      GPIO_NUM_33

#define LED_ON  1
#define LED_OFF 0

struct _led_state {
    uint8_t current;
    uint8_t previous;
    uint8_t pin;
    char *name;
};

typedef enum {
    IO_DEV_EVENT_BUTTON_PUSHED,
} io_dev_event;


void board_led_operation(uint8_t pin, uint8_t onoff);

void board_init(void);

#endif
