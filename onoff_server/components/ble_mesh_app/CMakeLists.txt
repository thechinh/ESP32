set(app_src ble_mesh_app.c)
set(pri_req bt board_app)
idf_component_register(SRCS ${app_src}
                    INCLUDE_DIRS "."
                    REQUIRES ${pri_req})
