#ifndef __LEDC_APP_H
#define __LEDC_APP_H

#include "driver/ledc.h"

esp_err_t app_ledc_init(ledc_channel_t channel, int pin);
esp_err_t app_ledc_update(ledc_channel_t channel, int brightness);

#endif