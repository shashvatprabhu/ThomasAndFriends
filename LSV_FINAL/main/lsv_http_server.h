#ifndef LSV_HTTP_SERVER_H
#define LSV_HTTP_SERVER_H

#include <string.h>
#include <fcntl.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mdns.h"
#include "lwip/apps/netbiosns.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "sdkconfig.h"
#include "config.h"

#define MDNS_INSTANCE "LSV Potentiostat Web Server"
#define MDNS_HOST_NAME CONFIG_MDNS_HOST_NAME
#define WEB_MOUNT_POINT "/www"
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)
#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

typedef struct lsv_data_json
{
    float voltage[300];
    float current[300];
    int num_points;
    float baseline_current;
    bool scan_complete;
    bool data_changed;
} lsv_data_json_t;

lsv_data_json_t read_lsv_data();
void reset_lsv_data_changed();
void start_lsv_http_server();

#endif