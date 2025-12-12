/**
 * @file main.c
 * @brief Main application for ESP32-WROOM-32E LSV Potentiostat
 * @author Smart India Hackathon 2025 Team
 *
 * ESP-IDF Native Application (No Arduino Framework)
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "config.h"
#include "lsv_scan.h"
#include "peak_analysis.h"
#include "karat_estimation.h"
#include "lsv_http_server.h"

static const char *TAG = "MAIN";

// ═══════════════════════════════════════════════════════════════════════════
// PRE-SCAN CHECKLIST
// ═══════════════════════════════════════════════════════════════════════════

static void print_checklist(void) {
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════════════");
    ESP_LOGI(TAG, "PRE-SCAN CHECKLIST");
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════════════");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Please verify before starting scan:");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "☐ Electrodes properly positioned");
    ESP_LOGI(TAG, "  • Working Electrode (WE): Gold sample or test electrode");
    ESP_LOGI(TAG, "  • Reference Electrode (RE): Copper wire or graphite");
    ESP_LOGI(TAG, "  • Counter Electrode (CE): Copper wire or graphite");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "☐ Electrodes submerged in electrolyte");
    ESP_LOGI(TAG, "  • Depth: 2-3 cm below surface");
    ESP_LOGI(TAG, "  • Electrolyte: 0.1M KCl or 0.5M H₂SO₄");
    ESP_LOGI(TAG, "  • Volume: At least 50-100 mL");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "☐ Electrodes NOT touching each other");
    ESP_LOGI(TAG, "  • Maintain 1-2 cm spacing between electrodes");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "☐ Beaker on stable, non-conductive surface");
    ESP_LOGI(TAG, "  • Prevent spills and electrical shorts");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════════════\n");
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN APPLICATION TASK
// ═══════════════════════════════════════════════════════════════════════════

void lsv_scanning_task(void *pvParameters)
{
    int scan_counter = 0;

    // Countdown before first scan
    ESP_LOGI(TAG, "Starting first scan in 10 seconds...");
    for (int i = 10; i > 0; i--) {
        ESP_LOGI(TAG, "  %d...", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "  GO!\n");

    while (1) {
        scan_counter++;

        ESP_LOGI(TAG, "\n\n");
        ESP_LOGI(TAG, "╔═══════════════════════════════════════════════════════════╗");
        ESP_LOGI(TAG, "║  SCAN #%-3d                                               ║", scan_counter);
        ESP_LOGI(TAG, "╚═══════════════════════════════════════════════════════════╝");
        ESP_LOGI(TAG, "");

        // Step 1: Perform LSV scan
        esp_err_t ret = lsv_perform_scan();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Scan failed - skipping analysis\n");
            lsv_return_to_zero();
            vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_MS));
            continue;
        }

        // Step 2: Analyze peaks
        peak_analysis_result_t peak_results;
        analyze_peaks(&peak_results);

        // Step 3: Estimate karat
        karat_estimate_t karat_estimate;
        estimate_karat(&peak_results, &karat_estimate);

        // Step 4: Return to safe state
        lsv_return_to_zero();

        // Print completion message
        ESP_LOGI(TAG, "\n╔═══════════════════════════════════════════════════════════╗");
        ESP_LOGI(TAG, "║  SCAN COMPLETE                                            ║");
        ESP_LOGI(TAG, "╚═══════════════════════════════════════════════════════════╝");
        ESP_LOGI(TAG, "\nNext scan in %d seconds...", SCAN_INTERVAL_MS / 1000);
        ESP_LOGI(TAG, "(You may remove/replace sample between scans)\n");
        ESP_LOGI(TAG, "════════════════════════════════════════════════════════════\n\n");

        // Wait before next scan
        vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_MS));
    }
}

void app_main(void) {
    // Set log level to INFO for all components (ensures logs are visible)
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("I2C_DEVICES", ESP_LOG_INFO);
    esp_log_level_set("LSV_SCAN", ESP_LOG_INFO);
    esp_log_level_set("MAIN", ESP_LOG_INFO);

    ESP_LOGI(TAG, "\n\n");
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════════════");
    ESP_LOGI(TAG, "  ESP32 LSV POTENTIOSTAT - STARTING");
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════════════\n");

    // Initialize NVS (required for some ESP-IDF components)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize hardware
    ret = lsv_init_hardware();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Hardware initialization failed!");
        ESP_LOGE(TAG, "System halted. Check connections and reset.");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // Print pre-scan checklist
    print_checklist();

    // Start HTTP server
    ESP_LOGI(TAG, "Starting HTTP server...");
    start_lsv_http_server();

    // Start LSV scanning task
    xTaskCreate(lsv_scanning_task, "lsv_scanning_task", 4096, NULL, 5, NULL);
}
