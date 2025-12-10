/**
 * @file main.c
 * @brief Eddy Current Gold Tester for ESP32-WROOM-32E
 * @author Smart India Hackathon 2025 Team
 *
 * Circuit:
 * - NE555 oscillator (~100kHz) driving 90-turn coil
 * - LM358 op-amp buffer/amplifier
 * - Voltage divider (10kΩ + 10kΩ) sensing coil impedance
 * - ADS1115 16-bit ADC reading voltage
 * - ESP32 processing and display
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "config.h"
#include "ads1115.h"

static const char *TAG = "GOLD_TESTER";

// ═══════════════════════════════════════════════════════════════════════════
// GLOBAL VARIABLES
// ═══════════════════════════════════════════════════════════════════════════

// Calibration data (UPDATE THESE WITH YOUR MEASUREMENTS!)
static gold_calibration_t calibration[NUM_CALIBRATION_POINTS] = {
    {2.450f, 0},            // No metal (baseline)
    {2.200f, 14},           // 14K gold
    {2.000f, 18},           // 18K gold
    {1.850f, 22},           // 22K gold
    {1.700f, 24}            // 24K gold (pure)
};

static float baseline_voltage = 0.0f;

// ═══════════════════════════════════════════════════════════════════════════
// MEASUREMENT FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Measure voltage by averaging multiple samples
 * @param num_samples Number of samples to average
 * @return Averaged voltage in volts
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
 * @brief Measure noise (peak-to-peak variation)
 * @return Noise in volts
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
// KARAT ESTIMATION
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Estimate gold karat using linear interpolation
 * @param voltage Measured voltage
 * @return Estimated karat (0 = no metal, 14-24 = gold purity)
 */
static int estimate_karat(float voltage) {
    // Check if close to baseline (no metal)
    if (fabsf(voltage - baseline_voltage) < NO_METAL_THRESHOLD_V) {
        return 0;  // No metal detected
    }

    // Find calibration points bracketing this voltage
    for (int i = 0; i < NUM_CALIBRATION_POINTS - 1; i++) {
        float v1 = calibration[i].voltage;
        float v2 = calibration[i + 1].voltage;
        int k1 = calibration[i].karat;
        int k2 = calibration[i + 1].karat;

        // Check if voltage between these points
        if ((voltage <= v1 && voltage >= v2) ||
            (voltage >= v1 && voltage <= v2)) {

            // Linear interpolation
            float fraction = (voltage - v1) / (v2 - v1);
            int karat = (int)(k1 + fraction * (k2 - k1));

            return karat;
        }
    }

    // Outside calibration range
    if (voltage < calibration[NUM_CALIBRATION_POINTS - 1].voltage) {
        return 24;  // Very high purity
    } else {
        return 0;   // Very low or no metal
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// DISPLAY FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Display measurement results
 * @param voltage Measured voltage (V)
 * @param delta_v Change from baseline (V)
 * @param karat Estimated karat
 * @param noise Measured noise (V)
 */
static void display_results(float voltage, float delta_v, int karat, float noise) {
    // Voltage
    printf("V: %.3fV | ", voltage);

    // Change from baseline (in mV)
    printf("ΔV: %6.1fmV | ", delta_v * 1000.0f);

    // Noise level (in mV)
    printf("Noise: %5.1fmV | ", noise * 1000.0f);

    // Interpretation
    if (karat == 0) {
        printf("NO METAL");
    } else {
        printf("%dK Gold", karat);

        // Confidence indicator
        float noise_mv = noise * 1000.0f;
        if (noise_mv < NOISE_GOOD_MV) {
            printf(" ✓✓✓");          // High confidence
        } else if (noise_mv < NOISE_ACCEPTABLE_MV) {
            printf(" ✓✓");            // Medium confidence
        } else {
            printf(" ✓");             // Low confidence
        }
    }

    printf("\n");
}

// ═══════════════════════════════════════════════════════════════════════════
// CALIBRATION DISPLAY
// ═══════════════════════════════════════════════════════════════════════════

static void display_calibration(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Current calibration data:");
    ESP_LOGI(TAG, "Karat | Voltage");
    ESP_LOGI(TAG, "──────┼─────────");

    for (int i = 0; i < NUM_CALIBRATION_POINTS; i++) {
        if (calibration[i].karat == 0) {
            ESP_LOGI(TAG, " None | %.3fV (baseline)", calibration[i].voltage);
        } else {
            ESP_LOGI(TAG, "  %2dK | %.3fV", calibration[i].karat, calibration[i].voltage);
        }
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "⚠️  UPDATE config.h with YOUR measured values!");
    ESP_LOGI(TAG, "");
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
    ESP_LOGI(TAG, "║   GOLD TESTER - Eddy Current Method   ║");
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

    // Display calibration data
    display_calibration();

    // Measure baseline (no metal near coil)
    ESP_LOGI(TAG, "Measuring baseline...");
    ESP_LOGI(TAG, "(Keep metal >30cm away from coil)");
    vTaskDelay(pdMS_TO_TICKS(3000));  // Give user time

    baseline_voltage = measure_voltage(BASELINE_SAMPLES);

    ESP_LOGI(TAG, "✓ Baseline: %.3fV", baseline_voltage);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "Ready! Bring gold near coil...");
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "");

    // Main measurement loop
    while (1) {
        // Take measurement
        float voltage = measure_voltage(SAMPLES);
        float delta_v = baseline_voltage - voltage;
        float noise = measure_noise();

        // Estimate karat
        int karat = estimate_karat(voltage);

        // Display results
        display_results(voltage, delta_v, karat, noise);

        // Wait before next reading
        vTaskDelay(pdMS_TO_TICKS(DELAY_BETWEEN_READINGS_MS));
    }
}
