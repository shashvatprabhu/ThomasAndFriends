/**
 * @file peak_analysis.cpp
 * @brief Peak Detection and Analysis Module Implementation
 * @author Smart India Hackathon 2025 Team
 */

#include "peak_analysis.h"

// ═══════════════════════════════════════════════════════════════════════════
// MAIN PEAK ANALYSIS FUNCTION
// ═══════════════════════════════════════════════════════════════════════════

bool analyzePeaks(PeakAnalysisResult* result) {
    Serial.println("═══════════════════════════════════════");
    Serial.println("PEAK ANALYSIS");
    Serial.println("═══════════════════════════════════════\n");

    // Check if scan data is valid
    if (!scanData.scanComplete || scanData.numPoints < 10) {
        Serial.println("❌ ERROR: Invalid scan data for analysis");
        return false;
    }

    // Apply smoothing filter if enabled
    if (ENABLE_SMOOTHING) {
        Serial.println("Applying smoothing filter...");
        applySmoothingFilter(5); // 5-point moving average
    }

    // Initialize result structure
    result->numPeaksDetected = 0;
    result->totalMagnitude = 0.0;

    // Search for copper peak (0.0V to 0.25V)
    Serial.printf("Scanning for copper peak (%.2fV to %.2fV):\n",
                  CU_PEAK_V_MIN, CU_PEAK_V_MAX);
    result->copper.detected = findPeakInRange(CU_PEAK_V_MIN, CU_PEAK_V_MAX,
                                               &result->copper);

    if (result->copper.detected && result->copper.peakHeight > CU_THRESHOLD) {
        Serial.printf("✓ COPPER detected: %.3f µA peak at %.3fV\n",
                      result->copper.peakHeight, result->copper.peakVoltage);
        Serial.println("  → Indicates copper-containing alloy");
        result->numPeaksDetected++;
        result->totalMagnitude += result->copper.peakHeight;
    } else {
        Serial.println("✗ No significant copper peak detected");
        Serial.println("  → Sample may be pure gold or low-Cu alloy");
    }
    Serial.println();

    // Search for silver peak (0.3V to 0.55V)
    Serial.printf("Scanning for silver peak (%.2fV to %.2fV):\n",
                  AG_PEAK_V_MIN, AG_PEAK_V_MAX);
    result->silver.detected = findPeakInRange(AG_PEAK_V_MIN, AG_PEAK_V_MAX,
                                               &result->silver);

    if (result->silver.detected && result->silver.peakHeight > AG_THRESHOLD) {
        Serial.printf("✓ SILVER detected: %.3f µA peak at %.3fV\n",
                      result->silver.peakHeight, result->silver.peakVoltage);
        Serial.println("  → Indicates silver-containing alloy");
        result->numPeaksDetected++;
        result->totalMagnitude += result->silver.peakHeight;
    } else {
        Serial.println("✗ No significant silver peak detected");
        Serial.println("  → Sample may contain minimal silver");
    }
    Serial.println();

    // Optional: Search for nickel and zinc (if extended voltage range)
    if (START_VOLTAGE <= NI_PEAK_V_MIN) {
        Serial.printf("Scanning for nickel peak (%.2fV to %.2fV):\n",
                      NI_PEAK_V_MIN, NI_PEAK_V_MAX);
        result->nickel.detected = findPeakInRange(NI_PEAK_V_MIN, NI_PEAK_V_MAX,
                                                   &result->nickel);
        if (result->nickel.detected && result->nickel.peakHeight > 0.3) {
            Serial.printf("✓ NICKEL detected: %.3f µA peak at %.3fV\n",
                          result->nickel.peakHeight, result->nickel.peakVoltage);
            result->numPeaksDetected++;
            result->totalMagnitude += result->nickel.peakHeight;
        } else {
            Serial.println("✗ No significant nickel peak detected");
        }
        Serial.println();
    }

    if (START_VOLTAGE <= ZN_PEAK_V_MIN) {
        Serial.printf("Scanning for zinc peak (%.2fV to %.2fV):\n",
                      ZN_PEAK_V_MIN, ZN_PEAK_V_MAX);
        result->zinc.detected = findPeakInRange(ZN_PEAK_V_MIN, ZN_PEAK_V_MAX,
                                                 &result->zinc);
        if (result->zinc.detected && result->zinc.peakHeight > 0.3) {
            Serial.printf("✓ ZINC detected: %.3f µA peak at %.3fV\n",
                          result->zinc.peakHeight, result->zinc.peakVoltage);
            result->numPeaksDetected++;
            result->totalMagnitude += result->zinc.peakHeight;
        } else {
            Serial.println("✗ No significant zinc peak detected");
        }
        Serial.println();
    }

    // Print summary
    Serial.println("───────────────────────────────────────");
    Serial.printf("Total peaks detected: %d\n", result->numPeaksDetected);
    Serial.printf("Combined peak magnitude: %.3f µA\n", result->totalMagnitude);
    Serial.println("═══════════════════════════════════════\n");

    // Print interpretation
    printInterpretation(result);

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// PEAK DETECTION IN VOLTAGE RANGE
// ═══════════════════════════════════════════════════════════════════════════

bool findPeakInRange(float vMin, float vMax, PeakInfo* peakInfo) {
    float maxCurrent = 0.0;
    float maxVoltage = 0.0;
    bool peakFound = false;

    // Initialize peak info
    peakInfo->detected = false;
    peakInfo->peakHeight = 0.0;
    peakInfo->peakVoltage = 0.0;
    peakInfo->peakArea = 0.0;

    // Search through data points
    for (int i = 0; i < scanData.numPoints; i++) {
        float v = scanData.voltage[i];
        float current = abs(scanData.current[i]); // Use absolute value

        // Check if within voltage window
        if (v >= vMin && v <= vMax) {
            if (current > maxCurrent) {
                maxCurrent = current;
                maxVoltage = v;
                peakFound = true;
            }
        }
    }

    // Store results if peak exceeds noise floor
    if (peakFound && maxCurrent > NOISE_FLOOR) {
        peakInfo->detected = true;
        peakInfo->peakHeight = maxCurrent;
        peakInfo->peakVoltage = maxVoltage;

        // Calculate peak area (simple trapezoidal integration)
        peakInfo->peakArea = calculatePeakArea(vMin, vMax);
    }

    return peakInfo->detected;
}

// ═══════════════════════════════════════════════════════════════════════════
// PEAK AREA CALCULATION (for quantitative analysis)
// ═══════════════════════════════════════════════════════════════════════════

float calculatePeakArea(float vMin, float vMax) {
    float area = 0.0;
    float prevV = 0.0;
    float prevI = 0.0;
    bool started = false;

    for (int i = 0; i < scanData.numPoints; i++) {
        float v = scanData.voltage[i];
        float current = abs(scanData.current[i]);

        if (v >= vMin && v <= vMax) {
            if (started) {
                // Trapezoidal rule: Area = (I1 + I2) / 2 × ΔV
                float deltaV = v - prevV;
                area += (current + prevI) / 2.0 * deltaV;
            }
            prevV = v;
            prevI = current;
            started = true;
        }
    }

    return area;
}

// ═══════════════════════════════════════════════════════════════════════════
// SMOOTHING FILTER (Moving Average)
// ═══════════════════════════════════════════════════════════════════════════

void applySmoothingFilter(int windowSize) {
    // Validate window size (must be odd)
    if (windowSize % 2 == 0) windowSize++;
    if (windowSize < 3) windowSize = 3;
    if (windowSize > 9) windowSize = 9;

    int halfWindow = windowSize / 2;

    // Create temporary array for smoothed data
    float smoothed[MAX_DATA_POINTS];

    // Apply moving average filter
    for (int i = 0; i < scanData.numPoints; i++) {
        float sum = 0.0;
        int count = 0;

        // Average over window
        for (int j = -halfWindow; j <= halfWindow; j++) {
            int idx = i + j;
            if (idx >= 0 && idx < scanData.numPoints) {
                sum += scanData.current[idx];
                count++;
            }
        }

        smoothed[i] = sum / count;
    }

    // Copy smoothed data back to original array
    for (int i = 0; i < scanData.numPoints; i++) {
        scanData.current[i] = smoothed[i];
    }

    Serial.printf("✓ Applied %d-point moving average filter\n\n", windowSize);
}

// ═══════════════════════════════════════════════════════════════════════════
// INTERPRETATION AND REPORTING
// ═══════════════════════════════════════════════════════════════════════════

void printInterpretation(const PeakAnalysisResult* result) {
    Serial.println("═══════════════════════════════════════");
    Serial.println("ELECTROCHEMICAL INTERPRETATION");
    Serial.println("═══════════════════════════════════════\n");

    if (result->numPeaksDetected == 0) {
        Serial.println("✨ No oxidation peaks detected");
        Serial.println("\nPossible interpretations:");
        Serial.println("1. Sample is PURE GOLD (24K)");
        Serial.println("   → Gold oxidizes at ~1.5V (outside scan range)");
        Serial.println("   → No alloy metals present to oxidize");
        Serial.println("\n2. Electrodes not properly positioned");
        Serial.println("   → Check that WE is in contact with sample");
        Serial.println("   → Ensure all electrodes submerged in electrolyte");
        Serial.println("\n3. Insufficient electrolyte conductivity");
        Serial.println("   → Use fresh 0.1M KCl or 0.5M H₂SO₄");
    } else {
        Serial.println("Detected alloy composition:\n");

        // Copper interpretation
        if (result->copper.detected && result->copper.peakHeight > CU_THRESHOLD) {
            float cuPercent = (result->copper.peakHeight / result->totalMagnitude) * 100;
            Serial.printf("• Copper: %.1f%% of total peak signal\n", cuPercent);
            Serial.println("  Reaction: Cu → Cu²⁺ + 2e⁻");
            Serial.printf("  Peak at %.3fV indicates Cu oxidation\n",
                          result->copper.peakVoltage);

            // Estimate copper content (rough approximation)
            if (result->copper.peakHeight > 8.0) {
                Serial.println("  → HIGH copper content (~20-30% alloy)");
            } else if (result->copper.peakHeight > 3.0) {
                Serial.println("  → MEDIUM copper content (~10-20% alloy)");
            } else {
                Serial.println("  → LOW copper content (~5-10% alloy)");
            }
            Serial.println();
        }

        // Silver interpretation
        if (result->silver.detected && result->silver.peakHeight > AG_THRESHOLD) {
            float agPercent = (result->silver.peakHeight / result->totalMagnitude) * 100;
            Serial.printf("• Silver: %.1f%% of total peak signal\n", agPercent);
            Serial.println("  Reaction: Ag → Ag⁺ + e⁻");
            Serial.printf("  Peak at %.3fV indicates Ag oxidation\n",
                          result->silver.peakVoltage);

            if (result->silver.peakHeight > 4.0) {
                Serial.println("  → HIGH silver content (~10-20% alloy)");
            } else if (result->silver.peakHeight > 1.5) {
                Serial.println("  → MEDIUM silver content (~5-10% alloy)");
            } else {
                Serial.println("  → LOW silver content (~2-5% alloy)");
            }
            Serial.println();
        }

        // Nickel interpretation
        if (result->nickel.detected && result->nickel.peakHeight > 0.3) {
            Serial.printf("• Nickel detected: %.3f µA peak\n", result->nickel.peakHeight);
            Serial.println("  → White gold often contains nickel for hardness");
            Serial.println();
        }

        // Zinc interpretation
        if (result->zinc.detected && result->zinc.peakHeight > 0.3) {
            Serial.printf("• Zinc detected: %.3f µA peak\n", result->zinc.peakHeight);
            Serial.println("  → Some alloys use zinc for color/workability");
            Serial.println();
        }
    }

    Serial.println("═══════════════════════════════════════\n");
}

// ═══════════════════════════════════════════════════════════════════════════
// ADVANCED: LOCAL MAXIMUM FINDER
// ═══════════════════════════════════════════════════════════════════════════

bool findLocalMaximum(int startIdx, int endIdx, PeakInfo* peakInfo) {
    // This function finds true local maxima (not just highest point)
    // A local maximum is where: I[i-1] < I[i] > I[i+1]

    float maxCurrent = 0.0;
    float maxVoltage = 0.0;
    bool foundPeak = false;

    for (int i = startIdx + 1; i < endIdx - 1; i++) {
        float prevI = abs(scanData.current[i - 1]);
        float currI = abs(scanData.current[i]);
        float nextI = abs(scanData.current[i + 1]);

        // Check if this is a local maximum
        if (currI > prevI && currI > nextI && currI > NOISE_FLOOR) {
            if (currI > maxCurrent) {
                maxCurrent = currI;
                maxVoltage = scanData.voltage[i];
                foundPeak = true;
            }
        }
    }

    if (foundPeak) {
        peakInfo->detected = true;
        peakInfo->peakHeight = maxCurrent;
        peakInfo->peakVoltage = maxVoltage;
    }

    return foundPeak;
}

void printPeakAnalysis(const PeakAnalysisResult* result) {
    Serial.println("\n═══════════════════════════════════════");
    Serial.println("DETAILED PEAK ANALYSIS REPORT");
    Serial.println("═══════════════════════════════════════\n");

    Serial.println("Metal    | Peak Height | Peak Voltage | Area");
    Serial.println("─────────┼─────────────┼──────────────┼────────");

    if (result->copper.detected) {
        Serial.printf("Copper   | %7.3f µA  | %8.3f V  | %.3f\n",
                      result->copper.peakHeight,
                      result->copper.peakVoltage,
                      result->copper.peakArea);
    }

    if (result->silver.detected) {
        Serial.printf("Silver   | %7.3f µA  | %8.3f V  | %.3f\n",
                      result->silver.peakHeight,
                      result->silver.peakVoltage,
                      result->silver.peakArea);
    }

    if (result->nickel.detected) {
        Serial.printf("Nickel   | %7.3f µA  | %8.3f V  | %.3f\n",
                      result->nickel.peakHeight,
                      result->nickel.peakVoltage,
                      result->nickel.peakArea);
    }

    if (result->zinc.detected) {
        Serial.printf("Zinc     | %7.3f µA  | %8.3f V  | %.3f\n",
                      result->zinc.peakHeight,
                      result->zinc.peakVoltage,
                      result->zinc.peakArea);
    }

    Serial.println("─────────┴─────────────┴──────────────┴────────");
    Serial.printf("Total peaks: %d | Total magnitude: %.3f µA\n",
                  result->numPeaksDetected, result->totalMagnitude);
    Serial.println("═══════════════════════════════════════\n");
}
