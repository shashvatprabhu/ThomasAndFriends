/**
 * @file peak_analysis.c
 * @brief Peak detection implementation
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "peak_analysis.h"
#include "lsv_scan.h"
#include "config.h"

static const char *TAG = "PEAK_ANALYSIS";

static bool find_peak_in_range(float v_min, float v_max, peak_info_t *peak_info) {
    float max_current = 0.0f;
    float max_voltage = 0.0f;
    bool peak_found = false;

    for (int i = 0; i < g_scan_data.num_points; i++) {
        float v = g_scan_data.voltage[i];
        float current = fabsf(g_scan_data.current[i]);

        if (v >= v_min && v <= v_max) {
            if (current > max_current) {
                max_current = current;
                max_voltage = v;
                peak_found = true;
            }
        }
    }

    if (peak_found && max_current > NOISE_FLOOR) {
        peak_info->detected = true;
        peak_info->peak_height = max_current;
        peak_info->peak_voltage = max_voltage;
        return true;
    }

    peak_info->detected = false;
    peak_info->peak_height = 0.0f;
    peak_info->peak_voltage = 0.0f;
    return false;
}

void analyze_peaks(peak_analysis_result_t *result) {
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "PEAK ANALYSIS");
    ESP_LOGI(TAG, "═══════════════════════════════════════\n");

    if (!g_scan_data.scan_complete || g_scan_data.num_points < 10) {
        ESP_LOGE(TAG, "Invalid scan data for analysis");
        return;
    }

    memset(result, 0, sizeof(peak_analysis_result_t));

    // Search for copper peak
    ESP_LOGI(TAG, "Scanning for copper peak (%.2fV to %.2fV):",
             CU_PEAK_V_MIN, CU_PEAK_V_MAX);
    result->copper.detected = find_peak_in_range(CU_PEAK_V_MIN, CU_PEAK_V_MAX,
                                                   &result->copper);

    if (result->copper.detected && result->copper.peak_height > CU_THRESHOLD) {
        ESP_LOGI(TAG, "✓ COPPER detected: %.3f µA peak at %.3fV",
                 result->copper.peak_height, result->copper.peak_voltage);
        if (result->copper.peak_height > 5.0f) {
            ESP_LOGI(TAG, "  → ⚠️  VERY LARGE PEAK - May be pure copper!\n");
        } else {
            ESP_LOGI(TAG, "  → Indicates copper-containing alloy\n");
        }
        result->num_peaks_detected++;
        result->total_magnitude += result->copper.peak_height;
    } else {
        ESP_LOGI(TAG, "✗ No significant copper peak detected (max: %.3f µA)",
                 result->copper.peak_height);
        ESP_LOGI(TAG, "  → Sample may be pure gold or low-Cu alloy\n");
    }

    // Search for silver peak
    ESP_LOGI(TAG, "Scanning for silver peak (%.2fV to %.2fV):",
             AG_PEAK_V_MIN, AG_PEAK_V_MAX);
    result->silver.detected = find_peak_in_range(AG_PEAK_V_MIN, AG_PEAK_V_MAX,
                                                   &result->silver);

    if (result->silver.detected && result->silver.peak_height > AG_THRESHOLD) {
        ESP_LOGI(TAG, "✓ SILVER detected: %.3f µA peak at %.3fV",
                 result->silver.peak_height, result->silver.peak_voltage);
        ESP_LOGI(TAG, "  → Indicates silver-containing alloy\n");
        result->num_peaks_detected++;
        result->total_magnitude += result->silver.peak_height;
    } else {
        ESP_LOGI(TAG, "✗ No significant silver peak detected");
        ESP_LOGI(TAG, "  → Sample may contain minimal silver\n");
    }

    ESP_LOGI(TAG, "───────────────────────────────────────");
    ESP_LOGI(TAG, "Total peaks detected: %d", result->num_peaks_detected);
    ESP_LOGI(TAG, "Combined peak magnitude: %.3f µA", result->total_magnitude);
    ESP_LOGI(TAG, "═══════════════════════════════════════\n");
}
