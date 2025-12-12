#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_devices.h"
#include "config.h"

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
    if (err != ESP_OK) return err;

    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                             I2C_MASTER_RX_BUF_DISABLE,
                             I2C_MASTER_TX_BUF_DISABLE, 0);
    return err;
}

esp_err_t mcp4725_init(void) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MCP4725_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd,
                                          pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        ret = mcp4725_set_voltage(0);
    }

    return ret;
}

esp_err_t mcp4725_set_voltage(uint16_t dac_value) {
    if (dac_value > DAC_RESOLUTION) {
        dac_value = DAC_RESOLUTION;
    }

    uint8_t data[3];
    data[0] = 0x40;
    data[1] = (dac_value >> 4) & 0xFF;
    data[2] = (dac_value & 0x0F) << 4;

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

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADS1115_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd,
                                          pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) return ret;

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
    uint16_t config;
    return ads1115_read_register(ADS1115_REG_CONFIG, &config);
}

esp_err_t ads1115_read_differential_A0_A1(int16_t *result) {
    uint16_t config = ADS1115_OS_SINGLE |
                      ADS1115_MUX_AIN0_AIN1 |
                      ADS1115_PGA_2_048V |
                      ADS1115_MODE_SINGLE |
                      ADS1115_DR_128SPS |
                      ADS1115_COMP_QUE_DISABLE;

    esp_err_t ret = ads1115_write_register(ADS1115_REG_CONFIG, config);
    if (ret != ESP_OK) return ret;

    int timeout = 50;
    bool conversion_done = false;
    
    vTaskDelay(pdMS_TO_TICKS(2));
    
    while (timeout-- > 0) {
        uint16_t config_read;
        ret = ads1115_read_register(ADS1115_REG_CONFIG, &config_read);
        if (ret != ESP_OK) return ret;
        
        if (config_read & 0x8000) {
            conversion_done = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    if (!conversion_done) {
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
    return (float)counts * (ADS1115_MAX_VOLTAGE / 32768.0f);
}
