#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_io.h"

#define BUTTON      GPIO_NUM_0
#define LED         GPIO_NUM_2

#define BIT_EVENT_BUTTON (1 << 0)

EventGroupHandle_t xEventGroup;

void buttonISR(int pin)
{
    // NEVER EVER printf inside ISR
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(xEventGroup, BIT_EVENT_BUTTON, &pxHigherPriorityTaskWoken); 
}

void ledToggleTask(void* pvParam) {
    EventBits_t uxBits;
    // const TickType_t xTicksToWait = 100 / portTICK_PERIOD_MS;

    for (;;) {
        printf("Waiting for button... \n");

        uxBits = xEventGroupWaitBits(
            xEventGroup,   /* The event group being tested. */
            BIT_EVENT_BUTTON, /* The bits within the event group to wait for. */
            pdTRUE,        /* Both bit should be cleared before returning. */
            pdFALSE,       /* Don't wait for both bits, either bit will do. */
            portMAX_DELAY);/* Wait a maximum of 100ms for either bit to be set. */

        if (uxBits & BIT_EVENT_BUTTON) {
            printf("Button pressed! \n");
            esp_output_toggle(2);
        }
    }
}

void backgroundTask(void* pvParam) {
    for(;;) {
        printf("Do background task here... \n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void timer0Task() {
    printf("Timer 0 expired, let's do something... \n");
}

void timer1Task() {
    printf("Timer 1 expired, let's do something... \n");
}

void vTimerCallback(TimerHandle_t xTimer) // All soft-timer call this when expired
{
    configASSERT(xTimer);
    int id = (uint32_t) pvTimerGetTimerID(xTimer);

    if (id == 0)
        timer0Task();
    else if (id == 1)
        timer1Task();
}

// *********** MAIN ***********
void app_main(void) {

    TimerHandle_t xTimers[2]; // Declare 2 soft timers

    // Config button and LED
    esp_config_output(LED);
    esp_config_input(BUTTON, GPIO_INTR_NEGEDGE);
    esp_input_set_callback(buttonISR);

    // Config tasks
    xEventGroup = xEventGroupCreate();
    xTaskCreate(backgroundTask, "Non-important tasks", 1024, NULL, 1, NULL);  // low priority
    xTaskCreate(ledToggleTask, "Handle button and LED", 4096, NULL, 4, NULL);

    // Config timers
    xTimers[0] = xTimerCreate("Timer 1", pdMS_TO_TICKS(1000), pdTRUE, (void *) 0, vTimerCallback);
    xTimers[1] = xTimerCreate("Timer 2", pdMS_TO_TICKS(1000), pdTRUE, (void *) 1, vTimerCallback);

    xTimerStart(xTimers[0], 0);
    xTimerStart(xTimers[1], 0);
}
