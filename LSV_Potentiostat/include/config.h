/**
 * @file config.h
 * @brief Hardware Configuration and Constants for LSV Potentiostat
 * @author Smart India Hackathon 2025 Team
 * @date 2025-12-09
 *
 * This file contains all hardware pin definitions, I2C addresses, calibration
 * constants, and tunable parameters for the Linear Sweep Voltammetry system.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ═══════════════════════════════════════════════════════════════════════════
// I2C CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════

#define I2C_SDA_PIN         21      // ESP32 GPIO21 for I2C data
#define I2C_SCL_PIN         22      // ESP32 GPIO22 for I2C clock
#define I2C_FREQUENCY       400000  // 400kHz fast mode

// I2C Device Addresses
#define MCP4725_ADDR        0x60    // DAC address (voltage control)
#define ADS1115_ADDR        0x48    // ADC address (current measurement)

// ═══════════════════════════════════════════════════════════════════════════
// LSV SCAN PARAMETERS (Tunable for performance)
// ═══════════════════════════════════════════════════════════════════════════

// Voltage sweep range
#define START_VOLTAGE       0.0     // Starting voltage (V) - typically 0V
#define END_VOLTAGE         1.2     // Ending voltage (V) - stops before Au oxidation
#define STEP_SIZE           0.01    // Voltage step (V) - 10mV for good resolution

// Timing parameters
#define SETTLE_TIME_MS      80      // Wait time per step (ms) for equilibration
#define SCAN_INTERVAL_MS    30000   // Time between scans (30 seconds)
#define STARTUP_DELAY_S     10      // Initial countdown before first scan

// Data storage
#define MAX_DATA_POINTS     300     // Maximum array size for voltage/current data
#define PRINT_INTERVAL      10      // Print every Nth data point (reduce clutter)

// ═══════════════════════════════════════════════════════════════════════════
// CALIBRATION CONSTANTS
// ═══════════════════════════════════════════════════════════════════════════

// Current-to-Voltage Conversion (Transimpedance Amplifier)
#define R_FEEDBACK          100000.0    // 100kΩ feedback resistor on LM358 #2
#define INVERT_CURRENT      true        // Inverting op-amp (flip sign)

// DAC Calibration (MCP4725)
#define DAC_RESOLUTION      4095        // 12-bit DAC (0-4095)
#define DAC_VREF            3.3         // Reference voltage (ESP32 supply)

// ADC Calibration (ADS1115)
#define ADC_GAIN            GAIN_TWO    // ±2.048V range (best for our application)
#define ADC_MAX_VOLTAGE     2.048       // Maximum readable voltage at GAIN_TWO

// ═══════════════════════════════════════════════════════════════════════════
// PEAK DETECTION PARAMETERS
// ═══════════════════════════════════════════════════════════════════════════

// Voltage windows for each metal (vs Cu reference electrode)
#define CU_PEAK_V_MIN       0.00    // Copper oxidation starts
#define CU_PEAK_V_MAX       0.25    // Copper oxidation ends
#define AG_PEAK_V_MIN       0.30    // Silver oxidation starts
#define AG_PEAK_V_MAX       0.55    // Silver oxidation ends
#define NI_PEAK_V_MIN      -0.10    // Nickel oxidation (if extended scan)
#define NI_PEAK_V_MAX       0.15
#define ZN_PEAK_V_MIN      -0.80    // Zinc oxidation (negative voltage!)
#define ZN_PEAK_V_MAX      -0.50

// Peak detection thresholds (microamps)
#define CU_THRESHOLD        0.5     // Minimum current to consider Cu peak valid
#define AG_THRESHOLD        0.3     // Minimum current to consider Ag peak valid
#define NOISE_FLOOR         0.3     // Background noise level
#define MAX_REALISTIC_I     100.0   // Maximum realistic current (error detection)

// ═══════════════════════════════════════════════════════════════════════════
// KARAT ESTIMATION THRESHOLDS
// ═══════════════════════════════════════════════════════════════════════════

// Total peak magnitude ranges (microamps) for karat classification
// NOTE: These are ESTIMATES - calibrate with known standards for accuracy!

#define KARAT_24_MAX        0.5     // 24K: <0.5µA (pure gold, no alloy peaks)
#define KARAT_22_MAX        2.0     // 22K: 0.5-2.0µA (91.6% Au, small peaks)
#define KARAT_18_MAX        5.0     // 18K: 2.0-5.0µA (75% Au, medium peaks)
// 14K or lower: >5.0µA (58.3% Au or less, large peaks)

// ═══════════════════════════════════════════════════════════════════════════
// SERIAL COMMUNICATION
// ═══════════════════════════════════════════════════════════════════════════

#define SERIAL_BAUD         115200  // High baud rate for fast data transfer
#define ENABLE_CSV_OUTPUT   false   // Set true for CSV data export
#define ENABLE_VERBOSE      true    // Set false to reduce output

// ═══════════════════════════════════════════════════════════════════════════
// ADVANCED FEATURES (Enable/Disable)
// ═══════════════════════════════════════════════════════════════════════════

#define ENABLE_BASELINE_SUB false   // Baseline subtraction (offset correction)
#define ENABLE_SMOOTHING    false   // Moving average filter
#define ENABLE_MULTI_SCAN   false   // Multiple scan averaging
#define NUM_SCANS_AVG       3       // Number of scans to average (if enabled)
#define ENABLE_REVERSE_SCAN false   // Bidirectional scan (hysteresis check)

// ═══════════════════════════════════════════════════════════════════════════
// ERROR DETECTION
// ═══════════════════════════════════════════════════════════════════════════

#define HIGH_BASELINE_WARN  1.0     // Warn if baseline current >1µA (possible short)
#define SATURATION_CHECK    true    // Check for ADC saturation
#define ZERO_CURRENT_WARN   true    // Warn if all readings are zero

// ═══════════════════════════════════════════════════════════════════════════
// ELECTROCHEMISTRY REFERENCE DATA (for educational purposes)
// ═══════════════════════════════════════════════════════════════════════════

/*
 * OXIDATION REACTIONS:
 * - Copper (Cu):  Cu → Cu²⁺ + 2e⁻   (E° ≈ +0.1V vs Cu)
 * - Silver (Ag):  Ag → Ag⁺ + e⁻     (E° ≈ +0.4V vs Cu)
 * - Nickel (Ni):  Ni → Ni²⁺ + 2e⁻   (E° ≈ 0.0V vs Cu)
 * - Zinc (Zn):    Zn → Zn²⁺ + 2e⁻   (E° ≈ -0.7V vs Cu)
 * - Gold (Au):    Au → Au³⁺ + 3e⁻   (E° ≈ +1.5V vs Cu) - outside our range!
 *
 * GOLD ALLOY COMPOSITIONS:
 * - 24K: 99.9% Au (pure gold, no significant alloy metals)
 * - 22K: 91.6% Au, 8.4% alloy (typically Cu + Ag)
 * - 18K: 75.0% Au, 25% alloy (Cu, Ag, Ni, Zn combinations)
 * - 14K: 58.3% Au, 41.7% alloy (higher alloy content)
 *
 * Peak height is proportional to alloy metal concentration!
 */

#endif // CONFIG_H
