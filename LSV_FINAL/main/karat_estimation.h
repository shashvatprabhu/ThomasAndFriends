/**
 * @file karat_estimation.h
 * @brief Gold karat estimation for ESP32
 */

#ifndef KARAT_ESTIMATION_H
#define KARAT_ESTIMATION_H

#include "peak_analysis.h"

typedef struct {
    int estimated_karat;
    float gold_percentage;
    float confidence;
} karat_estimate_t;

void estimate_karat(const peak_analysis_result_t *peaks, karat_estimate_t *estimate);

#endif // KARAT_ESTIMATION_H
