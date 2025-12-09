/**
 * @file lsv_scan.cpp
 * @brief Linear Sweep Voltammetry Scan Module Implementation
 * @author Smart India Hackathon 2025 Team
 */

#include "lsv_scan.h"

// ═══════════════════════════════════════════════════════════════════════════
// GLOBAL OBJECTS
// ═══════════════════════════════════════════════════════════════════════════

Adafruit_ADS1115 ads;          // 16-bit ADC for current measurement
Adafruit_MCP4725 dac;          // 12-bit DAC for voltage control
LSVData scanData;              // Global scan data storage

// ═══════════════════════════════════════════════════════════════════════════
// HARDWARE INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

bool initializeHardware() {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║  LSV POTENTIOSTAT - ESP32             ║");
    Serial.println("║  Gold Purity Analyzer                 ║");
    Serial.println("║  Smart India Hackathon 2025           ║");
    Serial.println("╚═══════════════════════════════════════╝\n");

    // Initialize I2C bus
    Serial.println("Initializing I2C bus...");
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_FREQUENCY);
    Serial.printf("✓ I2C initialized on GPIO%d (SDA) and GPIO%d (SCL)\n",
                  I2C_SDA_PIN, I2C_SCL_PIN);
    Serial.printf("  Clock frequency: %d Hz\n\n", I2C_FREQUENCY);

    // Initialize ADS1115 ADC
    Serial.println("Connecting to ADS1115 ADC...");
    if (!ads.begin(ADS1115_ADDR)) {
        Serial.println("❌ ERROR: ADS1115 not detected at address 0x48");
        Serial.println("   Troubleshooting:");
        Serial.println("   • Check SDA connection to GPIO21");
        Serial.println("   • Check SCL connection to GPIO22");
        Serial.println("   • Verify VDD connected to 3.3V");
        Serial.println("   • Verify GND connected to ground");
        Serial.println("   • Check I2C pull-up resistors (4.7kΩ recommended)");
        return false;
    }
    Serial.printf("✓ ADS1115 found at address 0x%02X\n", ADS1115_ADDR);

    // Set ADC gain for optimal range
    ads.setGain(ADC_GAIN);
    Serial.printf("✓ ADC gain set to ±%.3fV range\n", ADC_MAX_VOLTAGE);
    float lsb = ADC_MAX_VOLTAGE / 32768.0;
    Serial.printf("  Resolution: %.4f mV per bit\n", lsb * 1000);
    Serial.printf("  Current resolution: %.3f nA\n\n", (lsb / R_FEEDBACK) * 1e9);

    // Initialize MCP4725 DAC
    Serial.println("Connecting to MCP4725 DAC...");
    if (!dac.begin(MCP4725_ADDR)) {
        Serial.println("❌ ERROR: MCP4725 not detected at address 0x60");
        Serial.println("   Troubleshooting:");
        Serial.println("   • Check SDA connection to GPIO21");
        Serial.println("   • Check SCL connection to GPIO22");
        Serial.println("   • Verify VDD connected to 3.3V");
        Serial.println("   • Verify GND connected to ground");
        return false;
    }
    Serial.printf("✓ MCP4725 found at address 0x%02X\n", MCP4725_ADDR);

    // Set initial DAC output to 0V (safe state)
    setDACVoltage(0.0);
    Serial.println("✓ DAC output set to 0.000V (safe state)\n");

    // Initialize scan data
    clearScanData();

    Serial.println("System ready!\n");
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// DAC CONTROL
// ═══════════════════════════════════════════════════════════════════════════

bool setDACVoltage(float voltage) {
    // Validate voltage range
    if (voltage < 0.0 || voltage > DAC_VREF) {
        Serial.printf("⚠️  WARNING: Voltage %.3fV out of range [0, %.1fV]\n",
                      voltage, DAC_VREF);
        return false;
    }

    // Convert voltage to DAC value
    // Formula: DAC_value = (V_desired / V_ref) × DAC_resolution
    uint16_t dacValue = (uint16_t)((voltage / DAC_VREF) * DAC_RESOLUTION);

    // Set DAC output
    dac.setVoltage(dacValue, false); // false = don't write to EEPROM

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// CURRENT MEASUREMENT
// ═══════════════════════════════════════════════════════════════════════════

float readCurrent() {
    // Read voltage from ADC channel A0 (transimpedance amplifier output)
    int16_t adcValue = ads.readADC_SingleEnded(0);
    float voltage = ads.computeVolts(adcValue);

    // Convert voltage to current using Ohm's law
    // I = V / R (where R is the feedback resistor)
    float current = voltage / R_FEEDBACK;

    // Convert to microamps
    current *= 1e6;

    // Invert sign if using inverting op-amp configuration
    if (INVERT_CURRENT) {
        current = -current;
    }

    return current;
}

// ═══════════════════════════════════════════════════════════════════════════
// BASELINE MEASUREMENT
// ═══════════════════════════════════════════════════════════════════════════

float measureBaseline() {
    Serial.println("Measuring baseline current at 0V...");

    setDACVoltage(0.0);
    delay(200); // Allow settling

    // Average multiple readings for better accuracy
    float sum = 0;
    const int numReadings = 10;

    for (int i = 0; i < numReadings; i++) {
        sum += readCurrent();
        delay(10);
    }

    float baseline = sum / numReadings;
    Serial.printf("Baseline current: %.3f µA\n", baseline);

    // Check for high baseline (possible short circuit)
    if (abs(baseline) > HIGH_BASELINE_WARN) {
        Serial.println("⚠️  WARNING: High baseline current detected!");
        Serial.println("   Possible causes:");
        Serial.println("   • Short circuit between electrodes");
        Serial.println("   • Contaminated electrolyte");
        Serial.println("   • Check electrode positioning\n");
    }

    return baseline;
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN LSV SCAN FUNCTION
// ═══════════════════════════════════════════════════════════════════════════

bool performLSVScan() {
    Serial.println("═══════════════════════════════════════");
    Serial.println("Starting LSV Scan");
    Serial.println("═══════════════════════════════════════");
    Serial.printf("Voltage range: %.3fV to %.3fV\n", START_VOLTAGE, END_VOLTAGE);
    Serial.printf("Step size: %.3fV\n", STEP_SIZE);
    Serial.printf("Settle time: %dms per step\n", SETTLE_TIME_MS);

    // Calculate number of points
    int numPoints = (int)((END_VOLTAGE - START_VOLTAGE) / STEP_SIZE) + 1;

    if (numPoints > MAX_DATA_POINTS) {
        Serial.printf("❌ ERROR: Too many data points (%d > %d)\n",
                      numPoints, MAX_DATA_POINTS);
        Serial.println("   Increase STEP_SIZE or reduce voltage range");
        return false;
    }

    Serial.printf("Total data points: %d\n", numPoints);
    Serial.printf("Estimated scan time: %.1f seconds\n\n",
                  (numPoints * SETTLE_TIME_MS) / 1000.0);

    // Measure baseline if enabled
    if (ENABLE_BASELINE_SUB) {
        scanData.baselineCurrent = measureBaseline();
    } else {
        scanData.baselineCurrent = 0.0;
    }

    // Clear previous data
    clearScanData();

    // Print table header
    printScanHeader();

    // Perform voltage sweep
    int dataIndex = 0;
    bool errorDetected = false;

    for (float voltage = START_VOLTAGE;
         voltage <= END_VOLTAGE && dataIndex < MAX_DATA_POINTS;
         voltage += STEP_SIZE) {

        // Set DAC to target voltage
        if (!setDACVoltage(voltage)) {
            errorDetected = true;
            break;
        }

        // Wait for electrochemical equilibration
        delay(SETTLE_TIME_MS);

        // Read current
        float current = readCurrent();

        // Apply baseline subtraction if enabled
        if (ENABLE_BASELINE_SUB) {
            current -= scanData.baselineCurrent;
        }

        // Store data
        scanData.voltage[dataIndex] = voltage;
        scanData.current[dataIndex] = current;
        dataIndex++;

        // Print data point (every Nth point to reduce clutter)
        if (dataIndex % PRINT_INTERVAL == 0 || dataIndex == 1) {
            printDataPoint(dataIndex - 1, abs(current) > 1.0);
        }

        // Error checking
        if (abs(current) > MAX_REALISTIC_I) {
            Serial.printf("\n⚠️  WARNING: Unrealistic current detected (%.1f µA)\n",
                          current);
            Serial.println("   Check for circuit problems\n");
        }
    }

    scanData.numPoints = dataIndex;
    scanData.scanComplete = !errorDetected;

    // Return to safe state
    returnToZero();

    Serial.println("────────────────────────────────────────");
    Serial.printf("Scan complete - %d data points collected\n", dataIndex);
    Serial.println("════════════════════════════════════════\n");

    // Export CSV if enabled
    if (ENABLE_CSV_OUTPUT) {
        exportCSV();
    }

    return !errorDetected;
}

// ═══════════════════════════════════════════════════════════════════════════
// DATA MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

void clearScanData() {
    scanData.numPoints = 0;
    scanData.baselineCurrent = 0.0;
    scanData.scanComplete = false;
    // Arrays don't need explicit clearing (will be overwritten)
}

void printScanHeader() {
    Serial.println("\nVoltage (V) | Current (µA) | Notes");
    Serial.println("────────────┼──────────────┼─────────────");
}

void printDataPoint(int index, bool highlight) {
    char note[30] = "";

    float current = scanData.current[index];
    float voltage = scanData.voltage[index];

    // Add notes for interesting features
    if (abs(current) > 3.0) {
        sprintf(note, "▲ Peak detected");
    } else if (abs(current) > 1.0) {
        sprintf(note, "Rising");
    }

    // Format output
    Serial.printf("   %5.3f    |   %7.3f    | %s\n",
                  voltage, current, note);
}

void exportCSV() {
    Serial.println("\n═══ CSV DATA EXPORT ═══");
    Serial.println("Voltage,Current");

    for (int i = 0; i < scanData.numPoints; i++) {
        Serial.printf("%.4f,%.4f\n",
                      scanData.voltage[i],
                      scanData.current[i]);
    }

    Serial.println("═══ END CSV DATA ═══\n");
}

void returnToZero() {
    setDACVoltage(0.0);
    delay(100);
}

bool errorCheck() {
    // Check for all-zero readings (disconnected electrode)
    if (ZERO_CURRENT_WARN) {
        bool allZero = true;
        for (int i = 0; i < scanData.numPoints; i++) {
            if (abs(scanData.current[i]) > 0.01) {
                allZero = false;
                break;
            }
        }

        if (allZero) {
            Serial.println("⚠️  WARNING: All current readings near zero");
            Serial.println("   Possible causes:");
            Serial.println("   • Working electrode disconnected");
            Serial.println("   • Electrodes not in electrolyte");
            Serial.println("   • No electrochemical reaction occurring\n");
            return false;
        }
    }

    return true;
}
