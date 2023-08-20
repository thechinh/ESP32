#ifndef __JSON_APP_H
#define __JSON_APP_H

typedef struct {
    char buf[512];
    int offset;
} json_gen_test_result_t;

int json_generator_board_status(json_gen_test_result_t *result, int id, char* board, bool led_state);
int json_generator_example(json_gen_test_result_t *result, char* name, bool male, int d, int m, int y);
int json_parser(char* json, char* name, bool *male, int *d, int *m, int* y);
int json_parser_cmd(char* json, char* addr, int* state, int* update);

#endif