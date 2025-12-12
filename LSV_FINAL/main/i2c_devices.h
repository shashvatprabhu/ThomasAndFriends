/**
 * @file i2c_devices.h
 * @brief I2C device drivers for MCP4725 DAC and ADS1115 ADC
 */

#ifndef I2C_DEVICES_H
#define I2C_DEVICES_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// ═══════════════════════════════════════════════════════════════════════════
// I2C INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

esp_err_t i2c_master_init(void);

// ═══════════════════════════════════════════════════════════════════════════
// MCP4725 DAC FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

esp_err_t mcp4725_set_voltage(uint16_t dac_value);
esp_err_t mcp4725_init(void);

// ═══════════════════════════════════════════════════════════════════════════
// ADS1115 ADC FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

// ADS1115 Register Addresses
#define ADS1115_REG_CONVERSION      0x00
#define ADS1115_REG_CONFIG          0x01

// ADS1115 Configuration Bits
#define ADS1115_OS_SINGLE           0x8000  // Start single conversion
#define ADS1115_MUX_AIN0_GND        0x4000  // AIN0 vs GND
#define ADS1115_MUX_AIN0_AIN1       0x0000  // AIN0 vs AIN1 (differential)
#define ADS1115_MUX_AIN1_GND        0x5000  // AIN1 vs GND
#define ADS1115_PGA_2_048V          0x0200  // ±2.048V range
#define ADS1115_MODE_SINGLE         0x0100  // Single-shot mode
#define ADS1115_DR_128SPS           0x0080  // 128 samples/sec
#define ADS1115_COMP_QUE_DISABLE    0x0003  // Disable comparator

esp_err_t ads1115_init(void);
esp_err_t ads1115_read_differential_A0_A1(int16_t *result);
float ads1115_counts_to_volts(int16_t counts);

#endif // I2C_DEVICES_H
