set(app_src json_app.c json_generator.c json_parser.c)
set(pri_req  )
idf_component_register(SRCS ${app_src}
                    INCLUDE_DIRS "."
                    REQUIRES ${pri_req})
