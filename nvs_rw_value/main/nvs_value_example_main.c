/* Non-Volatile Storage (NVS) Read and Write a Value - Example

   For other examples please check:
   https://github.com/espressif/esp-idf/tree/master/examples

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "nvs_io.h"

#define NAMESPACE   "data_user"
#define KEY1        "reset_cnt"
#define KEY2        "ssid"
#define KEY3        "date"

uint32_t restart_counter = 0;
char ssid[50];

date_t date = {
    .day = 20,
    .month = 8,
    .year = 1986,
};

date_t date_out; 

void app_main(void)
{
    esp_nvs_init();

    // KEY 1 - restart counter (int)
    esp_nvs_read(NAMESPACE, KEY1, &restart_counter, "int");
    printf("Restart counter = %d \n", restart_counter);

    restart_counter++;
    esp_nvs_write(NAMESPACE, KEY1, &restart_counter, "int");
    printf("\n");

    // KEY 2 - ssid (str)
    esp_nvs_write(NAMESPACE, KEY2, "Nguyen The Chinh", "str");

    esp_nvs_read(NAMESPACE, KEY2, ssid, "str");
    printf("SSID = %s \n", ssid);

    // KEY 3 - date (struct)
    esp_nvs_write(NAMESPACE, KEY3, &date, "struct");
    esp_nvs_read(NAMESPACE, KEY3, &date_out, "struct");
    printf("%d - %d - %d\n", date_out.day, date_out.month, date_out.year);
}


