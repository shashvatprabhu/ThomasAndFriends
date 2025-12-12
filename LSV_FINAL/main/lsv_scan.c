#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lsv_scan.h"
#include "i2c_devices.h"
#include "config.h"

lsv_data_t g_scan_data = {0};
static int16_t g_last_adc_diff = 0;
static float g_last_v_diff = 0.0f;
static int g_last_sat = 0; // 1 = ADC saturated/invalid, 0 = OK
static float g_last_dac_v = 0.0f;
static int g_last_tia_sat = 0; // 1 = TIA output likely railed, 0 = OK
static int g_last_adc_sat_printed = 0;

esp_err_t lsv_init_hardware(void) {
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK) return ret;

    ret = mcp4725_init();
    if (ret != ESP_OK) return ret;

    ret = ads1115_init();
    if (ret != ESP_OK) return ret;

    lsv_set_voltage(0.0f);
    lsv_clear_data();
    return ESP_OK;
}

esp_err_t lsv_set_voltage(float voltage) {
    // Sweep is expressed relative to CELL_BIAS_V (mid-supply reference).
    float dac_voltage = CELL_BIAS_V + voltage;
    if (dac_voltage < 0.0f || dac_voltage > DAC_VREF) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t dac_value = (uint16_t)((dac_voltage / DAC_VREF) * DAC_RESOLUTION);
    g_last_dac_v = dac_voltage;
    return mcp4725_set_voltage(dac_value);
}

float lsv_read_current(void) {
    int16_t adc_diff;
    
    esp_err_t ret = ads1115_read_differential_A0_A1(&adc_diff);
    if (ret != ESP_OK) {
        g_last_adc_diff = 0;
        g_last_v_diff = 0.0f;
        g_last_sat = 1;
        if (!g_last_adc_sat_printed) {
            g_last_adc_sat_printed = 1;
            printf("#SAT,ADC,read_failed\n");
        }
        return 0.0f;
    }

    float v_diff = ads1115_counts_to_volts(adc_diff);
    g_last_adc_diff = adc_diff;
    g_last_v_diff = v_diff;

    if (adc_diff == -32768 || adc_diff == 32767) {
        g_last_sat = 1;
        if (!g_last_adc_sat_printed) {
            g_last_adc_sat_printed = 1;
            printf("#SAT,ADC,counts=%d\n", (int)adc_diff);
        }
        return 0.0f;
    }
    g_last_sat = 0;
    if (g_last_adc_sat_printed) {
        g_last_adc_sat_printed = 0;
        printf("#SAT_CLEAR,ADC\n");
    }

    // If A1 is tied to VREF (CELL_BIAS_V), then Vout_TIA ~= VREF + (A0-A1).
    // LM358 @ 3.3V typically cannot swing to rails; ~0.05V low and ~1.97V high are practical limits.
    const float v_tia_out = CELL_BIAS_V + v_diff;
    const int tia_sat = (v_tia_out < 0.05f) || (v_tia_out > 1.95f);
    if (tia_sat != g_last_tia_sat) {
        g_last_tia_sat = tia_sat;
        if (tia_sat) {
            // Print once when saturation begins. Prefix with # so CSV parsers can ignore it.
            printf("#SAT,TIA_OUT,%.3fV (VREF=%.3f,diff=%.6f)\n", v_tia_out, CELL_BIAS_V, v_diff);
        } else {
            printf("#SAT_CLEAR,TIA_OUT\n");
        }
    }

    float current = v_diff / R_FEEDBACK;
    current *= 1e6f;

    return current;
}

esp_err_t lsv_perform_scan(void) {
    int num_points = (int)((END_VOLTAGE - START_VOLTAGE) / STEP_SIZE) + 1;

    if (num_points > MAX_DATA_POINTS) {
        return ESP_ERR_INVALID_SIZE;
    }

    lsv_clear_data();

    int data_idx = 0;

    for (float voltage = START_VOLTAGE;
         voltage <= END_VOLTAGE && data_idx < MAX_DATA_POINTS;
         voltage += STEP_SIZE) {

        lsv_set_voltage(voltage);
        vTaskDelay(pdMS_TO_TICKS(SETTLE_TIME_MS));

        float current = lsv_read_current();

        g_scan_data.voltage[data_idx] = voltage;
        g_scan_data.current[data_idx] = current;
        data_idx++;

        // CSV: sweep_V, dac_V, I_uA, adc_diff_counts, adc_diff_V, sat_adc(0/1), sat_tia(0/1)
        printf("%.3f,%.3f,%.3f,%d,%.6f,%d,%d\n",
               voltage, g_last_dac_v, current, (int)g_last_adc_diff, g_last_v_diff, g_last_sat, g_last_tia_sat);
    }

    g_scan_data.num_points = data_idx;
    g_scan_data.scan_complete = true;
    lsv_return_to_zero();

    return ESP_OK;
}

void lsv_clear_data(void) {
    memset(&g_scan_data, 0, sizeof(lsv_data_t));
}

void lsv_return_to_zero(void) {
    lsv_set_voltage(0.0f);
    vTaskDelay(pdMS_TO_TICKS(100));
}
