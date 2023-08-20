/* GPIO Example

Learning outcome:
- How ISR on GPIO works
- How to use function pointers and callbacks

DEMO: 
    Push button 0 -> toggle LED

*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_io.h"

#define BLINK_GPIO 2

void input_event_callback(int pin) {
    static bool state = true;
    if (pin == 0) {
        gpio_set_level(BLINK_GPIO, state);
        state = !state;
    }
}

void app_main(void)
{
    esp_config_output(BLINK_GPIO);
    
    esp_config_input(0, GPIO_INTR_NEGEDGE); // config button pin 0
    esp_input_set_callback(input_event_callback);

}