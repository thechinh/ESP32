#ifndef  __WS2812B_H
#define  __WS2812B_H

void ws2812_init(int tx_pin, int led_number);
void ws2812_update(uint32_t red, uint32_t green, uint32_t blue, int led_number);

#endif