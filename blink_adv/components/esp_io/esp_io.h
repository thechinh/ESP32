#ifndef __ESP_IO_H
#define __ESP_IO_H

#include "hal/gpio_types.h"

typedef void (*input_callback_t) (int, uint64_t);
typedef void (*timeoutButtonCb_t) ();

void esp_config_input(gpio_num_t pin, gpio_int_type_t type);
void esp_config_output(int pin);

void esp_input_set_callback(void* cb);
void esp_timeout_set_callback(void* cb);
#endif