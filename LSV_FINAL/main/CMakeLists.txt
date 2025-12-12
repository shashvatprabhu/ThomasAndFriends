idf_component_register(SRCS "main.c"
                            "i2c_devices.c"
                            "lsv_scan.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver nvs_flash)
