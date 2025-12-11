/**
 * @file main.c
 * @brief Eddy-current measurement: report induced voltage/current
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "config.h"
#include "ads1115.h"

static const char *TAG = "EDDY_CURRENT";

// ═══════════════════════════════════════════════════════════════════════════
// MEASUREMENT FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Measure voltage at ADS1115 input by averaging samples
 * @param num_samples Number of samples to average
 * @return Averaged voltage in volts (ADS input)
 */
static float measure_voltage(int num_samples) {
    int64_t sum = 0;

    for (int i = 0; i < num_samples; i++) {
        int16_t adc_value;
        esp_err_t ret = ads1115_read_single(0, &adc_value);  // Read A0

        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "ADC read failed");
            continue;
        }

        sum += adc_value;
        vTaskDelay(pdMS_TO_TICKS(1));  // Small delay between samples
    }

    float avg_counts = (float)sum / num_samples;
    float voltage = ads1115_counts_to_volts((int16_t)avg_counts, ADS1115_PGA_4_096V);

    return voltage;
}

/**
 * @brief Measure peak-to-peak noise at ADS input
 * @return Noise in volts (ADS input)
 */
static float measure_noise(void) {
    float min_v = 999.0f;
    float max_v = -999.0f;

    for (int i = 0; i < 30; i++) {
        int16_t adc_value;
        esp_err_t ret = ads1115_read_single(0, &adc_value);

        if (ret == ESP_OK) {
            float v = ads1115_counts_to_volts(adc_value, ADS1115_PGA_4_096V);

            if (v < min_v) min_v = v;
            if (v > max_v) max_v = v;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    return (max_v - min_v);  // Peak-to-peak
}

// ═══════════════════════════════════════════════════════════════════════════
// DISPLAY HELPERS
// ═══════════════════════════════════════════════════════════════════════════

static void display_results(float v_adc, float v_input, float current_a, float noise) {
    printf("ADC: %.3fV | Input: %.3fV | I: %.3fmA | Noise: %.1fmV\n",
           v_adc, v_input, current_a * 1000.0f, noise * 1000.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN APPLICATION
// ═══════════════════════════════════════════════════════════════════════════

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Print header
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   Eddy Current Monitor                ║");
    ESP_LOGI(TAG, "║   90-turn coil, 0.7mm wire            ║");
    ESP_LOGI(TAG, "║   ESP32-WROOM-32E + ADS1115           ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    // Initialize ADS1115
    ret = ads1115_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADS1115 initialization failed!");
        ESP_LOGE(TAG, "Check connections:");
        ESP_LOGE(TAG, "  VCC → 5V");
        ESP_LOGE(TAG, "  GND → GND");
        ESP_LOGE(TAG, "  SCL → GPIO22");
        ESP_LOGE(TAG, "  SDA → GPIO21");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // Quick notice about measurement chain
    ESP_LOGI(TAG, "Measuring divider node (R1=%dΩ, R2=%dΩ, sense resistor=%dΩ)",
             R1_OHMS, R2_OHMS, CURRENT_RESISTOR_OHMS);

    // Main measurement loop
    while (1) {
        // Take measurement
        float v_adc = measure_voltage(SAMPLES);  // Divider midpoint
        float noise = measure_noise();

        // Reconstruct pre-divider voltage and coil current.
        float v_input = v_adc * ((float)(R1_OHMS + R2_OHMS) / (float)R2_OHMS);
        float current_a = v_input / (float)CURRENT_RESISTOR_OHMS;

        // Display results
        display_results(v_adc, v_input, current_a, noise);

        // Wait before next reading
        vTaskDelay(pdMS_TO_TICKS(DELAY_BETWEEN_READINGS_MS));
    }
}
