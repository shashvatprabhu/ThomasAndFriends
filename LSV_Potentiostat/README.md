# LSV Potentiostat - ESP32-WROOM-32E (ESP-IDF)

**Native ESP32 C Application for Gold Purity Testing**

Linear Sweep Voltammetry system using ESP-IDF framework - **no Arduino framework!**

---

## 🎯 Overview

Electrochemical gold purity analyzer that sweeps voltage across a 3-electrode cell and measures oxidation currents to identify alloy metals (Cu, Ag) in gold samples.

**Hardware:** ESP32-WROOM-32E + MCP4725 DAC + ADS1115 ADC + LM358 Op-Amps

**Framework:** ESP-IDF (Espressif IoT Development Framework) - Pure C

---

## ⚡ Quick Start

### 1. Install ESP-IDF

```bash
# Clone ESP-IDF
mkdir -p ~/esp && cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && git checkout v5.1
./install.sh esp32

# Set environment (run in every terminal)
. ~/esp/esp-idf/export.sh
```

### 2. Build & Flash

```bash
cd LSV_Potentiostat

# Configure for ESP32
idf.py set-target esp32

# Build
idf.py build

# Flash to ESP32-WROOM-32E
idf.py flash

# Monitor output
idf.py monitor
```

**Done!** System starts scanning automatically after 10-second countdown.

---

## 📁 Project Structure

```
LSV_Potentiostat/
├── CMakeLists.txt              # ESP-IDF project config
├── main/
│   ├── main.c                  # Entry point (app_main)
│   ├── config.h                # All tunable parameters ⭐
│   ├── i2c_devices.c/h         # MCP4725 & ADS1115 drivers
│   ├── lsv_scan.c/h            # Voltage sweep logic
│   ├── peak_analysis.c/h       # Cu/Ag peak detection
│   └── karat_estimation.c/h    # Karat classification
└── build/                      # Generated build files
```

---

## 🔌 Hardware Connections

| ESP32 GPIO | Connection |
|------------|------------|
| GPIO 21 | I2C SDA (MCP4725 + ADS1115) |
| GPIO 22 | I2C SCL (MCP4725 + ADS1115) |
| 3.3V | VDD (all modules) |
| GND | GND (all modules) |

**Electrochemical Cell:**
- **WE (Working):** Gold sample being tested
- **RE (Reference):** Copper wire (voltage reference)
- **CE (Counter):** Copper wire (current return)
- **Electrolyte:** 0.1M KCl or 0.5M H₂SO₄

See `CIRCUIT_DETAILS.md` for complete circuit diagrams.

---

## ⚙️ Configuration

Edit `main/config.h` to customize:

```c
// Scan parameters
#define START_VOLTAGE    0.0f      // Starting voltage (V)
#define END_VOLTAGE      1.2f      // Ending voltage (V)
#define STEP_SIZE        0.01f     // Voltage step (V)
#define SETTLE_TIME_MS   80        // Wait per step (ms)

// Peak detection windows
#define CU_PEAK_V_MIN    0.00f     // Copper: 0.0-0.25V
#define CU_PEAK_V_MAX    0.25f
#define AG_PEAK_V_MIN    0.30f     // Silver: 0.3-0.55V
#define AG_PEAK_V_MAX    0.55f

// Karat classification thresholds (calibrate these!)
#define KARAT_24_MAX     0.5f      // 24K: <0.5µA
#define KARAT_22_MAX     2.0f      // 22K: 0.5-2.0µA
#define KARAT_18_MAX     5.0f      // 18K: 2.0-5.0µA
```

After editing: `idf.py build flash`

---

## 📊 Expected Output

```
I (123) MAIN: ╔═══════════════════════════════════════╗
I (123) MAIN: ║  LSV POTENTIOSTAT - ESP32-WROOM-32E   ║
I (123) MAIN: ║  Gold Purity Analyzer                 ║
I (123) MAIN: ║  Smart India Hackathon 2025           ║
I (123) MAIN: ╚═══════════════════════════════════════╝

I (234) I2C_DEVICES: MCP4725 found at address 0x60
I (245) I2C_DEVICES: ADS1115 found at address 0x48
I (256) LSV_SCAN: System ready!

I (10345) LSV_SCAN: Starting LSV Scan
I (10345) LSV_SCAN: Voltage range: 0.000V to 1.200V

Voltage (V) | Current (µA) | Notes
────────────┼──────────────┼─────────────
   0.000    |    0.02      |
   0.050    |    0.48      |
   0.100    |    4.67      | ▲ Peak detected
   0.400    |    2.45      | ▲ Peak detected
   ...

═══════════════════════════════════════
PEAK ANALYSIS
═══════════════════════════════════════

✓ COPPER detected: 4.67 µA peak at 0.100V
  → Indicates copper-containing alloy

✓ SILVER detected: 2.45 µA peak at 0.400V
  → Indicates silver-containing alloy

═══════════════════════════════════════
KARAT ESTIMATION
═══════════════════════════════════════

Estimated purity: 18K (75.0% gold)
Classification: Medium Purity Gold (Jewelry Grade)
Confidence level: 72%
```

---

## 🔧 Calibration

**CRITICAL:** Default thresholds are estimates. Calibrate with known samples!

### Procedure:

1. **Test known gold standards:** 14K, 18K, 22K, 24K
2. **Run 3 scans per sample** and record peak heights
3. **Update thresholds** in `main/config.h`:
   ```c
   #define KARAT_24_MAX     [your_24K_max]
   #define KARAT_22_MAX     [your_22K_max]
   #define KARAT_18_MAX     [your_18K_max]
   ```
4. **Rebuild and flash:** `idf.py build flash`

---

## 🐛 Troubleshooting

### "MCP4725/ADS1115 not found"
- Check GPIO21 (SDA) and GPIO22 (SCL) connections
- Verify 3.3V and GND power
- Test I2C with multimeter (pull-ups should be ~4.7kΩ)

### All current readings zero
- Electrodes not submerged in electrolyte
- Working electrode disconnected
- Check 100kΩ feedback resistor on LM358

### Build errors
```bash
idf.py fullclean
idf.py build
```

### Port not found
```bash
# Linux: Add user to dialout group
sudo usermod -a -G dialout $USER

# Then log out and back in

# MacOS: Port is /dev/cu.usbserial-*
idf.py -p /dev/cu.usbserial-XXXX flash

# Windows: Check Device Manager for COM port
idf.py -p COM3 flash
```

---

## 📚 Documentation Files

| File | Purpose |
|------|---------|
| **BUILD_INSTRUCTIONS.md** | Detailed ESP-IDF build guide |
| **CIRCUIT_DETAILS.md** | Circuit theory & design |
| **PROJECT_STRUCTURE.md** | Code architecture |
| **PROJECT_SUMMARY.txt** | High-level overview |

---

## 🔬 Technical Specs

| Parameter | Value |
|-----------|-------|
| **Voltage Range** | 0V to 1.2V (configurable) |
| **Voltage Resolution** | 0.8mV (12-bit DAC) |
| **Current Range** | ±100µA typical |
| **Current Resolution** | 0.625nA (16-bit ADC) |
| **Scan Speed** | 80ms per step |
| **Total Scan Time** | 15-30 seconds |
| **I2C Frequency** | 400kHz (fast mode) |

**Detection Capability:**
- Copper (Cu): Peak at 0.0-0.2V
- Silver (Ag): Peak at 0.3-0.5V
- Karat estimation: 24K, 22K, 18K, 14K

---

## 🎓 Electrochemistry Principles

**Why different metals show peaks at different voltages:**

Different metals oxidize at characteristic voltages (electrochemical fingerprint):

| Metal | Oxidation Reaction | Peak Voltage |
|-------|-------------------|--------------|
| Copper (Cu) | Cu → Cu²⁺ + 2e⁻ | ~0.1V |
| Silver (Ag) | Ag → Ag⁺ + e⁻ | ~0.4V |
| Gold (Au) | Au → Au³⁺ + 3e⁻ | ~1.5V (outside scan range) |

**Key Insight:** Gold oxidizes at 1.5V, above our scan range (0-1.2V). Pure gold shows NO peaks. Alloy metals oxidize in our range, creating peaks proportional to concentration.

**Karat determination:**
- 24K (pure): No alloy → No peaks
- 18K (75% Au): 25% alloy → Medium peaks
- 14K (58% Au): 42% alloy → Large peaks

---

## ⚠️ Limitations

**This is an EDUCATIONAL/DEMONSTRATION system:**

✅ Good for:
- Hackathon projects
- Educational demonstrations
- Preliminary screening
- Learning electrochemistry

❌ NOT suitable for:
- Regulatory compliance testing
- Commercial gold transactions
- Legal assay requirements
- High-value certification

**Accuracy:** ±1-2 karats after calibration (typical)

**Professional alternatives:**
- X-ray Fluorescence (XRF): $10K-$50K
- Fire Assay: Most accurate (destructive)
- Commercial Potentiostat: $5K-$20K

---

## 🏆 Project Stats

- **Code:** 1,500+ lines of pure C
- **Framework:** ESP-IDF (no Arduino)
- **Total Cost:** <$50 hardware
- **Setup Time:** 30 minutes
- **Scan Time:** 15-30 seconds per sample

---

## 🔗 Useful Commands

```bash
# Build
idf.py build

# Flash and monitor
idf.py flash monitor

# Clean rebuild
idf.py fullclean build

# Erase flash
idf.py erase-flash

# Change log level
idf.py menuconfig → Component config → Log output → Default log level

# Exit monitor
Ctrl + ]
```

---

## 📞 ESP-IDF Resources

- ESP-IDF Docs: https://docs.espressif.com/projects/esp-idf/
- I2C Driver API: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html
- FreeRTOS: https://www.freertos.org/

---

**Ready to use!** Build, flash, and start testing gold samples. 🚀

**Smart India Hackathon 2025** | ESP32-WROOM-32E | Native C | No Arduino Bullshit!
