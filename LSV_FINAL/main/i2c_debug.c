/**
 * @file i2c_debug.c
 * @brief I2C diagnostic and debugging utilities
 */

#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"

static const char *TAG = "I2C_DEBUG";

// Scan I2C bus for devices
void i2c_scan_bus(void) {
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "I2C BUS SCAN");
    ESP_LOGI(TAG, "═══════════════════════════════════════");

    int devices_found = 0;

    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "  ✓ Device found at 0x%02X", addr);
            devices_found++;

            // Identify known devices
            if (addr == 0x48) ESP_LOGI(TAG, "    → ADS1115 ADC");
            if (addr == 0x60) ESP_LOGI(TAG, "    → MCP4725 DAC");
        }
    }

    if (devices_found == 0) {
        ESP_LOGE(TAG, "  ✗ NO DEVICES FOUND!");
        ESP_LOGE(TAG, "  Check:");
        ESP_LOGE(TAG, "    - I2C wiring (SDA=GPIO21, SCL=GPIO22)");
        ESP_LOGE(TAG, "    - Pull-up resistors present");
        ESP_LOGE(TAG, "    - Device power (3.3V)");
    } else {
        ESP_LOGI(TAG, "  Total devices found: %d", devices_found);
    }

    ESP_LOGI(TAG, "═══════════════════════════════════════\n");
}

// Test MCP4725 DAC communication
void i2c_test_dac(void) {
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "MCP4725 DAC TEST");
    ESP_LOGI(TAG, "═══════════════════════════════════════");

    // Test 1: Check device presence
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MCP4725_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ MCP4725 NOT responding at 0x%02X", MCP4725_ADDR);
        ESP_LOGE(TAG, "  Error: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "✓ MCP4725 responding at 0x%02X", MCP4725_ADDR);

    // Test 2: Read current DAC value
    uint8_t read_data[5];
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MCP4725_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, read_data, 5, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        uint16_t dac_value = ((read_data[1] << 4) | (read_data[2] >> 4));
        float voltage = (dac_value / 4095.0f) * 3.3f;
        ESP_LOGI(TAG, "✓ Current DAC value: %d (%.3fV)", dac_value, voltage);
    } else {
        ESP_LOGW(TAG, "⚠ Could not read DAC register");
    }

    // Test 3: Write test values
    ESP_LOGI(TAG, "\nWriting test voltages (measure VOUT with multimeter):");

    uint16_t test_values[] = {0, 1024, 2048, 3072, 4095};
    const char* test_labels[] = {"0.00V", "0.83V", "1.65V", "2.48V", "3.30V"};

    for (int i = 0; i < 5; i++) {
        uint8_t data[3];
        data[0] = 0x40;  // Fast mode write
        data[1] = (test_values[i] >> 4) & 0xFF;
        data[2] = (test_values[i] << 4) & 0xF0;

        cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (MCP4725_ADDR << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, data, 3, true);
        i2c_master_stop(cmd);

        ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "  ✓ Set DAC to %s (code %d)", test_labels[i], test_values[i]);
        } else {
            ESP_LOGE(TAG, "  ✗ Failed to set DAC to %s", test_labels[i]);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));  // Wait 2 seconds for measurement
    }

    ESP_LOGI(TAG, "═══════════════════════════════════════\n");
}

// Test ADS1115 ADC communication
void i2c_test_adc(void) {
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "ADS1115 ADC TEST");
    ESP_LOGI(TAG, "═══════════════════════════════════════");

    // Check device presence
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADS1115_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ ADS1115 NOT responding at 0x%02X", ADS1115_ADDR);
        return;
    }

    ESP_LOGI(TAG, "✓ ADS1115 responding at 0x%02X", ADS1115_ADDR);

    // Read config register
    uint8_t reg = 0x01;  // Config register
    uint8_t data[2];

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADS1115_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);  // Repeated start
    i2c_master_write_byte(cmd, (ADS1115_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        uint16_t config = (data[0] << 8) | data[1];
        ESP_LOGI(TAG, "✓ Config register: 0x%04X", config);
    } else {
        ESP_LOGW(TAG, "⚠ Could not read config register");
    }

    ESP_LOGI(TAG, "═══════════════════════════════════════\n");
}

// Monitor I2C bus activity
void i2c_monitor_traffic(void) {
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "I2C TRAFFIC MONITOR");
    ESP_LOGI(TAG, "Press RESET to stop");
    ESP_LOGI(TAG, "═══════════════════════════════════════\n");

    while (1) {
        // Try to detect any traffic by attempting reads
        for (uint8_t addr = 0x48; addr <= 0x60; addr += 0x18) {
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
            i2c_master_stop(cmd);

            esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(10));
            i2c_cmd_link_delete(cmd);

            const char* device = (addr == 0x48) ? "ADS1115" : "MCP4725";

            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "[%s] ACK", device);
            } else if (ret == ESP_ERR_TIMEOUT) {
                ESP_LOGW(TAG, "[%s] TIMEOUT", device);
            } else {
                ESP_LOGW(TAG, "[%s] NACK", device);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
