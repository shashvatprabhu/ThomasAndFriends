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
    // Test I2C communication with MCP4725
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MCP4725_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd,
                                          pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "MCP4725 found at address 0x%02X", MCP4725_ADDR);
        // Set initial voltage to 0V
        mcp4725_set_voltage(0);
    } else {
        ESP_LOGE(TAG, "MCP4725 not found at address 0x%02X", MCP4725_ADDR);
    }

    return ret;
}

esp_err_t mcp4725_set_voltage(uint16_t dac_value) {
    if (dac_value > DAC_RESOLUTION) {
        ESP_LOGW(TAG, "DAC value %d exceeds max %d", dac_value, DAC_RESOLUTION);
        dac_value = DAC_RESOLUTION;
    }

    uint8_t data[3];
    // Fast mode write: C2 C1 C0 X X PD1 PD0 X D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0 X X X X
    data[0] = 0x40;  // Write DAC register, normal mode
    data[1] = (dac_value >> 4) & 0xFF;  // Upper 8 bits
    data[2] = (dac_value << 4) & 0xF0;  // Lower 4 bits

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MCP4725_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, 3, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd,
                                          pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

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

float ads1115_counts_to_volts(int16_t counts) {
    // With ±2.048V range: LSB = 2.048V / 32768 = 0.0625mV
    return (float)counts * (ADS1115_MAX_VOLTAGE / 32768.0f);
}
