set(app_src board_app.c)
set(pri_req esp_io)
idf_component_register(SRCS ${app_src}
                    INCLUDE_DIRS "."
                    REQUIRES ${pri_req})
