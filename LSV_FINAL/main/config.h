/**
 * @file config.h
 * @brief Configuration for ESP32-WROOM-32E LSV Potentiostat
 * @author Smart India Hackathon 2025 Team
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
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000

// I2C Device Addresses
#define MCP4725_ADDR                0x60
#define ADS1115_ADDR                0x48

// ═══════════════════════════════════════════════════════════════════════════
// LSV SCAN PARAMETERS
// ═══════════════════════════════════════════════════════════════════════════

#define START_VOLTAGE               0.0f
#define END_VOLTAGE                 1.2f
#define STEP_SIZE                   0.01f
#define SETTLE_TIME_MS              1000
#define SCAN_INTERVAL_MS            6000
#define MAX_DATA_POINTS             300

// ═══════════════════════════════════════════════════════════════════════════
// CALIBRATION CONSTANTS
// ═══════════════════════════════════════════════════════════════════════════

#define R_FEEDBACK                  50000.0f    // 50kΩ (measured)
#define DAC_RESOLUTION              4095        // 12-bit
#define DAC_VREF                    3.3f
// If you added a mid-supply reference (divider) for the TIA, put its voltage here.
// We will sweep the DAC around this bias: DAC = CELL_BIAS_V + sweep_voltage
#define CELL_BIAS_V                 1.65f

// ADS1115 Configuration
// NOTE: ADS1115_PGA_2_048V (0x0200) = ±2.048V full-scale (not ±4.096V).
#define ADS1115_GAIN_TWO            0x0200      // (kept for compatibility; PGA bits in i2c_devices.h are the source of truth)
#define ADS1115_MAX_VOLTAGE         2.048f      // Must match ADS1115_PGA_2_048V


// ═══════════════════════════════════════════════════════════════════════════
// PEAK DETECTION
// ═══════════════════════════════════════════════════════════════════════════

#define CU_PEAK_V_MIN               0.00f
#define CU_PEAK_V_MAX               0.25f
#define AG_PEAK_V_MIN               0.30f
#define AG_PEAK_V_MAX               0.55f

#define CU_THRESHOLD                0.3f    // Lowered for better sensitivity
#define AG_THRESHOLD                0.2f    // Lowered for better sensitivity
#define NOISE_FLOOR                 0.15f   // Lowered to detect smaller signals

// ═══════════════════════════════════════════════════════════════════════════
// KARAT THRESHOLDS
// ═══════════════════════════════════════════════════════════════════════════

#define KARAT_24_MAX                0.5f
#define KARAT_22_MAX                2.0f
#define KARAT_18_MAX                5.0f

#endif // CONFIG_H
