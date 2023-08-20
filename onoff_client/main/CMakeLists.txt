set(srcs "main.c"
        "board.c" "wifi_config.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS  ".")
