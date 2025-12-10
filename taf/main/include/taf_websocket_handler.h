#ifndef TAF_WEBSOCKET_SERVER_H
#define TAF_WEBSOCKET_SERVER_H

#include <string.h>
#include <fcntl.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mdns.h"
#include "lwip/apps/netbiosns.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "wifi_handler.h"
#include "websocket_server.h"  // This is from the websocket component
#include "lwip/api.h"

#define MDNS_INSTANCE "TAF Density Measurement Web Server"
#define MDNS_HOST_NAME CONFIG_MDNS_HOST_NAME

typedef struct weight_data
{
    float weight_in_air;
    float weight_in_water;
    float density;
    bool new_data;
} weight_data_t;

void start_websocket_server();
weight_data_t read_weight_data();
void reset_new_data_flag();
void send_density_data();

#endif // TAF_WEBSOCKET_SERVER_H
