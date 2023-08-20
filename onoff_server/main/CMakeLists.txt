set(srcs "app_main.c" "app_sensor.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS  ".")
