/**
 * @file LSV_Potentiostat.ino
 * @brief Main Arduino Sketch for ESP32-based LSV Gold Purity Analyzer
 * @author Smart India Hackathon 2025 Team
 * @date 2025-12-09
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * PROJECT: Linear Sweep Voltammetry (LSV) Potentiostat for Gold Purity Testing
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * DESCRIPTION:
 * This system performs electrochemical analysis of gold samples using Linear
 * Sweep Voltammetry. By sweeping voltage across a 3-electrode cell and
 * measuring oxidation currents, it detects alloy metals (Cu, Ag) in gold
 * to estimate karat purity.
 *
 * HARDWARE:
 * - ESP32 Development Board
 * - MCP4725 12-bit DAC (voltage control)
 * - ADS1115 16-bit ADC (current measurement)
 * - LM358 Dual Op-Amp (control + transimpedance amplifier)
 * - 3-electrode electrochemical cell (WE, RE, CE)
 *
 * ELECTROCHEMICAL PRINCIPLE:
 * - Different metals oxidize at characteristic voltages
 * - Cu: ~0.1V, Ag: ~0.4V, Au: ~1.5V (vs Cu reference)
 * - Peak height proportional to alloy metal concentration
 * - Higher alloy content = lower karat gold
 *
 * USAGE:
 * 1. Upload this sketch to ESP32
 * 2. Open Serial Monitor at 115200 baud
 * 3. Prepare electrochemical cell with electrolyte (0.1M KCl or 0.5M H₂SO₄)
 * 4. Attach gold sample as working electrode
 * 5. System automatically runs LSV scans every 30 seconds
 * 6. View real-time analysis and karat estimation
 *
 * CALIBRATION:
 * - Test known gold standards (14K, 18K, 22K, 24K)
 * - Update threshold values in include/config.h
 * - See README.md for detailed calibration procedure
 *
 * ═══════════════════════════════════════════════════════════════════════════
 */

// ═══════════════════════════════════════════════════════════════════════════
// INCLUDES
// ═══════════════════════════════════════════════════════════════════════════

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MCP4725.h>

// Project-specific modules
#include "include/config.h"
#include "include/lsv_scan.h"
#include "include/peak_analysis.h"
#include "include/karat_estimation.h"

// ═══════════════════════════════════════════════════════════════════════════
// GLOBAL VARIABLES
// ═══════════════════════════════════════════════════════════════════════════

PeakAnalysisResult peakResults;    // Stores detected peaks
KaratEstimate karatEstimate;       // Stores karat estimation
unsigned long lastScanTime = 0;    // Timestamp of last scan
int scanCounter = 0;               // Total number of scans performed

// ═══════════════════════════════════════════════════════════════════════════
// ARDUINO SETUP - Runs once at startup
// ═══════════════════════════════════════════════════════════════════════════

void setup() {
    // Initialize serial communication
    Serial.begin(SERIAL_BAUD);
    delay(1000); // Allow serial to stabilize

    // Print welcome banner
    printWelcomeBanner();

    // Initialize hardware (I2C, DAC, ADC)
    if (!initializeHardware()) {
        Serial.println("\n❌ FATAL ERROR: Hardware initialization failed");
        Serial.println("System halted. Please check connections and reset.\n");
        while (1) {
            delay(1000); // Infinite loop (halt execution)
        }
    }

    // Print pre-scan checklist
    printPreScanChecklist();

    // Countdown before first scan
    Serial.printf("Starting first scan in %d seconds...\n", STARTUP_DELAY_S);
    for (int i = STARTUP_DELAY_S; i > 0; i--) {
        Serial.printf("  %d...\n", i);
        delay(1000);
    }
    Serial.println("  GO!\n");

    lastScanTime = millis();
}

// ═══════════════════════════════════════════════════════════════════════════
// ARDUINO LOOP - Runs continuously
// ═══════════════════════════════════════════════════════════════════════════

void loop() {
    unsigned long currentTime = millis();

    // Check if it's time for next scan
    if (currentTime - lastScanTime >= SCAN_INTERVAL_MS || scanCounter == 0) {
        scanCounter++;

        // Print scan header
        Serial.println("\n\n");
        Serial.println("╔═══════════════════════════════════════════════════════════╗");
        Serial.printf("║  SCAN #%-3d                                               ║\n", scanCounter);
        Serial.println("╚═══════════════════════════════════════════════════════════╝");
        Serial.println();

        // Step 1: Perform LSV scan
        bool scanSuccess = performLSVScan();

        if (!scanSuccess) {
            Serial.println("❌ Scan failed - skipping analysis\n");
            returnToZero();
            lastScanTime = currentTime;
            return;
        }

        // Step 2: Check for errors
        if (!errorCheck()) {
            Serial.println("⚠️  Scan completed with warnings\n");
        }

        // Step 3: Analyze peaks
        bool analysisSuccess = analyzePeaks(&peakResults);

        if (!analysisSuccess) {
            Serial.println("❌ Peak analysis failed\n");
            returnToZero();
            lastScanTime = currentTime;
            return;
        }

        // Step 4: Estimate karat
        estimateKarat(&peakResults, &karatEstimate);

        // Step 5: Return to safe state
        returnToZero();

        // Print next scan info
        Serial.println("\n╔═══════════════════════════════════════════════════════════╗");
        Serial.println("║  SCAN COMPLETE                                            ║");
        Serial.println("╚═══════════════════════════════════════════════════════════╝");
        Serial.printf("\nNext scan in %d seconds...\n", SCAN_INTERVAL_MS / 1000);
        Serial.println("(You may remove/replace sample between scans)\n");
        Serial.println("════════════════════════════════════════════════════════════\n\n");

        // Update last scan time
        lastScanTime = currentTime;
    }

    // Small delay to prevent tight loop
    delay(100);
}

// ═══════════════════════════════════════════════════════════════════════════
// HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

void printWelcomeBanner() {
    Serial.println("\n\n\n");
    Serial.println("╔═══════════════════════════════════════════════════════════╗");
    Serial.println("║                                                           ║");
    Serial.println("║        LINEAR SWEEP VOLTAMMETRY POTENTIOSTAT              ║");
    Serial.println("║        Gold Purity Analyzer - ESP32 Based                ║");
    Serial.println("║                                                           ║");
    Serial.println("║        Smart India Hackathon 2025                         ║");
    Serial.println("║        Non-Destructive Gold Testing System                ║");
    Serial.println("║                                                           ║");
    Serial.println("╚═══════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("Firmware Version: 1.0.0");
    Serial.println("Build Date: 2025-12-09");
    Serial.println();
}

void printPreScanChecklist() {
    Serial.println("═══════════════════════════════════════════════════════════");
    Serial.println("PRE-SCAN CHECKLIST");
    Serial.println("═══════════════════════════════════════════════════════════");
    Serial.println();
    Serial.println("Please verify before starting scan:");
    Serial.println();
    Serial.println("☐ Electrodes properly positioned");
    Serial.println("  • Working Electrode (WE): Gold sample or test electrode");
    Serial.println("  • Reference Electrode (RE): Copper wire or graphite");
    Serial.println("  • Counter Electrode (CE): Copper wire or graphite");
    Serial.println();
    Serial.println("☐ Electrodes submerged in electrolyte");
    Serial.println("  • Depth: 2-3 cm below surface");
    Serial.println("  • Electrolyte: 0.1M KCl or 0.5M H₂SO₄");
    Serial.println("  • Volume: At least 50-100 mL");
    Serial.println();
    Serial.println("☐ Electrodes NOT touching each other");
    Serial.println("  • Maintain 1-2 cm spacing between electrodes");
    Serial.println("  • Check for shorts with multimeter if available");
    Serial.println();
    Serial.println("☐ Beaker on stable, non-conductive surface");
    Serial.println("  • Prevent spills and electrical shorts");
    Serial.println("  • Keep away from ESP32 and other electronics");
    Serial.println();
    Serial.println("☐ Serial Monitor ready");
    Serial.println("  • Baud rate: 115200");
    Serial.println("  • Line ending: Newline or Both NL & CR");
    Serial.println();
    Serial.println("═══════════════════════════════════════════════════════════");
    Serial.println();

    // Wait for user acknowledgment (optional - comment out if not needed)
    // Serial.println("Press any key to continue...");
    // while (!Serial.available()) { delay(100); }
    // while (Serial.available()) { Serial.read(); } // Clear buffer
}

// ═══════════════════════════════════════════════════════════════════════════
// ADVANCED FEATURES (Optional - uncomment to enable)
// ═══════════════════════════════════════════════════════════════════════════

/*
 * MULTI-SCAN AVERAGING:
 * Uncomment this function and call it from loop() to enable automatic
 * averaging of multiple scans for improved accuracy.
 */

/*
void performMultiScanAverage() {
    const int numScans = NUM_SCANS_AVG;
    float avgCuPeak = 0.0;
    float avgAgPeak = 0.0;

    Serial.printf("Performing %d scans for averaging...\n\n", numScans);

    for (int i = 0; i < numScans; i++) {
        Serial.printf("--- Scan %d of %d ---\n", i + 1, numScans);

        performLSVScan();
        analyzePeaks(&peakResults);

        avgCuPeak += peakResults.copper.peakHeight;
        avgAgPeak += peakResults.silver.peakHeight;

        if (i < numScans - 1) {
            delay(5000); // Wait between scans
        }
    }

    // Calculate averages
    avgCuPeak /= numScans;
    avgAgPeak /= numScans;

    Serial.println("\n═══════════════════════════════════════");
    Serial.println("AVERAGED RESULTS");
    Serial.println("═══════════════════════════════════════");
    Serial.printf("Average Cu peak: %.3f µA\n", avgCuPeak);
    Serial.printf("Average Ag peak: %.3f µA\n", avgAgPeak);
    Serial.println("═══════════════════════════════════════\n");

    // Use averaged values for karat estimation
    peakResults.copper.peakHeight = avgCuPeak;
    peakResults.silver.peakHeight = avgAgPeak;
    peakResults.totalMagnitude = avgCuPeak + avgAgPeak;

    estimateKarat(&peakResults, &karatEstimate);
}
*/

// ═══════════════════════════════════════════════════════════════════════════
// END OF SKETCH
// ═══════════════════════════════════════════════════════════════════════════
