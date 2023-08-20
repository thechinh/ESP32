/* LEDC (LED Controller) basic example */

#include <stdio.h>
#include "driver/ledc.h"

#include "ledc_app.h"

esp_err_t app_ledc_init(ledc_channel_t channel, int pin)
{
    // Prepare the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .freq_hz          = 5000,  
        .clk_cfg          = LEDC_AUTO_CLK
    };
    
    // Prepare the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = channel,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = pin,
        .duty           = 0,    // Set init duty to 0%
        .hpoint         = 0
    };

    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    return ESP_OK;
}

esp_err_t app_ledc_update(ledc_channel_t channel, int brightness)
{
    int duty = brightness * 10;     // 100% brightness ~ 1024
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, channel));

    return ESP_OK;
}
