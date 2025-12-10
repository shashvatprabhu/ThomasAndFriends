set(COMPONENT_SRCS "taf_main.c" "wifi_handler.c" "taf_http_server.c")
set(COMPONENT_ADD_INCLUDEDIRS "include")

register_component()

spiffs_create_partition_image(www "${CMAKE_CURRENT_SOURCE_DIR}/../frontend" FLASH_IN_PROJECT)
