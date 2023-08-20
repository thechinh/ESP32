#include "json_generator.h"
#include "json_parser.h"
#include "json_app.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "JSON_app";


static void flush_str(char *buf, void *priv)
{
    json_gen_test_result_t *result = (json_gen_test_result_t *)priv;
    if (result) {
        if (strlen(buf) > sizeof(result->buf) - result->offset) {
            ESP_LOGE(TAG, "Result Buffer too small\r");
            return;
        }
        memcpy(result->buf + result->offset, buf, strlen(buf));
        result->offset += strlen(buf);
    }
}

int json_generator_board_status(json_gen_test_result_t *result, int id, char* board, bool led_state)
{
	char buf[20];
    memset(result, 0, sizeof(json_gen_test_result_t));
	json_gen_str_t jstr;
	json_gen_str_start(&jstr, buf, sizeof(buf), flush_str, result);
	json_gen_start_object(&jstr);

	json_gen_obj_set_int(&jstr, "id", id);
    json_gen_obj_set_string(&jstr, "board", board);
    json_gen_obj_set_bool(&jstr, "led_state", led_state);
	
    json_gen_end_object(&jstr);
	json_gen_str_end(&jstr);

    return 0;
}

int json_generator_example(json_gen_test_result_t *result, char* name, bool male, int d, int m, int y)
{
	char buf[20];
    memset(result, 0, sizeof(json_gen_test_result_t));
	json_gen_str_t jstr;
	json_gen_str_start(&jstr, buf, sizeof(buf), flush_str, result);
	json_gen_start_object(&jstr);

    json_gen_obj_set_string(&jstr, "name", name);
    json_gen_obj_set_bool(&jstr, "male", male);
    
	json_gen_push_object(&jstr, "dob");
	json_gen_obj_set_int(&jstr, "d", d);
	json_gen_obj_set_int(&jstr, "m", m);
	json_gen_obj_set_int(&jstr, "y", y);
	json_gen_pop_object(&jstr);

    json_gen_end_object(&jstr);
	json_gen_str_end(&jstr);

    return 0;
}

int json_parser_example(char* json, char* name, bool *male, int *d, int *m, int* y)
{
	jparse_ctx_t jctx;
	int ret = json_parse_start(&jctx, json, strlen(json));
	if (ret != OS_SUCCESS) {
		printf("Parser failed\n");
		return -1;
	}

    if (json_obj_get_string(&jctx, "name", name, 20) != OS_SUCCESS)
        printf("ERROR \n");

    if (json_obj_get_bool(&jctx, "male", male) != OS_SUCCESS)
        printf("ERROR \n");

    if (json_obj_get_object(&jctx, "dob") == OS_SUCCESS) {
        if (json_obj_get_int(&jctx, "d", d) != OS_SUCCESS)
            printf("ERROR \n");
        if (json_obj_get_int(&jctx, "m", m) != OS_SUCCESS)
            printf("ERROR \n");
        if (json_obj_get_int(&jctx, "y", y) != OS_SUCCESS)
            printf("ERROR \n");
        json_obj_leave_object(&jctx);
    }

    json_parse_end(&jctx);
    return 0;
}

int json_parser_cmd(char* json, char* addr, int* led_state, int* update)
{
	jparse_ctx_t jctx;
	int ret = json_parse_start(&jctx, json, strlen(json));

    if (ret != OS_SUCCESS) {
		ESP_LOGE(TAG, "Parser failed");
		return -1;
	}

    if (json_obj_get_string(&jctx, "addr", addr, 7) != OS_SUCCESS)  // the address is 6 bytes, e.g 0x0007
        ESP_LOGI(TAG, "addr value missed");

    if (json_obj_get_int(&jctx, "led", led_state) != OS_SUCCESS)
        ESP_LOGI(TAG, "led value missed");

    if (json_obj_get_int(&jctx, "update", update) != OS_SUCCESS)
        ESP_LOGE(TAG, "update value missed");

    json_parse_end(&jctx);
    return 0;
}