#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_uart.h"
#include "esp_output.h"

// Chinh: if uart rcv "on" -> ON LED, "off" -> OFF LED
void uart_rx_callback (uint8_t *data, uint16_t length)
{
    printf("DATA: %.*s\n", length, (char*)data);
    if (strstr((char*)data, "on")) {
        esp_output_set_level(2, 1);
    } 
    if (strstr((char*)data, "off")) {
        esp_output_set_level(2, 0);
    }
}

void app_main(void)
{
    esp_output_init(2); // set pin 2 to output
    esp_uart_set_rx_callback(uart_rx_callback);
    esp_uart_init(115200);
}
