/* OTA example

DEMO
- Step 0: Make sure that both new and old image have the same partitions.csv
- Step 1: Build a new image, e.g. Blink -> output "blink.bin" in Build folder
- Step 2: Run Simple Web Server on local computer, let it host a folder contains
new image (blink.bin)
- Step 3: Edit the IMAGE_URL var, build and run. Make sure web server and esp are on
the same network

It should auto-connect to server, download and run the new image. Both new and old image
are stored in OTA1/2

NOTE: 
- Clear content of Kconfig.projbuild
- Menuconfig > Component config > ESP HTTPS OTA > Allow HTTP for OTA
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "protocol_examples_common.h"
#include "string.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include <sys/socket.h>
#if CONFIG_EXAMPLE_CONNECT_WIFI
#include "esp_wifi.h"
#endif

static const char *TAG = "simple_ota_example";
char* IMAGE_URL = "http://192.168.1.5:8080/blink.bin";
int duty;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    }
    return ESP_OK;
}

int simple_ota_task(char* IMAGE_URL, int* duty)
{
    // Config
    ESP_LOGI(TAG, "Starting OTA example");
    esp_http_client_config_t config = {
        .url = IMAGE_URL,
        .event_handler = _http_event_handler,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };
    ESP_LOGI(TAG, "Attempting to download update from %s", config.url);

    // Start
    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (https_ota_handle == NULL) {
        return ESP_FAIL;
    }
    
    // Show progress
    int image_len_total = esp_https_ota_get_image_size(https_ota_handle);
    int duty_old = 0;
    
    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }

        // Calculate progress
        int download_len = esp_https_ota_get_image_len_read(https_ota_handle);
        *duty = download_len * 100 / image_len_total;
        if(*duty != duty_old) {
            printf("Downloading: %d %%\n", *duty);
            duty_old = *duty;
        }
    }

    // Clean-up HTTPS OTA Firmware upgrade and close HTTP(S) connection
    if (err != ESP_OK) {
        esp_https_ota_abort(https_ota_handle);
        return err;
    }

    // Switch the boot partition to the OTA partition containing the new firmware image.
    esp_err_t ota_finish_err = esp_https_ota_finish(https_ota_handle);

    if (ota_finish_err == ESP_OK) {
        ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Firmware upgrade failed");
        return ota_finish_err;
    }
    return ESP_OK;
}

void simple_ota_example_task(void* pvParameter)
{
    simple_ota_task(IMAGE_URL, &duty);
    
    // since this is a task, it should not return or exit
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

#if CONFIG_EXAMPLE_CONNECT_WIFI
    /* Ensure to disable any WiFi power save mode, this allows best throughput
     * and hence timings for overall OTA operation.
     */
    esp_wifi_set_ps(WIFI_PS_NONE);
#endif // CONFIG_EXAMPLE_CONNECT_WIFI

    xTaskCreate(&simple_ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);
}
