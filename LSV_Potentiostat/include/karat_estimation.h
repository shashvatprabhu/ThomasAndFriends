/**
 * @file karat_estimation.h
 * @brief Gold Karat Estimation Module Header
 * @author Smart India Hackathon 2025 Team
 *
 * This module estimates gold purity (karat) based on oxidation peak analysis.
 * Uses peak heights to infer alloy metal content and classify gold samples.
 */

#ifndef KARAT_ESTIMATION_H
#define KARAT_ESTIMATION_H

#include <Arduino.h>
#include "config.h"
#include "peak_analysis.h"

// ═══════════════════════════════════════════════════════════════════════════
// KARAT DEFINITIONS
// ═══════════════════════════════════════════════════════════════════════════

#define KARAT_24    24  // 99.9% gold (pure)
#define KARAT_22    22  // 91.6% gold
#define KARAT_21    21  // 87.5% gold
#define KARAT_18    18  // 75.0% gold
#define KARAT_14    14  // 58.3% gold
#define KARAT_10    10  // 41.7% gold

// ═══════════════════════════════════════════════════════════════════════════
// DATA STRUCTURES
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    int estimatedKarat;         // Estimated karat (24, 22, 18, 14, etc.)
    float goldPercentage;       // Estimated gold percentage
    float confidence;           // Confidence level (0-100%)
    float alloyContent;         // Estimated total alloy percentage
    char classification[50];    // Text classification (e.g., "High purity gold")
    char reasoning[200];        // Explanation of estimation
} KaratEstimate;

// ═══════════════════════════════════════════════════════════════════════════
// FUNCTION PROTOTYPES
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Estimate gold karat from peak analysis results
 * @param peaks Pointer to peak analysis results
 * @param estimate Pointer to structure to store estimation
 * @return true if estimation successful
 */
bool estimateKarat(const PeakAnalysisResult* peaks, KaratEstimate* estimate);

/**
 * @brief Classify karat based on total peak magnitude (simple method)
 * @param totalPeakHeight Sum of all peak heights (µA)
 * @return Estimated karat value
 */
int classifyByTotalPeak(float totalPeakHeight);

/**
 * @brief Classify karat based on individual metal peaks (advanced method)
 * @param cuPeak Copper peak height (µA)
 * @param agPeak Silver peak height (µA)
 * @return Estimated karat value
 */
int classifyByMetalRatio(float cuPeak, float agPeak);

/**
 * @brief Calculate confidence level based on peak characteristics
 * @param peaks Pointer to peak analysis results
 * @return Confidence percentage (0-100)
 */
float calculateConfidence(const PeakAnalysisResult* peaks);

/**
 * @brief Print detailed karat estimation report
 * @param estimate Pointer to karat estimation results
 */
void printKaratEstimation(const KaratEstimate* estimate);

/**
 * @brief Print calibration recommendations
 */
void printCalibrationNotes();

/**
 * @brief Convert karat to gold percentage
 * @param karat Karat value (e.g., 18)
 * @return Gold percentage (e.g., 75.0 for 18K)
 */
float karatToPercentage(int karat);

/**
 * @brief Estimate alloy composition breakdown
 * @param peaks Pointer to peak analysis results
 * @param cuPercent Output: estimated copper percentage
 * @param agPercent Output: estimated silver percentage
 */
void estimateAlloyComposition(const PeakAnalysisResult* peaks,
                               float* cuPercent, float* agPercent);

#endif // KARAT_ESTIMATION_H
