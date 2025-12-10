/**
 * @file ads1115.h
 * @brief ADS1115 16-bit ADC driver for ESP32
 */

#ifndef ADS1115_H
#define ADS1115_H

#include <stdint.h>
#include "esp_err.h"

// ADS1115 Register Addresses
#define ADS1115_REG_CONVERSION      0x00
#define ADS1115_REG_CONFIG          0x01

// ADS1115 Configuration Bits
#define ADS1115_OS_SINGLE           0x8000  // Start single conversion
#define ADS1115_MUX_AIN0_GND        0x4000  // AIN0 vs GND
#define ADS1115_MUX_AIN1_GND        0x5000  // AIN1 vs GND
#define ADS1115_MUX_AIN2_GND        0x6000  // AIN2 vs GND
#define ADS1115_MUX_AIN3_GND        0x7000  // AIN3 vs GND

// Programmable Gain Amplifier (PGA) settings
#define ADS1115_PGA_6_144V          0x0000  // ±6.144V range (0.1875mV/bit)
#define ADS1115_PGA_4_096V          0x0200  // ±4.096V range (0.125mV/bit)
#define ADS1115_PGA_2_048V          0x0400  // ±2.048V range (0.0625mV/bit)
#define ADS1115_PGA_1_024V          0x0600  // ±1.024V range (0.03125mV/bit)
#define ADS1115_PGA_0_512V          0x0800  // ±0.512V range (0.015625mV/bit)
#define ADS1115_PGA_0_256V          0x0A00  // ±0.256V range (0.0078125mV/bit)

// Other config bits
#define ADS1115_MODE_SINGLE         0x0100  // Single-shot mode
#define ADS1115_DR_128SPS           0x0080  // 128 samples/sec
#define ADS1115_COMP_QUE_DISABLE    0x0003  // Disable comparator

// Function prototypes
esp_err_t ads1115_init(void);
esp_err_t ads1115_read_single(uint8_t channel, int16_t *result);
float ads1115_counts_to_volts(int16_t counts, uint16_t gain_config);

#endif // ADS1115_H
