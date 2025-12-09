/**
 * @file peak_analysis.h
 * @brief Peak detection for ESP32
 */

#ifndef PEAK_ANALYSIS_H
#define PEAK_ANALYSIS_H

#include <stdbool.h>

typedef struct {
    bool detected;
    float peak_height;
    float peak_voltage;
} peak_info_t;

typedef struct {
    peak_info_t copper;
    peak_info_t silver;
    float total_magnitude;
    int num_peaks_detected;
} peak_analysis_result_t;

void analyze_peaks(peak_analysis_result_t *result);

#endif // PEAK_ANALYSIS_H
