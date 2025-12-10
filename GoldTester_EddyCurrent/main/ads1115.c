/**
 * @file ads1115.c
 * @brief ADS1115 ADC driver implementation
 */

#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ads1115.h"
#include "config.h"

static const char *TAG = "ADS1115";

// ═══════════════════════════════════════════════════════════════════════════
// I2C LOW-LEVEL FUNCTIONS
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

// ═══════════════════════════════════════════════════════════════════════════
// PUBLIC FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

esp_err_t ads1115_init(void) {
    // Initialize I2C
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

    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "I2C initialized on GPIO%d (SDA) and GPIO%d (SCL)",
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

    // Test communication
    uint16_t config;
    err = ads1115_read_register(ADS1115_REG_CONFIG, &config);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "ADS1115 found at address 0x%02X", ADS1115_ADDR);
    } else {
        ESP_LOGE(TAG, "ADS1115 not found at address 0x%02X", ADS1115_ADDR);
    }

    return err;
}

esp_err_t ads1115_read_single(uint8_t channel, int16_t *result) {
    // Configure for single-shot conversion
    uint16_t mux_config;
    switch (channel) {
        case 0: mux_config = ADS1115_MUX_AIN0_GND; break;
        case 1: mux_config = ADS1115_MUX_AIN1_GND; break;
        case 2: mux_config = ADS1115_MUX_AIN2_GND; break;
        case 3: mux_config = ADS1115_MUX_AIN3_GND; break;
        default: return ESP_ERR_INVALID_ARG;
    }

    uint16_t config = ADS1115_OS_SINGLE |
                      mux_config |
                      ADS1115_PGA_4_096V |      // ±4.096V range
                      ADS1115_MODE_SINGLE |
                      ADS1115_DR_128SPS |
                      ADS1115_COMP_QUE_DISABLE;

    esp_err_t ret = ads1115_write_register(ADS1115_REG_CONFIG, config);
    if (ret != ESP_OK) return ret;

    // Wait for conversion (8ms typical for 128SPS)
    vTaskDelay(pdMS_TO_TICKS(10));

    // Read result
    uint16_t raw_value;
    ret = ads1115_read_register(ADS1115_REG_CONVERSION, &raw_value);
    if (ret == ESP_OK) {
        *result = (int16_t)raw_value;
    }

    return ret;
}

float ads1115_counts_to_volts(int16_t counts, uint16_t gain_config) {
    // Determine LSB size based on gain setting
    float lsb;
    switch (gain_config) {
        case ADS1115_PGA_6_144V: lsb = 6.144f / 32768.0f; break;
        case ADS1115_PGA_4_096V: lsb = 4.096f / 32768.0f; break;  // 0.125mV
        case ADS1115_PGA_2_048V: lsb = 2.048f / 32768.0f; break;
        case ADS1115_PGA_1_024V: lsb = 1.024f / 32768.0f; break;
        case ADS1115_PGA_0_512V: lsb = 0.512f / 32768.0f; break;
        case ADS1115_PGA_0_256V: lsb = 0.256f / 32768.0f; break;
        default: lsb = 4.096f / 32768.0f; break;
    }

    return (float)counts * lsb;
}
