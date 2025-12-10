#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "taf_http_server.h"

static const char *TAG = "taf_main";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting TAF Density Measurement System");
    ESP_LOGI(TAG, "Initializing HTTP server...");

    start_taf_http_server();

    ESP_LOGI(TAG, "HTTP server started successfully");
    ESP_LOGI(TAG, "Connect to the web interface to input weight measurements");

    while(1)
    {
        weight_data_t data = read_weight_data();

        if (data.val_changed)
        {
            ESP_LOGI(TAG, "Weight in Air: %.3f g", data.weight_in_air);
            ESP_LOGI(TAG, "Weight in Water: %.3f g", data.weight_in_water);

            if (data.density > 0)
            {
                ESP_LOGI(TAG, "Calculated Density: %.3f g/cm³", data.density);
            }

            reset_val_changed_weight_data();
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
