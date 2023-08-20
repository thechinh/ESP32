#include "esp_io.h"
#include "driver/gpio.h"

input_callback_t input_cb = NULL;

static void IRAM_ATTR gpio_input_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    input_cb(gpio_num);
}

void esp_input_set_callback(void * cb) {
    input_cb = cb;
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
}

// ********** OUTPUT ************

void esp_config_output(int pin)
{
    gpio_pad_select_gpio(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
}