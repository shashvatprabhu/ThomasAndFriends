#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "config.h"
#include "lsv_scan.h"

void app_main(void) {
    nvs_flash_init();

    esp_err_t ret = lsv_init_hardware();
    if (ret != ESP_OK) {
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    vTaskDelay(pdMS_TO_TICKS(1000));

    while (1) {
        lsv_perform_scan();
        lsv_return_to_zero();
        vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_MS));
    }
}
