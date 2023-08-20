#ifndef __ESP_UART_H
#define __ESP_UART_H

typedef void (*uart_rx_handle_t) (uint8_t *data, uint16_t length);

void esp_uart_init(int baudrate);
void esp_uart_put(uint8_t *data, uint16_t length);
void esp_uart_set_rx_callback(void *cb);
#endif