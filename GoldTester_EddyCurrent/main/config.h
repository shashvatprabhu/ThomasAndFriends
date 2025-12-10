/**
 * @file config.h
 * @brief Configuration for ESP32 Eddy Current Gold Tester
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// ═══════════════════════════════════════════════════════════════════════════
// ESP32 GPIO CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════

#define I2C_MASTER_SCL_IO           22      // GPIO22 for I2C SCL
#define I2C_MASTER_SDA_IO           21      // GPIO21 for I2C SDA
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          400000  // 400kHz fast mode
#define I2C_MASTER_TIMEOUT_MS       1000

// ADS1115 I2C Address
#define ADS1115_ADDR                0x48

// ═══════════════════════════════════════════════════════════════════════════
// MEASUREMENT PARAMETERS
// ═══════════════════════════════════════════════════════════════════════════

#define SAMPLES                     200     // Number of samples to average
#define DELAY_BETWEEN_READINGS_MS   100     // Delay between measurements
#define BASELINE_SAMPLES            500     // Extra samples for baseline

// ═══════════════════════════════════════════════════════════════════════════
// CIRCUIT SPECIFICATIONS
// ═══════════════════════════════════════════════════════════════════════════

// Coil specifications
#define COIL_TURNS                  90      // Number of wire turns
#define COIL_WIRE_DIAMETER_MM       0.7     // Wire diameter
#define COIL_FORMER_DIAMETER_MM     10.0    // Bobbin/former diameter

// NE555 frequency (approximately 100kHz based on your circuit)
#define OSCILLATOR_FREQ_HZ          100000  // 100kHz

// Current limiting resistor
#define CURRENT_RESISTOR_OHMS       220     // 220Ω resistor

// Voltage divider (10kΩ + 10kΩ)
#define R1_OHMS                     10000   // Upper resistor
#define R2_OHMS                     10000   // Lower resistor

// ═══════════════════════════════════════════════════════════════════════════
// CALIBRATION DATA STRUCTURE
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    float voltage;          // Measured voltage (V)
    int karat;             // Gold purity (K)
} gold_calibration_t;

// Default calibration points (MUST UPDATE with your measured values!)
#define NUM_CALIBRATION_POINTS  5

// ═══════════════════════════════════════════════════════════════════════════
// DETECTION THRESHOLDS
// ═══════════════════════════════════════════════════════════════════════════

#define NO_METAL_THRESHOLD_V        0.02    // Voltage change < 20mV = no metal
#define NOISE_GOOD_MV               5.0     // Noise < 5mV = high confidence
#define NOISE_ACCEPTABLE_MV         15.0    // Noise < 15mV = medium confidence

#endif // CONFIG_H
