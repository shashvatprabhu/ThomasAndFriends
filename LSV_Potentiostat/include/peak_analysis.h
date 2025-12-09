/**
 * @file peak_analysis.h
 * @brief Peak Detection and Analysis Module Header
 * @author Smart India Hackathon 2025 Team
 *
 * This module analyzes LSV scan data to identify oxidation peaks
 * characteristic of different metals (Cu, Ag, Ni, Zn) in gold alloys.
 */

#ifndef PEAK_ANALYSIS_H
#define PEAK_ANALYSIS_H

#include <Arduino.h>
#include "config.h"
#include "lsv_scan.h"

// ═══════════════════════════════════════════════════════════════════════════
// DATA STRUCTURES
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    bool detected;          // Peak detected above threshold
    float peakHeight;       // Maximum current in voltage window (µA)
    float peakVoltage;      // Voltage at which peak occurs (V)
    float peakArea;         // Integrated peak area (for quantification)
} PeakInfo;

typedef struct {
    PeakInfo copper;        // Copper oxidation peak (~0.0 to +0.2V)
    PeakInfo silver;        // Silver oxidation peak (~0.3 to +0.5V)
    PeakInfo nickel;        // Nickel oxidation peak (~-0.1 to +0.1V)
    PeakInfo zinc;          // Zinc oxidation peak (~-0.8 to -0.5V)
    float totalMagnitude;   // Sum of all peak heights
    int numPeaksDetected;   // Total number of peaks above threshold
} PeakAnalysisResult;

// ═══════════════════════════════════════════════════════════════════════════
// FUNCTION PROTOTYPES
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Analyze scan data for oxidation peaks
 * @param result Pointer to structure to store results
 * @return true if analysis completed successfully
 */
bool analyzePeaks(PeakAnalysisResult* result);

/**
 * @brief Find maximum current in specified voltage range
 * @param vMin Minimum voltage of search window (V)
 * @param vMax Maximum voltage of search window (V)
 * @param peakInfo Pointer to PeakInfo structure to store results
 * @return true if peak found above noise floor
 */
bool findPeakInRange(float vMin, float vMax, PeakInfo* peakInfo);

/**
 * @brief Calculate integrated peak area (for quantitative analysis)
 * @param vMin Minimum voltage of integration window (V)
 * @param vMax Maximum voltage of integration window (V)
 * @return Integrated area (µA·V)
 */
float calculatePeakArea(float vMin, float vMax);

/**
 * @brief Apply smoothing filter to scan data
 * @param windowSize Number of points for moving average (3, 5, or 7)
 * @note Modifies scanData.current[] in place
 */
void applySmoothingFilter(int windowSize);

/**
 * @brief Print detailed peak analysis report
 * @param result Peak analysis results to display
 */
void printPeakAnalysis(const PeakAnalysisResult* result);

/**
 * @brief Print electrochemistry interpretation of detected peaks
 * @param result Peak analysis results
 */
void printInterpretation(const PeakAnalysisResult* result);

/**
 * @brief Find local maxima in current data
 * @param startIdx Starting index in scanData arrays
 * @param endIdx Ending index in scanData arrays
 * @param peakInfo Pointer to store peak information
 * @return true if local maximum found
 */
bool findLocalMaximum(int startIdx, int endIdx, PeakInfo* peakInfo);

#endif // PEAK_ANALYSIS_H
