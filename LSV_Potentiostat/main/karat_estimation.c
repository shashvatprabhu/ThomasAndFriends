/**
 * @file karat_estimation.c
 * @brief Karat estimation implementation
 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "karat_estimation.h"
#include "config.h"

static const char *TAG = "KARAT_ESTIMATION";

static int classify_by_total_peak(float total_peak) {
    if (total_peak < KARAT_24_MAX) {
        return 24;
    } else if (total_peak < KARAT_22_MAX) {
        return 22;
    } else if (total_peak < KARAT_18_MAX) {
        return 18;
    } else {
        return 14;
    }
}

static float karat_to_percentage(int karat) {
    return (karat / 24.0f) * 100.0f;
}

void estimate_karat(const peak_analysis_result_t *peaks, karat_estimate_t *estimate) {
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "KARAT ESTIMATION");
    ESP_LOGI(TAG, "═══════════════════════════════════════\n");

    float total_peak = peaks->total_magnitude;
    ESP_LOGI(TAG, "Total peak magnitude: %.3f µA", total_peak);
    ESP_LOGI(TAG, "Copper peak: %.3f µA (detected: %s)",
             peaks->copper.peak_height,
             peaks->copper.detected ? "YES" : "NO");
    ESP_LOGI(TAG, "Silver peak: %.3f µA (detected: %s)\n",
             peaks->silver.peak_height,
             peaks->silver.detected ? "YES" : "NO");

    // Check for pure copper or non-gold samples
    // If ONLY copper peak detected and it's very large, it's NOT gold
    if (peaks->copper.detected && !peaks->silver.detected &&
        peaks->copper.peak_height > 5.0f) {
        estimate->estimated_karat = 0;
        estimate->gold_percentage = 0.0f;
        estimate->confidence = 85.0f;

        ESP_LOGI(TAG, "───────────────────────────────────────");
        ESP_LOGI(TAG, "⚠️  NOT GOLD DETECTED!");
        ESP_LOGI(TAG, "───────────────────────────────────────\n");
        ESP_LOGI(TAG, "Large copper oxidation peak (%.3f µA)", peaks->copper.peak_height);
        ESP_LOGI(TAG, "No silver peak detected");
        ESP_LOGI(TAG, "\nSample appears to be PURE COPPER or");
        ESP_LOGI(TAG, "copper-based alloy with NO GOLD content.\n");
        ESP_LOGI(TAG, "═══════════════════════════════════════\n");
        return;
    }

    // Classify by total peak height
    estimate->estimated_karat = classify_by_total_peak(total_peak);

    // Calculate gold percentage
    estimate->gold_percentage = karat_to_percentage(estimate->estimated_karat);

    // Calculate confidence
    estimate->confidence = 50.0f;
    if (peaks->copper.detected && peaks->copper.peak_height > CU_THRESHOLD * 2) {
        estimate->confidence += 15.0f;
    }
    if (peaks->silver.detected && peaks->silver.peak_height > AG_THRESHOLD * 2) {
        estimate->confidence += 10.0f;
    }
    if (peaks->num_peaks_detected >= 2) {
        estimate->confidence += 10.0f;
    }
    if (total_peak < 0.5f) {
        estimate->confidence -= 20.0f;
    }

    ESP_LOGI(TAG, "───────────────────────────────────────");
    ESP_LOGI(TAG, "ESTIMATION RESULTS");
    ESP_LOGI(TAG, "───────────────────────────────────────\n");

    const char *classification = "Unknown";
    if (estimate->estimated_karat == 0) {
        ESP_LOGI(TAG, "Estimated purity: NOT GOLD (0K)");
        classification = "NOT GOLD - Copper or Other Metal";
    } else {
        ESP_LOGI(TAG, "Estimated purity: %dK (%.1f%% gold)",
                 estimate->estimated_karat, estimate->gold_percentage);

        if (estimate->estimated_karat == 24) {
            classification = "Pure Gold";
        } else if (estimate->estimated_karat == 22) {
            classification = "High Purity Gold";
        } else if (estimate->estimated_karat == 18) {
            classification = "Medium Purity Gold (Jewelry Grade)";
        } else {
            classification = "Lower Purity Gold";
        }
    }

    ESP_LOGI(TAG, "Classification: %s", classification);
    ESP_LOGI(TAG, "Confidence level: %.0f%%\n", estimate->confidence);

    ESP_LOGI(TAG, "Reasoning:");
    if (estimate->estimated_karat == 0) {
        ESP_LOGI(TAG, "  Very large copper peak with no silver detected.");
        ESP_LOGI(TAG, "  This is characteristic of copper wire or");
        ESP_LOGI(TAG, "  other non-gold conductive materials.\n");
    } else if (estimate->estimated_karat == 24) {
        ESP_LOGI(TAG, "  No significant oxidation peaks detected.");
        ESP_LOGI(TAG, "  Sample appears to be pure gold (24K) with");
        ESP_LOGI(TAG, "  minimal or no alloy metals.\n");
    } else if (estimate->estimated_karat == 22) {
        ESP_LOGI(TAG, "  Small alloy peaks (%.2f µA total) indicate", total_peak);
        ESP_LOGI(TAG, "  high purity gold with ~%.1f%% alloy content,",
                 100.0f - estimate->gold_percentage);
        ESP_LOGI(TAG, "  consistent with 22K gold.\n");
    } else if (estimate->estimated_karat == 18) {
        ESP_LOGI(TAG, "  Moderate alloy peaks (%.2f µA total) indicate", total_peak);
        ESP_LOGI(TAG, "  ~%.1f%% alloy content, consistent with 18K gold.",
                 100.0f - estimate->gold_percentage);
        ESP_LOGI(TAG, "  Typical jewelry grade.\n");
    } else {
        ESP_LOGI(TAG, "  Large alloy peaks (%.2f µA total) indicate", total_peak);
        ESP_LOGI(TAG, "  ~%.1f%% alloy content, consistent with %dK gold",
                 100.0f - estimate->gold_percentage, estimate->estimated_karat);
        ESP_LOGI(TAG, "  or lower.\n");
    }

    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "⚠️  IMPORTANT NOTE:");
    ESP_LOGI(TAG, "This is an ESTIMATED purity based on typical");
    ESP_LOGI(TAG, "alloy compositions. For accurate determination,");
    ESP_LOGI(TAG, "calibrate with known gold standards.");
    ESP_LOGI(TAG, "═══════════════════════════════════════\n");
}
