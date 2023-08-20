#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#include <string.h>
#include "nvs_io.h"

void esp_nvs_init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
}

esp_err_t esp_nvs_write(char* NAMESPACE, char* KEY, void* data, char* type) {
    nvs_handle_t my_handle;
    esp_err_t err;
    
    // Open
    err = nvs_open(NAMESPACE, NVS_READWRITE, &my_handle);
    
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    // Write
    else {
        if (strcmp(type, "int") == 0) {
            int32_t value = *(int32_t *)data;
            err = nvs_set_i32(my_handle, KEY, value);
        }
        else if (strcmp(type, "str") == 0) {
            char* value = (char*)data;
            err = nvs_set_str(my_handle, KEY, value);
        }
        else if (strcmp(type, "struct") == 0) {
            date_t* value = (date_t*)data;
            err = nvs_set_blob(my_handle, KEY, data, sizeof(*value));
        }
        
        // Commit
        if (err != ESP_OK) 
            printf("Error %s writting \n", esp_err_to_name(err));
        else {
            err = nvs_commit(my_handle);
            if (err != ESP_OK)
                printf("Error (%s) committing updates in NVS! \n", esp_err_to_name(err));
        }
    }
    
    // Close
    nvs_close(my_handle);
    return err;
}

esp_err_t esp_nvs_read(char* NAMESPACE, char* KEY, void* data, char* type) 
{
    nvs_handle_t my_handle;
    esp_err_t err;
    size_t len = 0;

    // Open
    err = nvs_open(NAMESPACE, NVS_READWRITE, &my_handle);
    
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    // Read 
    else {
        if (strcmp(type, "int") == 0) {
            int32_t* value = (int32_t*)data;
            err = nvs_get_i32(my_handle, KEY, value);
        }
        else if (strcmp(type, "str") == 0) {
            char* value = (char*)data;
            err = nvs_get_str(my_handle, KEY, NULL, &len);
            err = nvs_get_str(my_handle, KEY, value, &len);
        }
        else if (strcmp(type, "struct") == 0) {
            err = nvs_get_blob(my_handle, KEY, NULL, &len);
            err = nvs_get_blob(my_handle, KEY, data, &len);
        }

        switch (err) {
            case ESP_OK:
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
    }

    // Close
    nvs_close(my_handle);
    return err;
}
