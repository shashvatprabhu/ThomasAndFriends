/**
 * @file lsv_scan.c
 * @brief LSV scanning implementation for ESP32
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lsv_scan.h"
#include "i2c_devices.h"
#include "config.h"

static const char *TAG = "LSV_SCAN";

// Global scan data
lsv_data_t g_scan_data = {0};

// ═══════════════════════════════════════════════════════════════════════════
// HARDWARE INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

esp_err_t lsv_init_hardware(void) {
    ESP_LOGI(TAG, "╔═══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  LSV POTENTIOSTAT - ESP32-WROOM-32E   ║");
    ESP_LOGI(TAG, "║  Gold Purity Analyzer                 ║");
    ESP_LOGI(TAG, "║  Smart India Hackathon 2025           ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════╝");

    // Initialize I2C
    ESP_LOGI(TAG, "Initializing I2C bus...");
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed!");
        return ret;
    }

    // Initialize MCP4725 DAC
    ESP_LOGI(TAG, "Initializing MCP4725 DAC...");
    ret = mcp4725_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MCP4725 initialization failed!");
        return ret;
    }

    // Initialize ADS1115 ADC
    ESP_LOGI(TAG, "Initializing ADS1115 ADC...");
    ret = ads1115_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADS1115 initialization failed!");
        return ret;
    }

    // Set DAC to 0V (safe state)
    lsv_set_voltage(0.0f);
    ESP_LOGI(TAG, "DAC set to 0.000V (safe state)");

    // Clear scan data
    lsv_clear_data();

    ESP_LOGI(TAG, "System ready!");
    return ESP_OK;
}

// ═══════════════════════════════════════════════════════════════════════════
// VOLTAGE CONTROL
// ═══════════════════════════════════════════════════════════════════════════

esp_err_t lsv_set_voltage(float voltage) {
    if (voltage < 0.0f || voltage > DAC_VREF) {
        ESP_LOGW(TAG, "Voltage %.3fV out of range [0, %.1fV]", voltage, DAC_VREF);
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t dac_value = (uint16_t)((voltage / DAC_VREF) * DAC_RESOLUTION);
    return mcp4725_set_voltage(dac_value);
}

// ═══════════════════════════════════════════════════════════════════════════
// CURRENT MEASUREMENT
// ═══════════════════════════════════════════════════════════════════════════

float lsv_read_current(void) {
    int16_t adc_value;
    esp_err_t ret = ads1115_read_single(&adc_value);

    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ADC read failed");
        return 0.0f;
    }

    // Convert to voltage
    float voltage = ads1115_counts_to_volts(adc_value);

    // Convert to current (I = V / R)
    float current = voltage / R_FEEDBACK;

    // Convert to microamps
    current *= 1e6f;

    // Invert sign (inverting amplifier)
    current = -current;

    return current;
}

// ═══════════════════════════════════════════════════════════════════════════
// LSV SCAN
// ═══════════════════════════════════════════════════════════════════════════

esp_err_t lsv_perform_scan(void) {
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "Starting LSV Scan");
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "Voltage range: %.3fV to %.3fV", START_VOLTAGE, END_VOLTAGE);
    ESP_LOGI(TAG, "Step size: %.3fV", STEP_SIZE);
    ESP_LOGI(TAG, "Settle time: %dms per step", SETTLE_TIME_MS);

    int num_points = (int)((END_VOLTAGE - START_VOLTAGE) / STEP_SIZE) + 1;

    if (num_points > MAX_DATA_POINTS) {
        ESP_LOGE(TAG, "Too many data points (%d > %d)", num_points, MAX_DATA_POINTS);
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(TAG, "Total data points: %d", num_points);
    ESP_LOGI(TAG, "Estimated scan time: %.1f seconds\n",
             (num_points * SETTLE_TIME_MS) / 1000.0f);

    // Clear previous data
    lsv_clear_data();

    // Print header
    ESP_LOGI(TAG, "Voltage (V) | Current (µA) | Notes");
    ESP_LOGI(TAG, "────────────┼──────────────┼─────────────");

    // Perform voltage sweep
    int data_idx = 0;
    float max_current = 0.0f;
    float max_current_voltage = 0.0f;

    for (float voltage = START_VOLTAGE;
         voltage <= END_VOLTAGE && data_idx < MAX_DATA_POINTS;
         voltage += STEP_SIZE) {

        // Set DAC voltage
        esp_err_t ret = lsv_set_voltage(voltage);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set voltage");
            break;
        }

        // Wait for equilibration
        vTaskDelay(pdMS_TO_TICKS(SETTLE_TIME_MS));

        // Read current
        float current = lsv_read_current();

        // Track maximum current
        float abs_current = fabsf(current);
        if (abs_current > max_current) {
            max_current = abs_current;
            max_current_voltage = voltage;
        }

        // Store data
        g_scan_data.voltage[data_idx] = voltage;
        g_scan_data.current[data_idx] = current;
        data_idx++;

        // Print ALL data points for debugging
        char note[30] = "";
        if (abs_current > 3.0f) {
            sprintf(note, "▲▲ LARGE PEAK");
        } else if (abs_current > 1.0f) {
            sprintf(note, "▲ Peak");
        } else if (abs_current > 0.5f) {
            sprintf(note, "Rising");
        }
        ESP_LOGI(TAG, "   %5.3f    |   %7.3f    | %s", voltage, current, note);
    }

    ESP_LOGI(TAG, "────────────────────────────────────────");
    ESP_LOGI(TAG, "MAX CURRENT: %.3f µA at %.3fV", max_current, max_current_voltage);

    g_scan_data.num_points = data_idx;
    g_scan_data.scan_complete = true;

    // Return to safe state
    lsv_return_to_zero();

    ESP_LOGI(TAG, "────────────────────────────────────────");
    ESP_LOGI(TAG, "Scan complete - %d data points collected", data_idx);
    ESP_LOGI(TAG, "════════════════════════════════════════\n");

    return ESP_OK;
}

// ═══════════════════════════════════════════════════════════════════════════
// DATA MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

void lsv_clear_data(void) {
    memset(&g_scan_data, 0, sizeof(lsv_data_t));
}

void lsv_return_to_zero(void) {
    lsv_set_voltage(0.0f);
    vTaskDelay(pdMS_TO_TICKS(100));
}
