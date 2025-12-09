/**
 * @file karat_estimation.cpp
 * @brief Gold Karat Estimation Module Implementation
 * @author Smart India Hackathon 2025 Team
 */

#include "karat_estimation.h"

// ═══════════════════════════════════════════════════════════════════════════
// MAIN KARAT ESTIMATION FUNCTION
// ═══════════════════════════════════════════════════════════════════════════

bool estimateKarat(const PeakAnalysisResult* peaks, KaratEstimate* estimate) {
    Serial.println("═══════════════════════════════════════");
    Serial.println("KARAT ESTIMATION");
    Serial.println("═══════════════════════════════════════\n");

    // Initialize estimate structure
    estimate->estimatedKarat = 24;
    estimate->goldPercentage = 99.9;
    estimate->confidence = 0.0;
    estimate->alloyContent = 0.0;

    // Get total peak magnitude
    float totalPeak = peaks->totalMagnitude;

    Serial.printf("Total peak magnitude: %.3f µA\n\n", totalPeak);

    // Method 1: Simple classification by total peak height
    int karat1 = classifyByTotalPeak(totalPeak);

    // Method 2: Advanced classification by metal ratio
    float cuPeak = peaks->copper.detected ? peaks->copper.peakHeight : 0.0;
    float agPeak = peaks->silver.detected ? peaks->silver.peakHeight : 0.0;
    int karat2 = classifyByMetalRatio(cuPeak, agPeak);

    // Average the two methods for final estimate
    estimate->estimatedKarat = (karat1 + karat2) / 2;

    // Round to nearest standard karat
    if (estimate->estimatedKarat >= 23) {
        estimate->estimatedKarat = 24;
    } else if (estimate->estimatedKarat >= 20) {
        estimate->estimatedKarat = 22;
    } else if (estimate->estimatedKarat >= 19) {
        estimate->estimatedKarat = 21;
    } else if (estimate->estimatedKarat >= 16) {
        estimate->estimatedKarat = 18;
    } else if (estimate->estimatedKarat >= 12) {
        estimate->estimatedKarat = 14;
    } else {
        estimate->estimatedKarat = 10;
    }

    // Calculate gold percentage
    estimate->goldPercentage = karatToPercentage(estimate->estimatedKarat);
    estimate->alloyContent = 100.0 - estimate->goldPercentage;

    // Calculate confidence level
    estimate->confidence = calculateConfidence(peaks);

    // Generate classification text
    if (estimate->estimatedKarat == 24) {
        sprintf(estimate->classification, "Pure Gold");
        sprintf(estimate->reasoning,
                "No significant oxidation peaks detected. Sample appears to be "
                "pure gold (24K) with minimal or no alloy metals.");
    } else if (estimate->estimatedKarat == 22) {
        sprintf(estimate->classification, "High Purity Gold");
        sprintf(estimate->reasoning,
                "Small alloy peaks (%.2f µA total) indicate high purity gold "
                "with ~%.1f%% alloy content, consistent with 22K gold.",
                totalPeak, estimate->alloyContent);
    } else if (estimate->estimatedKarat == 18) {
        sprintf(estimate->classification, "Medium Purity Gold");
        sprintf(estimate->reasoning,
                "Moderate alloy peaks (%.2f µA total) indicate ~%.1f%% alloy "
                "content, consistent with 18K gold. Typical jewelry grade.",
                totalPeak, estimate->alloyContent);
    } else {
        sprintf(estimate->classification, "Lower Purity Gold");
        sprintf(estimate->reasoning,
                "Large alloy peaks (%.2f µA total) indicate ~%.1f%% alloy "
                "content, consistent with %dK gold or lower.",
                totalPeak, estimate->alloyContent, estimate->estimatedKarat);
    }

    // Print results
    printKaratEstimation(estimate);

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// CLASSIFICATION BY TOTAL PEAK HEIGHT
// ═══════════════════════════════════════════════════════════════════════════

int classifyByTotalPeak(float totalPeakHeight) {
    /*
     * This is a simple threshold-based classification.
     * These thresholds should be calibrated with known gold standards!
     *
     * Default thresholds (adjust based on your setup):
     * - 24K: <0.5 µA (pure gold, no alloy peaks)
     * - 22K: 0.5-2.0 µA (91.6% Au, small alloy content)
     * - 18K: 2.0-5.0 µA (75% Au, medium alloy content)
     * - 14K: >5.0 µA (58.3% Au, large alloy content)
     */

    if (totalPeakHeight < KARAT_24_MAX) {
        return 24; // Pure gold
    } else if (totalPeakHeight < KARAT_22_MAX) {
        return 22; // High purity
    } else if (totalPeakHeight < KARAT_18_MAX) {
        return 18; // Medium purity
    } else {
        return 14; // Lower purity
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// CLASSIFICATION BY METAL RATIO (Advanced)
// ═══════════════════════════════════════════════════════════════════════════

int classifyByMetalRatio(float cuPeak, float agPeak) {
    /*
     * Different gold alloys have characteristic Cu:Ag ratios:
     * - Yellow gold: High Cu, low Ag (Cu:Ag ratio ~2:1 to 3:1)
     * - White gold: Low Cu, high Ni (different peak pattern)
     * - Rose gold: Very high Cu (Cu peak dominates)
     * - Green gold: High Ag (Ag peak larger than Cu)
     *
     * This method uses both peak heights and their ratio for classification.
     */

    float totalPeak = cuPeak + agPeak;

    // If no peaks, assume pure gold
    if (totalPeak < 0.3) {
        return 24;
    }

    // Calculate Cu:Ag ratio
    float ratio = (agPeak > 0.1) ? (cuPeak / agPeak) : 10.0;

    // Typical yellow gold (most common):
    // 22K: Small peaks, Cu:Ag ~2:1, total <2µA
    // 18K: Medium peaks, Cu:Ag ~2:1, total 2-5µA
    // 14K: Large peaks, Cu:Ag ~2:1, total >5µA

    if (totalPeak < 1.5) {
        return 22; // Small peaks = high purity
    } else if (totalPeak < 4.0) {
        return 18; // Medium peaks = 18K
    } else if (totalPeak < 7.0) {
        return 14; // Large peaks = 14K
    } else {
        return 10; // Very large peaks = 10K or lower
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// CONFIDENCE CALCULATION
// ═══════════════════════════════════════════════════════════════════════════

float calculateConfidence(const PeakAnalysisResult* peaks) {
    float confidence = 50.0; // Start with medium confidence

    // Increase confidence if clear, well-defined peaks
    if (peaks->copper.detected && peaks->copper.peakHeight > CU_THRESHOLD * 2) {
        confidence += 15.0; // Strong Cu peak increases confidence
    }

    if (peaks->silver.detected && peaks->silver.peakHeight > AG_THRESHOLD * 2) {
        confidence += 10.0; // Strong Ag peak increases confidence
    }

    // Increase confidence if multiple peaks detected (more data points)
    if (peaks->numPeaksDetected >= 2) {
        confidence += 10.0;
    }

    // Decrease confidence if peaks are weak/ambiguous
    if (peaks->totalMagnitude < 0.5) {
        confidence -= 20.0; // Very small peaks = low confidence
    }

    // Decrease confidence if only at noise floor
    if (peaks->totalMagnitude < NOISE_FLOOR * 2) {
        confidence -= 30.0;
    }

    // Cap confidence at reasonable limits
    if (confidence > 85.0) confidence = 85.0; // Never 100% without calibration
    if (confidence < 10.0) confidence = 10.0; // Always some uncertainty

    return confidence;
}

// ═══════════════════════════════════════════════════════════════════════════
// ALLOY COMPOSITION ESTIMATION
// ═══════════════════════════════════════════════════════════════════════════

void estimateAlloyComposition(const PeakAnalysisResult* peaks,
                               float* cuPercent, float* agPercent) {
    /*
     * Very rough estimation of alloy percentages based on peak heights.
     * This requires proper calibration with known standards!
     *
     * Approximate calibration factors (adjust experimentally):
     * - 1 µA Cu peak ≈ 2-3% copper in alloy
     * - 1 µA Ag peak ≈ 2-3% silver in alloy
     */

    const float CU_CALIBRATION_FACTOR = 2.5; // µA per % copper
    const float AG_CALIBRATION_FACTOR = 2.8; // µA per % silver

    float cuPeak = peaks->copper.detected ? peaks->copper.peakHeight : 0.0;
    float agPeak = peaks->silver.detected ? peaks->silver.peakHeight : 0.0;

    *cuPercent = cuPeak / CU_CALIBRATION_FACTOR;
    *agPercent = agPeak / AG_CALIBRATION_FACTOR;

    // Cap at reasonable limits
    if (*cuPercent > 40.0) *cuPercent = 40.0;
    if (*agPercent > 25.0) *agPercent = 25.0;
}

// ═══════════════════════════════════════════════════════════════════════════
// UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

float karatToPercentage(int karat) {
    // Karat = (purity / 24) × 100
    // Therefore: purity = (karat / 24) × 100
    return (karat / 24.0) * 100.0;
}

// ═══════════════════════════════════════════════════════════════════════════
// OUTPUT FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

void printKaratEstimation(const KaratEstimate* estimate) {
    Serial.println("───────────────────────────────────────");
    Serial.println("ESTIMATION RESULTS");
    Serial.println("───────────────────────────────────────\n");

    Serial.printf("Estimated purity: %dK (%.1f%% gold)\n",
                  estimate->estimatedKarat, estimate->goldPercentage);
    Serial.printf("Classification: %s\n", estimate->classification);
    Serial.printf("Confidence level: %.0f%%\n\n", estimate->confidence);

    Serial.println("Reasoning:");
    Serial.printf("  %s\n\n", estimate->reasoning);

    // If significant alloy content, estimate composition
    if (estimate->alloyContent > 5.0) {
        Serial.println("Estimated alloy composition:");
        Serial.printf("  Gold:  %.1f%%\n", estimate->goldPercentage);
        Serial.printf("  Alloy: %.1f%% (Cu, Ag, other metals)\n\n",
                      estimate->alloyContent);
    }

    Serial.println("═══════════════════════════════════════\n");

    printCalibrationNotes();
}

void printCalibrationNotes() {
    Serial.println("⚠️  IMPORTANT CALIBRATION NOTE");
    Serial.println("───────────────────────────────────────");
    Serial.println("This is an ESTIMATED purity based on typical");
    Serial.println("alloy compositions and default thresholds.\n");

    Serial.println("For ACCURATE karat determination:");
    Serial.println("1. Calibrate with known gold standards");
    Serial.println("   • Test 14K, 18K, 22K, and 24K samples");
    Serial.println("   • Record peak heights for each");
    Serial.println("   • Update threshold values in config.h\n");

    Serial.println("2. Use consistent testing conditions");
    Serial.println("   • Same electrolyte concentration");
    Serial.println("   • Same electrode positioning");
    Serial.println("   • Same sample surface area\n");

    Serial.println("3. Run multiple scans and average");
    Serial.println("   • Improves accuracy and precision");
    Serial.println("   • Reduces measurement noise\n");

    Serial.println("4. Consider professional verification");
    Serial.println("   • X-ray fluorescence (XRF)");
    Serial.println("   • Fire assay for high-value samples");
    Serial.println("═══════════════════════════════════════\n");
}
