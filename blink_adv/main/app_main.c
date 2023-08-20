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
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_io.h"

#define BUTTON              0
#define LED                 2

#define BIT_SHORT_PRESS  (1<<0)

EventGroupHandle_t xEventGroup;

void button_ISR_callback(int pin, uint64_t tick) {
    int press_period;

    if (pin == BUTTON) {       
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        press_period = tick * portTICK_PERIOD_MS;
        if (press_period < 500)
            xEventGroupSetBitsFromISR(xEventGroup, BIT_SHORT_PRESS, &xHigherPriorityTaskWoken );        
    }
}

void button_event_handle(void* pvParam) {
    for (;;) {
        EventBits_t uxBits = xEventGroupWaitBits(xEventGroup,   /* The event group being tested. */
                                                 BIT_SHORT_PRESS, /* The bits within the event group to wait for. */
                                                 pdTRUE,        /* BIT_0 & BIT_4 should be cleared before returning. */
                                                 pdFALSE,       /* Don't wait for both bits, either bit will do. */
                                                 portMAX_DELAY );/* Wait a maximum of 100ms for either bit to be set. */

        if (uxBits & BIT_SHORT_PRESS) {
            printf("SHORT PRESS \n");
            gpio_set_level(LED, 0);
        }
    }
}

void timeout_callback(int pin) {
    printf("LONG PRESS \n");
    gpio_set_level(LED, 1);
}

void app_main(void)
{
    esp_config_output(LED);
    
    esp_config_input(BUTTON, GPIO_INTR_ANYEDGE); // config button pin 0
    esp_input_set_callback(button_ISR_callback);
    
    esp_timeout_set_callback(timeout_callback);

    xEventGroup = xEventGroupCreate();
    xTaskCreate(
                button_event_handle,       /* Function that implements the task. */
                "handle different events for button",          /* Text name for the task. */
                2048,   /* Stack size in words, not bytes. */
                NULL,   /* Parameter passed into the task. */
                4,      /* Priority at which the task is created. */
                NULL);  /* Used to pass out the created task's handle. */

}
    