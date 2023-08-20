typedef struct {
    uint8_t day;
    uint16_t month;
    uint32_t year;
} date_t;

void esp_nvs_init();
esp_err_t esp_nvs_read(char* NAMESPACE, char* KEY, void* data, char* type);
esp_err_t esp_nvs_write(char* NAMESPACE, char* KEY, void* data, char* type);
