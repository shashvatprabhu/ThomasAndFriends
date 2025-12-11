#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

#define I2C_PORT        I2C_NUM_0
#define SDA_PIN         21
#define SCL_PIN         22
#define I2C_FREQ_HZ     400000

#define MCP4725_ADDR    0x60
#define ADS1115_ADDR    0x48

#define R_FEEDBACK      100000.0f   // 100kΩ

// ---------- I2C INIT ----------
static void i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ
    };
    i2c_param_config(I2C_PORT, &conf);
    i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
}

// ---------- MCP4725: SET VOLTAGE ----------
static esp_err_t mcp4725_set_voltage(float voltage)
{
    if (voltage < 0) voltage = 0;
    if (voltage > 3.3) voltage = 3.3;

    uint16_t code = (uint16_t)((voltage / 3.3) * 4095);
    uint8_t out[3];

    out[0] = 0x40;                 // Fast mode
    out[1] = (code >> 4) & 0xFF;   // high
    out[2] = (code & 0xF) << 4;    // low (4 bits)

    return i2c_master_write_to_device(I2C_PORT, MCP4725_ADDR, out, 3, 10 / portTICK_PERIOD_MS);
}

// ---------- ADS1115: READ CHANNEL ----------
static float ads1115_read_voltage(uint8_t channel)
{
    // Config register: single-shot, channel, gain=±4.096V, 128SPS
    uint16_t config = 0x8583 |
                      (channel << 12);

    uint8_t cfg[3] = { 0x01, config >> 8, config & 0xFF };
    i2c_master_write_to_device(I2C_PORT, ADS1115_ADDR, cfg, 3, 10 / portTICK_PERIOD_MS);

    vTaskDelay(10 / portTICK_PERIOD_MS); // ADC conversion time

    uint8_t data[2];
    i2c_master_write_read_device(I2C_PORT, ADS1115_ADDR,
                                 (uint8_t[]){0x00}, 1,
                                 data, 2,
                                 10 / portTICK_PERIOD_MS);

    int16_t raw = (data[0] << 8) | data[1];

    // Gain = ±4.096V → LSB = 125uV
    return raw * 0.000125f;
}

// ---------- MAIN TEST LOOP ----------
void app_main()
{
    printf("\n=== LSV BASIC TEST ===\n");

    i2c_init();
    printf("I2C OK\n");

    // Test DAC set output (Vset)
    float vset = 2.00f;  // Set RE ≈ 2.00 V
    mcp4725_set_voltage(vset);
    printf("DAC set to %.2f V\n", vset);

    while (1)
    {
        float v_i  = ads1115_read_voltage(0);   // TIA output (A0)
        float vref = ads1115_read_voltage(1);   // Vref (A1)

        float current_ua = ((v_i - vref) / R_FEEDBACK) * 1e6;

        float e_we_re = (vref - vset);  // WE≈Vref, RE≈Vset

        printf("Vset=%.3f  V_I=%.3f  Vref=%.3f  I=%.2f uA  E=%.3f V\n",
               vset, v_i, vref, current_ua, e_we_re);

        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second
    }
}

