/**
 * @file i2c_devices.c
 * @brief I2C device drivers implementation using ESP-IDF
 */

#include <stdio.h>
#include <string.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_devices.h"
#include "config.h"

static const char *TAG = "I2C_DEVICES";

// ═══════════════════════════════════════════════════════════════════════════
// I2C MASTER INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                             I2C_MASTER_RX_BUF_DISABLE,
                             I2C_MASTER_TX_BUF_DISABLE, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "I2C initialized on GPIO%d (SDA) and GPIO%d (SCL)",
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    return ESP_OK;
}

// ═══════════════════════════════════════════════════════════════════════════
// MCP4725 DAC DRIVER
// ═══════════════════════════════════════════════════════════════════════════

esp_err_t mcp4725_init(void) {
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "MCP4725 POWER & CONNECTION CHECK");
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "STEP 1: Measure Power Supply");
    ESP_LOGI(TAG, "  → Set multimeter to DC voltage");
    ESP_LOGI(TAG, "  → Red probe on MCP4725 VDD pin");
    ESP_LOGI(TAG, "  → Black probe on MCP4725 GND pin");
    ESP_LOGI(TAG, "  → Should read: 3.3V ± 0.1V");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "STEP 2: Check Ground Connection");
    ESP_LOGI(TAG, "  → Measure continuity: ESP32 GND to MCP4725 GND");
    ESP_LOGI(TAG, "  → Should read: 0Ω (short circuit)");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "STEP 3: Testing I2C communication...");
    ESP_LOGI(TAG, "");
    
    // Test I2C communication with MCP4725
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MCP4725_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd,
                                          pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ MCP4725 responding at address 0x%02X", MCP4725_ADDR);
        ESP_LOGI(TAG, "  (I2C communication OK)");
        ESP_LOGI(TAG, "");
        
        // Read current DAC value
        uint16_t current_value;
        if (mcp4725_read_voltage(&current_value) == ESP_OK) {
            float current_voltage = (current_value / (float)DAC_RESOLUTION) * DAC_VREF;
            ESP_LOGI(TAG, "  Current DAC register: %d (%.3fV)", current_value, current_voltage);
        }
        ESP_LOGI(TAG, "");
        
        // HARDWARE CHECK: Try setting to mid-range voltage to test output
        ESP_LOGI(TAG, "STEP 4: Testing VOUT output...");
        ESP_LOGI(TAG, "  Setting DAC to 1.65V (50%% of range)...");
        ret = mcp4725_set_voltage(2048); // 50% = 1.65V
        if (ret == ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(200)); // Wait for output to stabilize
            uint16_t test_readback;
            if (mcp4725_read_voltage(&test_readback) == ESP_OK) {
                ESP_LOGI(TAG, "  ✓ DAC register verified: %d", test_readback);
                ESP_LOGI(TAG, "");
                ESP_LOGI(TAG, "STEP 5: Measure VOUT with multimeter");
                ESP_LOGI(TAG, "  → Red probe on MCP4725 VOUT pin");
                ESP_LOGI(TAG, "  → Black probe on MCP4725 GND pin");
                ESP_LOGI(TAG, "  → Expected: ~1.65V");
                ESP_LOGI(TAG, "");
                ESP_LOGI(TAG, "  If reading 0V:");
                ESP_LOGI(TAG, "    ✗ VDD not connected (check power)");
                ESP_LOGI(TAG, "    ✗ VOUT pin not connected to circuit");
                ESP_LOGI(TAG, "    ✗ VOUT shorted to GND");
                ESP_LOGI(TAG, "    ✗ MCP4725 chip damaged");
                ESP_LOGI(TAG, "");
            }
        }
        
        // Set initial voltage to 0V
        ESP_LOGI(TAG, "Setting to 0.000V (safe state)...");
        ret = mcp4725_set_voltage(0);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "✓ Initialized to 0.000V");
        } else {
            ESP_LOGE(TAG, "✗ Failed to initialize to 0V: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGE(TAG, "✗✗✗ MCP4725 NOT RESPONDING ✗✗✗");
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "I2C Error: %s (0x%X)", esp_err_to_name(ret), ret);
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "TROUBLESHOOTING CHECKLIST:");
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "1. POWER SUPPLY (MOST COMMON ISSUE):");
        ESP_LOGE(TAG, "   → Measure VDD to GND: Should be 3.3V");
        ESP_LOGE(TAG, "   → If 0V: Check ESP32 3.3V pin connection");
        ESP_LOGE(TAG, "   → If wrong voltage: Check power supply");
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "2. GROUND CONNECTION:");
        ESP_LOGE(TAG, "   → ESP32 GND must connect to MCP4725 GND");
        ESP_LOGE(TAG, "   → Measure continuity: Should be 0Ω");
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "3. I2C WIRING:");
        ESP_LOGE(TAG, "   → SDA: ESP32 GPIO%d → MCP4725 SDA", I2C_MASTER_SDA_IO);
        ESP_LOGE(TAG, "   → SCL: ESP32 GPIO%d → MCP4725 SCL", I2C_MASTER_SCL_IO);
        ESP_LOGE(TAG, "   → Check for loose connections");
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "4. PULL-UP RESISTORS:");
        ESP_LOGE(TAG, "   → SDA and SCL need 4.7kΩ pull-ups to 3.3V");
        ESP_LOGE(TAG, "   → Most breakout boards have these built-in");
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "5. I2C ADDRESS:");
        ESP_LOGE(TAG, "   → MCP4725 A0 pin to GND = address 0x%02X", MCP4725_ADDR);
        ESP_LOGE(TAG, "   → Check A0 pin connection");
        ESP_LOGE(TAG, "");
    }
    
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "");

    return ret;
}

esp_err_t mcp4725_read_voltage(uint16_t *dac_value) {
    uint8_t read_data[5];
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MCP4725_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, read_data, 5, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd,
                                          pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        // MCP4725 returns: [Status/Upper] [Upper] [Lower] [EEPROM Upper] [EEPROM Lower]
        // DAC value is in bytes 1-2: D11-D4 in byte 1, D3-D0 in upper nibble of byte 2
        *dac_value = ((read_data[1] << 4) | (read_data[2] >> 4)) & 0x0FFF;
    }

    return ret;
}

esp_err_t mcp4725_set_voltage(uint16_t dac_value) {
    if (dac_value > DAC_RESOLUTION) {
        ESP_LOGW(TAG, "DAC value %d exceeds max %d", dac_value, DAC_RESOLUTION);
        dac_value = DAC_RESOLUTION;
    }

    uint8_t data[3];
    // MCP4725 Fast Mode Write format (matches lsv.c):
    // Byte 0: 0x40 = Fast mode write to DAC register, normal power
    // Byte 1: D11-D4 (upper 8 bits of 12-bit value)
    // Byte 2: D3-D0 in upper nibble (lower 4 bits shifted left)
    data[0] = 0x40;                               // Fast mode write command
    data[1] = (dac_value >> 4) & 0xFF;            // D11-D4 (upper 8 bits)
    data[2] = (dac_value & 0x0F) << 4;            // D3-D0 in upper nibble (matches lsv.c format)

    float target_voltage = (dac_value / (float)DAC_RESOLUTION) * DAC_VREF;
    ESP_LOGI(TAG, "MCP4725: Writing DAC value %d (target %.3fV) [0x%02X 0x%02X 0x%02X]",
             dac_value, target_voltage, data[0], data[1], data[2]);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MCP4725_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, 3, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd,
                                          pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MCP4725 write FAILED! Error: %s (0x%X)", esp_err_to_name(ret), ret);
        ESP_LOGE(TAG, "  Address: 0x%02X, Data: [0x%02X 0x%02X 0x%02X]",
                 MCP4725_ADDR, data[0], data[1], data[2]);
    } else {
        // Verify write by reading back
        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay for DAC to update
        uint16_t readback_value;
        esp_err_t read_ret = mcp4725_read_voltage(&readback_value);
        if (read_ret == ESP_OK) {
            if (readback_value == dac_value) {
                ESP_LOGI(TAG, "MCP4725: Write verified ✓ (readback: %d = %.3fV)", 
                         readback_value, (readback_value / (float)DAC_RESOLUTION) * DAC_VREF);
            } else {
                ESP_LOGW(TAG, "MCP4725: Write mismatch! Wrote %d, read %d", 
                         dac_value, readback_value);
            }
        } else {
            ESP_LOGW(TAG, "MCP4725: Write OK but readback failed: %s", esp_err_to_name(read_ret));
        }
    }

    return ret;
}

// ═══════════════════════════════════════════════════════════════════════════
// ADS1115 ADC DRIVER
// ═══════════════════════════════════════════════════════════════════════════

static esp_err_t ads1115_write_register(uint8_t reg, uint16_t value) {
    uint8_t data[3];
    data[0] = reg;
    data[1] = (value >> 8) & 0xFF;
    data[2] = value & 0xFF;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADS1115_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, 3, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd,
                                          pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t ads1115_read_register(uint8_t reg, uint16_t *value) {
    uint8_t data[2];

    // Write register address
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADS1115_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd,
                                          pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) return ret;

    // Read register value
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADS1115_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd,
                                pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        *value = (data[0] << 8) | data[1];
    }

    return ret;
}

esp_err_t ads1115_init(void) {
    // Test I2C communication
    uint16_t config;
    esp_err_t ret = ads1115_read_register(ADS1115_REG_CONFIG, &config);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADS1115 found at address 0x%02X", ADS1115_ADDR);
        ESP_LOGI(TAG, "Config register: 0x%04X", config);
    } else {
        ESP_LOGE(TAG, "ADS1115 not found at address 0x%02X", ADS1115_ADDR);
    }

    return ret;
}

esp_err_t ads1115_read_single(int16_t *result) {
    // Configure for single-shot conversion on AIN0 vs GND
    uint16_t config = ADS1115_OS_SINGLE |
                      ADS1115_MUX_AIN0_GND |
                      ADS1115_PGA_2_048V |
                      ADS1115_MODE_SINGLE |
                      ADS1115_DR_128SPS |
                      ADS1115_COMP_QUE_DISABLE;

    esp_err_t ret = ads1115_write_register(ADS1115_REG_CONFIG, config);
    if (ret != ESP_OK) return ret;

    // Wait for conversion to complete (8ms typical for 128SPS)
    vTaskDelay(pdMS_TO_TICKS(10));

    // Read conversion result
    uint16_t raw_value;
    ret = ads1115_read_register(ADS1115_REG_CONVERSION, &raw_value);
    if (ret == ESP_OK) {
        *result = (int16_t)raw_value;
    }

    return ret;
}

esp_err_t ads1115_read_differential_A0_A1(int16_t *result) {
    // Configure for differential reading: A0 - A1
    uint16_t config = ADS1115_OS_SINGLE |
                      ADS1115_MUX_AIN0_AIN1 |  // A0 vs A1 (differential)
                      ADS1115_PGA_2_048V |
                      ADS1115_MODE_SINGLE |
                      ADS1115_DR_128SPS |
                      ADS1115_COMP_QUE_DISABLE;

    esp_err_t ret = ads1115_write_register(ADS1115_REG_CONFIG, config);
    if (ret != ESP_OK) return ret;

    // Wait for conversion to complete by polling OS bit
    // When OS=1 is written, conversion starts and OS bit goes to 0
    // When conversion completes, OS bit goes back to 1
    // So we need to wait for OS to become 1 again
    int timeout = 50; // 50ms timeout
    bool conversion_done = false;
    
    // Small delay to let conversion start (OS bit clears)
    vTaskDelay(pdMS_TO_TICKS(2));
    
    while (timeout-- > 0) {
        uint16_t config_read;
        ret = ads1115_read_register(ADS1115_REG_CONFIG, &config_read);
        if (ret != ESP_OK) return ret;
        
        // OS bit (bit 15) becomes 1 when conversion is complete
        if (config_read & 0x8000) {
            conversion_done = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    if (!conversion_done) {
        ESP_LOGW(TAG, "ADS1115 differential conversion timeout");
        return ESP_ERR_TIMEOUT;
    }

    // Read conversion result
    uint16_t raw_value;
    ret = ads1115_read_register(ADS1115_REG_CONVERSION, &raw_value);
    if (ret == ESP_OK) {
        *result = (int16_t)raw_value;
    }

    return ret;
}

esp_err_t ads1115_read_channel(uint8_t channel, int16_t *result) {
    // Map channel number to MUX configuration
    uint16_t mux;
    switch(channel) {
        case 0:  mux = ADS1115_MUX_AIN0_GND; break;
        case 1:  mux = ADS1115_MUX_AIN1_GND; break;
        default: return ESP_ERR_INVALID_ARG;
    }

    uint16_t config = ADS1115_OS_SINGLE |
                      mux |
                      ADS1115_PGA_2_048V |
                      ADS1115_MODE_SINGLE |
                      ADS1115_DR_128SPS |
                      ADS1115_COMP_QUE_DISABLE;

    esp_err_t ret = ads1115_write_register(ADS1115_REG_CONFIG, config);
    if (ret != ESP_OK) return ret;

    // Wait for conversion to complete by polling OS bit
    int timeout = 50; // 50ms timeout
    while (timeout-- > 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        uint16_t config_read;
        ret = ads1115_read_register(ADS1115_REG_CONFIG, &config_read);
        if (ret != ESP_OK) return ret;
        // OS bit (bit 15) should be 1 when conversion is complete
        if (config_read & 0x8000) {
            break; // Conversion complete
        }
    }
    if (timeout <= 0) {
        ESP_LOGW(TAG, "ADS1115 conversion timeout");
        return ESP_ERR_TIMEOUT;
    }

    uint16_t raw_value;
    ret = ads1115_read_register(ADS1115_REG_CONVERSION, &raw_value);
    if (ret == ESP_OK) {
        *result = (int16_t)raw_value;
    }

    return ret;
}

float ads1115_counts_to_volts(int16_t counts) {
    // With ±2.048V range: LSB = 2.048V / 32768 = 0.0625mV
    return (float)counts * (ADS1115_MAX_VOLTAGE / 32768.0f);
}
