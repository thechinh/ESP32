#include "esp_io.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

input_callback_t input_cb = NULL;
timeoutButtonCb_t timeout_cb = NULL;
static uint64_t _start, _stop, _pressPeriod;
TimerHandle_t xTimer;

static void IRAM_ATTR gpio_input_handler(void* arg)
{
    uint32_t pin = (uint32_t) arg;
    uint32_t rtc = xTaskGetTickCountFromISR();

    if (gpio_get_level(pin) == 0) {
        xTimerStart(xTimer, 0);
        _start = rtc;
    }
    else {
        xTimerStop(xTimer, 0);
        _stop = rtc;
        _pressPeriod = _stop - _start;
        input_cb(pin, _pressPeriod);
    }
}

void vTimerCallback(TimerHandle_t xTimer) {
    configASSERT(xTimer);
    uint32_t ID = (uint32_t) pvTimerGetTimerID(xTimer);
    if (ID==0)
    {    
        timeout_cb();
    }
}

void esp_input_set_callback(void * cb) {
    input_cb = cb;
}

void esp_timeout_set_callback(void * cb) {
    timeout_cb = cb;
}

void esp_config_input(gpio_num_t pin, gpio_int_type_t type) {
    gpio_pad_select_gpio(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(pin, type);

    //install default gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(pin, gpio_input_handler, NULL); 

    xTimer = xTimerCreate("Timer 1", pdMS_TO_TICKS(3000), pdFALSE, (void *) 0, vTimerCallback);
}

// ********** OUTPUT ************

void esp_config_output(int pin)
{
    gpio_pad_select_gpio(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
}