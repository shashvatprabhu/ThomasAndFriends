/**
 * @file i2c_debug.h
 * @brief I2C diagnostic and debugging utilities
 */

#ifndef I2C_DEBUG_H
#define I2C_DEBUG_H

/**
 * @brief Scan I2C bus for all devices
 * Prints addresses of all responding devices
 */
void i2c_scan_bus(void);

/**
 * @brief Test MCP4725 DAC communication
 * Writes test values and verifies communication
 */
void i2c_test_dac(void);

/**
 * @brief Test ADS1115 ADC communication
 * Reads registers and verifies communication
 */
void i2c_test_adc(void);

/**
 * @brief Monitor I2C bus activity
 * Continuously checks device responses
 */
void i2c_monitor_traffic(void);

#endif // I2C_DEBUG_H
