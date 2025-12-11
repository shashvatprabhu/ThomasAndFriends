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
    esp_err_t ret = mcp4725_set_voltage(dac_value);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set voltage %.3fV (DAC code %d): %s", 
                 voltage, dac_value, esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Voltage set to %.3fV (DAC code %d)", voltage, dac_value);
    }
    
    return ret;
}

// ═══════════════════════════════════════════════════════════════════════════
// CURRENT MEASUREMENT
// ═══════════════════════════════════════════════════════════════════════════

float lsv_read_current(void) {
    int16_t adc_diff;
    
    // Read differential: A0 - A1 (this is what we actually need)
    // Don't read A0 and A1 separately - it changes MUX and causes stale data
    esp_err_t ret = ads1115_read_differential_A0_A1(&adc_diff);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Differential read failed: %s", esp_err_to_name(ret));
        return 0.0f;
    }

    // Convert differential reading to voltage
    float v_diff = ads1115_counts_to_volts(adc_diff);

    // DEBUG: Print EVERY differential reading to see if it's updating
    static int debug_counter = 0;
    static int16_t last_diff = 0x7FFF; // Invalid value
    bool diff_changed = (adc_diff != last_diff);
    last_diff = adc_diff;
    
    if (debug_counter++ % 5 == 0 || diff_changed) {
        // Also read individual channels for debug (but don't use for calculation)
        int16_t adc_a0, adc_a1;
        float v_a0 = 0.0f, v_a1 = 0.0f;
        static float last_v_a0 = -1.0f, last_v_a1 = -1.0f;
        
        if (ads1115_read_channel(0, &adc_a0) == ESP_OK) {
            v_a0 = ads1115_counts_to_volts(adc_a0);
        }
        if (ads1115_read_channel(1, &adc_a1) == ESP_OK) {
            v_a1 = ads1115_counts_to_volts(adc_a1);
        }
        
        // Detect saturation: voltages not changing
        bool a0_saturated = (last_v_a0 >= 0.0f && fabsf(v_a0 - last_v_a0) < 0.01f);
        bool a1_saturated = (last_v_a1 >= 0.0f && fabsf(v_a1 - last_v_a1) < 0.01f);
        last_v_a0 = v_a0;
        last_v_a1 = v_a1;
        
        const char* sat_warning = "";
        if (a0_saturated && a1_saturated) {
            sat_warning = " ⚠⚠⚠ OP-AMP SATURATED! ⚠⚠⚠";
        } else if (a0_saturated) {
            sat_warning = " ⚠ A0 SATURATED";
        } else if (a1_saturated) {
            sat_warning = " ⚠ A1 SATURATED";
        }
        
        ESP_LOGI(TAG, "ADC: A0=%d (%.3fV), A1=%d (%.3fV), Diff=%d (%.3fV) %s%s", 
                 adc_a0, v_a0, adc_a1, v_a1, adc_diff, v_diff,
                 diff_changed ? "✓" : "⚠ SAME", sat_warning);
        
        // Print saturation diagnostic on first detection
        static bool saturation_warned = false;
        if ((a0_saturated || a1_saturated) && !saturation_warned) {
            saturation_warned = true;
            ESP_LOGE(TAG, "");
            ESP_LOGE(TAG, "═══════════════════════════════════════════════════════════");
            ESP_LOGE(TAG, "⚠️  OP-AMP SATURATION DETECTED!");
            ESP_LOGE(TAG, "═══════════════════════════════════════════════════════════");
            ESP_LOGE(TAG, "");
            ESP_LOGE(TAG, "A0 = %.3fV (constant) - TIA output stuck", (double)v_a0);
            ESP_LOGE(TAG, "A1 = %.3fV (constant) - Control op-amp output stuck", (double)v_a1);
            ESP_LOGE(TAG, "");
            ESP_LOGE(TAG, "TIA SATURATION (A0 = %.3fV):", (double)v_a0);
            ESP_LOGE(TAG, "  → LM358 #2 Pin 1 stuck at %.3fV", (double)v_a0);
            ESP_LOGE(TAG, "");
            ESP_LOGE(TAG, "  CHECK WITH MULTIMETER:");
            ESP_LOGE(TAG, "  1. LM358 #2 Pin 2 (virtual ground) → Should be < 0.1V");
            ESP_LOGE(TAG, "     If > 0.5V: Virtual ground not working!");
            ESP_LOGE(TAG, "");
            ESP_LOGE(TAG, "  2. LM358 #2 Pin 3 → Should be 0V (connected to GND)");
            ESP_LOGE(TAG, "");
            ESP_LOGE(TAG, "  3. Feedback resistor (40kΩ) → Between Pin 1 and Pin 2");
            ESP_LOGE(TAG, "     Measure resistance to verify");
            ESP_LOGE(TAG, "");
            ESP_LOGE(TAG, "  4. WE connection → Should connect to Pin 2");
            ESP_LOGE(TAG, "     Check continuity");
            ESP_LOGE(TAG, "");
            ESP_LOGE(TAG, "POSSIBLE CAUSES:");
            ESP_LOGE(TAG, "1. No electrochemical cell connected (most common)");
            ESP_LOGE(TAG, "2. Op-amp power not connected (Pin 8 ≠ 3.3V)");
            ESP_LOGE(TAG, "3. Feedback resistor missing/wrong (50kΩ)");
            ESP_LOGE(TAG, "4. Electrodes not connected (WE, RE, CE)");
            ESP_LOGE(TAG, "5. Short circuit or broken connections");
            ESP_LOGE(TAG, "");
            ESP_LOGE(TAG, "See OPAMP_SATURATION_DIAGNOSTIC.md for detailed troubleshooting");
            ESP_LOGE(TAG, "═══════════════════════════════════════════════════════════");
            ESP_LOGE(TAG, "");
        }
    }

    // Check for invalid/saturated readings
    if (adc_diff == -32768 || adc_diff == 32767) {
        ESP_LOGW(TAG, "WARNING: Saturated ADC reading! Diff=%d", adc_diff);
        return 0.0f; // Return 0 instead of garbage
    }

    // Convert to current (I = V / R)
    // For a transimpedance amplifier: I_WE = (Vout - Vref) / Rf
    // Differential reading is A0 - A1, which is (V_TIA - V_ref)
    float current = v_diff / R_FEEDBACK;

    // Convert to microamps
    current *= 1e6f;

    // REMOVED automatic sign inversion - let the hardware determine the sign
    // If you need to invert based on your TIA configuration, do it here:
    // current = -current;

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
