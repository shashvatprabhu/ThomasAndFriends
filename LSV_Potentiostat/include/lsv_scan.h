/**
 * @file lsv_scan.h
 * @brief Linear Sweep Voltammetry Scan Module Header
 * @author Smart India Hackathon 2025 Team
 *
 * This module handles the core LSV functionality:
 * - Voltage sweeping via DAC
 * - Current measurement via ADC
 * - Data storage and real-time output
 */

#ifndef LSV_SCAN_H
#define LSV_SCAN_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MCP4725.h>
#include "config.h"

// ═══════════════════════════════════════════════════════════════════════════
// DATA STRUCTURES
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    float voltage[MAX_DATA_POINTS];     // Applied voltage at each step (V)
    float current[MAX_DATA_POINTS];     // Measured current at each step (µA)
    int numPoints;                       // Actual number of data points collected
    float baselineCurrent;               // Baseline current at 0V (for subtraction)
    bool scanComplete;                   // Flag indicating scan completion
} LSVData;

// ═══════════════════════════════════════════════════════════════════════════
// GLOBAL OBJECTS (defined in .cpp file)
// ═══════════════════════════════════════════════════════════════════════════

extern Adafruit_ADS1115 ads;           // ADC object
extern Adafruit_MCP4725 dac;           // DAC object
extern LSVData scanData;               // Global scan data storage

// ═══════════════════════════════════════════════════════════════════════════
// FUNCTION PROTOTYPES
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Initialize I2C bus and hardware devices
 * @return true if initialization successful, false otherwise
 */
bool initializeHardware();

/**
 * @brief Set DAC output voltage
 * @param voltage Desired output voltage (0 to DAC_VREF)
 * @return true if successful, false if voltage out of range
 */
bool setDACVoltage(float voltage);

/**
 * @brief Read current from ADC (transimpedance amplifier output)
 * @return Measured current in microamps (µA)
 */
float readCurrent();

/**
 * @brief Perform complete LSV scan from START_VOLTAGE to END_VOLTAGE
 * @return true if scan completed successfully
 */
bool performLSVScan();

/**
 * @brief Measure and store baseline current at 0V
 * @return Baseline current in microamps
 */
float measureBaseline();

/**
 * @brief Clear scan data arrays and reset counters
 */
void clearScanData();

/**
 * @brief Print formatted table header for scan data
 */
void printScanHeader();

/**
 * @brief Print scan data point with formatting
 * @param index Data point index
 * @param highlight Set true to highlight significant currents
 */
void printDataPoint(int index, bool highlight);

/**
 * @brief Export scan data in CSV format
 * @note Only active if ENABLE_CSV_OUTPUT is true
 */
void exportCSV();

/**
 * @brief Return DAC to safe 0V state
 */
void returnToZero();

/**
 * @brief Check for common errors during scan
 * @return true if no errors detected
 */
bool errorCheck();

#endif // LSV_SCAN_H
