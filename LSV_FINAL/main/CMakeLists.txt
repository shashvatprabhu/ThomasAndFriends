idf_component_register(SRCS "main.c"
                            "i2c_devices.c"
                            "i2c_debug.c"
                            "lsv_scan.c"
                            "peak_analysis.c"
                            "karat_estimation.c"
                            "lsv_http_server.c"
                       INCLUDE_DIRS "."
                       REQUIRES nvs_flash esp_wifi esp_http_server mdns spiffs cJSON)

spiffs_create_partition_image(www "${CMAKE_CURRENT_SOURCE_DIR}/../frontend" FLASH_IN_PROJECT)
