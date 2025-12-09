/**
 * @file lsv_scan.h
 * @brief LSV scanning functionality for ESP32
 */

#ifndef LSV_SCAN_H
#define LSV_SCAN_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// ═══════════════════════════════════════════════════════════════════════════
// DATA STRUCTURES
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    float voltage[300];
    float current[300];
    int num_points;
    float baseline_current;
    bool scan_complete;
} lsv_data_t;

// Global scan data
extern lsv_data_t g_scan_data;

// ═══════════════════════════════════════════════════════════════════════════
// FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

esp_err_t lsv_init_hardware(void);
esp_err_t lsv_perform_scan(void);
esp_err_t lsv_set_voltage(float voltage);
float lsv_read_current(void);
void lsv_clear_data(void);
void lsv_return_to_zero(void);

#endif // LSV_SCAN_H
