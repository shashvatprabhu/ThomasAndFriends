# LSV Potentiostat - Gold Purity Analyzer

**ESP32-Based Linear Sweep Voltammetry System for Non-Destructive Gold Testing**

*Smart India Hackathon 2025 Project*

---

## 📋 Table of Contents

1. [Overview](#overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Circuit Diagram](#circuit-diagram)
4. [Pin Connections](#pin-connections)
5. [Software Setup](#software-setup)
6. [Usage Instructions](#usage-instructions)
7. [Calibration Procedure](#calibration-procedure)
8. [Understanding Results](#understanding-results)
9. [Troubleshooting](#troubleshooting)
10. [Theory of Operation](#theory-of-operation)
11. [References](#references)

---

## 🎯 Overview

This project implements a **Linear Sweep Voltammetry (LSV)** potentiostat for analyzing gold purity using electrochemical techniques. By measuring oxidation currents of alloy metals (copper, silver) in gold samples, the system estimates karat purity non-destructively.

### Key Features

- ✅ **Non-destructive testing** - Sample remains intact
- ✅ **Fast analysis** - Results in ~15-30 seconds
- ✅ **Low-cost** - Total hardware cost <$50
- ✅ **Portable** - Battery-powered ESP32 system
- ✅ **Real-time data** - Live serial monitor output
- ✅ **Educational** - Well-documented code for learning

### Technical Specifications

| Parameter | Value |
|-----------|-------|
| Voltage Range | 0V to 1.2V (configurable) |
| Voltage Resolution | 0.8 mV (12-bit DAC) |
| Current Range | ±100 µA typical |
| Current Resolution | 0.625 nA (16-bit ADC) |
| Scan Speed | 80ms per step (configurable) |
| Total Scan Time | ~15-30 seconds |

---

## 🔧 Hardware Requirements

### Electronic Components

| Component | Quantity | Purpose | Notes |
|-----------|----------|---------|-------|
| ESP32 Development Board | 1 | Microcontroller | 3.3V logic, I2C capable |
| MCP4725 12-bit DAC | 1 | Voltage control | I2C address 0x60 |
| ADS1115 16-bit ADC | 1 | Current measurement | I2C address 0x48 |
| LM358 Dual Op-Amp | 1 | Control + I-to-V amplifier | DIP-8 package |
| 100kΩ Resistor | 1 | Feedback resistor (Rf) | 1% tolerance, brown-black-yellow-gold |
| 100µF Capacitor | 1 | Power supply decoupling | Electrolytic |
| 100nF Capacitor | 2 | Op-amp decoupling | Ceramic |
| Breadboard | 1 | Prototyping | Full-size recommended |
| Jumper Wires | ~20 | Connections | Male-to-male |

### Electrochemical Components

| Item | Quantity | Purpose | Notes |
|------|----------|---------|-------|
| Copper Wire | 2 pieces | RE and CE electrodes | 1-2mm diameter, 10cm length |
| Gold Sample | 1 | Working electrode (WE) | Jewelry, coin, or scrap |
| Alligator Clips | 3 | Electrode connections | Insulated |
| Beaker (100mL) | 1 | Electrolyte container | Glass or plastic |
| Electrolyte | 50-100mL | Conducting solution | 0.1M KCl or 0.5M H₂SO₄ |

### Optional Components

- **Graphite rod** (alternative to Cu for RE/CE)
- **Platinum wire** (for high-precision reference electrode)
- **Multimeter** (for debugging)
- **Oscilloscope** (for circuit verification)

---

## 🔌 Circuit Diagram

### System Block Diagram

```
┌─────────────┐       I2C        ┌─────────────┐
│   ESP32     │◄─────────────────►│  MCP4725    │
│             │  (SDA=21,SCL=22) │  12-bit DAC │
│  3.3V GPIO  │                   └──────┬──────┘
│             │                          │ Vset
│             │                          ▼
│             │                   ┌─────────────┐
│             │       I2C         │   LM358 #1  │ Control
│             │◄───────────┬──────┤  (Op-Amp)   │ Amplifier
│             │            │      │  Non-Inv.   │
└─────────────┘            │      └──────┬──────┘
                           │             │ Vcontrol
                     ┌─────▼──────┐      ▼
                     │  ADS1115   │   ┌──────────────┐
                     │  16-bit    │   │ 3-Electrode  │
                     │  ADC       │◄──┤ Cell (WE,RE, │
                     └────────────┘   │ CE) + Sample │
                                      └──────────────┘
                                             │
                                      ┌──────▼──────┐
                                      │   LM358 #2  │ Transimpedance
                                      │  (Op-Amp)   │ Amplifier
                                      │   I-to-V    │ (100kΩ)
                                      └─────────────┘
```

### Detailed Circuit Schematic

**Control Amplifier (LM358 #1):**
```
MCP4725 VOUT ──┬──────────────► Pin 3 (Non-inverting input)
               │
               │                  Pin 1 (Output) ──► Counter Electrode (CE)
               │                        │
Reference ─────┴──────────────► Pin 2 (Inverting input)
Electrode (RE)
```

**Transimpedance Amplifier (LM358 #2):**
```
                         ┌─── Rf (100kΩ) ────┐
                         │                    │
Working Electrode ──► Pin 2           Pin 1 ──┴──► ADS1115 A0
(WE)                (Inverting)      (Output)

Pin 3 (Non-inv) ──► GND
```

**Electrochemical Cell:**
```
        Electrolyte Solution (0.1M KCl)
        ════════════════════════════════
              │        │        │
             WE       RE       CE
              │        │        │
          [Gold]   [Copper]  [Copper]
         [Sample]   [Wire]    [Wire]
```

---

## 📍 Pin Connections

### ESP32 Connections

| ESP32 Pin | Connection | Notes |
|-----------|------------|-------|
| GPIO 21 | SDA (I2C Data) | Connect to both MCP4725 and ADS1115 |
| GPIO 22 | SCL (I2C Clock) | Connect to both MCP4725 and ADS1115 |
| 3.3V | VDD (Power) | Connect to all modules |
| GND | Ground | Common ground for all components |

### MCP4725 DAC Connections

| MCP4725 Pin | Connection |
|-------------|------------|
| VDD | ESP32 3.3V |
| GND | ESP32 GND |
| SDA | ESP32 GPIO21 |
| SCL | ESP32 GPIO22 |
| VOUT | LM358 #1 Pin 3 |
| A0 | GND (sets I2C address to 0x60) |

### ADS1115 ADC Connections

| ADS1115 Pin | Connection |
|-------------|------------|
| VDD | ESP32 3.3V |
| GND | ESP32 GND |
| SDA | ESP32 GPIO21 |
| SCL | ESP32 GPIO22 |
| A0 | LM358 #2 Pin 1 (current measurement) |
| A1 | (Optional) LM358 #1 Pin 1 (voltage verify) |
| ADDR | Float or GND (I2C address 0x48) |

### LM358 #1 (Control Op-Amp) Connections

| Pin | Function | Connection |
|-----|----------|------------|
| 1 | Output | Counter Electrode (CE) |
| 2 | Inverting Input (-) | Reference Electrode (RE) |
| 3 | Non-Inverting Input (+) | MCP4725 VOUT |
| 4 | GND | ESP32 GND |
| 5-7 | (Op-Amp #2 - unused) | - |
| 8 | VCC | ESP32 3.3V |

### LM358 #2 (Transimpedance Amplifier) Connections

| Pin | Function | Connection |
|-----|----------|------------|
| 1 | Output | ADS1115 A0 (via 100kΩ to Pin 2) |
| 2 | Inverting Input (-) | Working Electrode (WE) |
| 3 | Non-Inverting Input (+) | GND |
| 4 | GND | ESP32 GND |
| 5-7 | (Op-Amp #1 - see above) | - |
| 8 | VCC | ESP32 3.3V |

### Electrode Connections

| Electrode | Symbol | Connection | Purpose |
|-----------|--------|------------|---------|
| Working Electrode | WE | Gold sample | Undergoes reaction |
| Reference Electrode | RE | Copper wire | Voltage reference |
| Counter Electrode | CE | Copper wire | Current return path |

**IMPORTANT:** Electrodes must NOT touch each other! Maintain 1-2 cm spacing.

---

## 💻 Software Setup

### Required Arduino Libraries

Install these libraries via Arduino IDE Library Manager:

1. **Wire.h** - Built-in I2C library (no installation needed)
2. **Adafruit_ADS1X15** - For ADS1115 ADC
   - Library Manager: Search "Adafruit ADS1X15"
   - Version: 2.4.0 or higher
3. **Adafruit_MCP4725** - For MCP4725 DAC
   - Library Manager: Search "Adafruit MCP4725"
   - Version: 2.0.0 or higher

### ESP32 Board Setup

1. **Install ESP32 Board Support:**
   - File → Preferences → Additional Board Manager URLs
   - Add: `https://dl.espressif.com/dl/package_esp32_index.json`
   - Tools → Board → Boards Manager → Search "ESP32" → Install

2. **Select Board:**
   - Tools → Board → ESP32 Arduino → "ESP32 Dev Module"

3. **Configure Settings:**
   - Upload Speed: 115200
   - Flash Frequency: 80MHz
   - Port: (Select your COM port)

### Upload Procedure

1. Open `LSV_Potentiostat.ino` in Arduino IDE
2. Verify all library dependencies are installed
3. Connect ESP32 via USB
4. Click **Upload** (→)
5. Wait for "Done uploading" message
6. Open **Serial Monitor** at **115200 baud**

---

## 🚀 Usage Instructions

### Step-by-Step Operating Procedure

#### 1. **Prepare Electrolyte Solution**

**Option A: Potassium Chloride (KCl) - Recommended for beginners**
- Mix 7.5g KCl in 1L distilled water → 0.1M solution
- Safe, easy to handle, readily available

**Option B: Sulfuric Acid (H₂SO₄) - Better sensitivity**
- Dilute 28mL concentrated H₂SO₄ in 1L distilled water → 0.5M solution
- ⚠️ CAUTION: Add acid to water slowly, wear safety gear
- Better conductivity = larger peaks

#### 2. **Assemble Electrochemical Cell**

1. Pour 50-100mL electrolyte into beaker
2. Attach alligator clips to electrodes:
   - **WE (Red):** Gold sample
   - **RE (Black):** Copper wire
   - **CE (White):** Copper wire
3. Submerge electrodes 2-3 cm below surface
4. Ensure electrodes do NOT touch each other!
5. Position beaker on stable surface

#### 3. **Power On and Initialize**

1. Connect ESP32 to USB power
2. Open Serial Monitor (115200 baud)
3. Watch initialization sequence:
   - I2C bus initialization
   - ADS1115 detection
   - MCP4725 detection
4. Verify "System ready!" message

#### 4. **Run Pre-Scan Checklist**

The system displays a checklist - verify each item:
- ☐ Electrodes properly positioned
- ☐ Electrodes submerged in electrolyte
- ☐ Electrodes NOT touching each other
- ☐ Beaker on stable surface

#### 5. **Perform Scan**

1. System automatically starts first scan after 10-second countdown
2. Watch real-time data scrolling:
   - Voltage sweep from 0V to 1.2V
   - Current readings at each step
   - Peak detection notes
3. Scan completes in ~15-30 seconds

#### 6. **Interpret Results**

The system displays three sections:

**A. Peak Analysis:**
- Copper peak: Height and voltage
- Silver peak: Height and voltage
- Interpretation of detected metals

**B. Karat Estimation:**
- Estimated purity (e.g., "18K")
- Gold percentage (e.g., "75.0%")
- Confidence level
- Reasoning for estimate

**C. Calibration Note:**
- Reminder to calibrate with known standards
- Instructions for improving accuracy

#### 7. **Replace Sample (Optional)**

- Next scan automatically runs after 30 seconds
- You can replace gold sample between scans
- System continues cycling indefinitely

---

## 🎯 Calibration Procedure

**CRITICAL:** Default thresholds are estimates. Calibrate with known gold standards for accurate results!

### Required Calibration Samples

Obtain certified gold samples:
- **24K** (99.9% Au) - Pure gold
- **22K** (91.6% Au) - High purity
- **18K** (75.0% Au) - Jewelry grade
- **14K** (58.3% Au) - Common alloy

### Calibration Steps

#### Step 1: Baseline Measurement

1. Run scan with **only electrodes** (no gold sample)
2. Record baseline noise:
   - Should be <0.3 µA
   - If higher: Check for shorts, improve connections
3. This is your noise floor

#### Step 2: Copper Wire Test

1. Attach bare **copper wire** as WE
2. Run scan - should see clear peak at ~0.1V
3. Peak height: 5-10 µA (confirms circuit works)
4. If no peak: Check circuit connections

#### Step 3: Known Sample Testing

For each known karat (24K, 22K, 18K, 14K):

1. **Attach sample** as WE
2. **Run 3 scans** and record:
   - Copper peak height (µA)
   - Silver peak height (µA)
   - Total peak magnitude (µA)
3. **Calculate average** of 3 scans
4. **Record in table:**

| Karat | Cu Peak (µA) | Ag Peak (µA) | Total (µA) |
|-------|-------------|-------------|-----------|
| 24K   |             |             |           |
| 22K   |             |             |           |
| 18K   |             |             |           |
| 14K   |             |             |           |

#### Step 4: Update Threshold Values

Edit `include/config.h`:

```cpp
// Update based on your calibration data:
#define CU_THRESHOLD        [Your Cu minimum]
#define AG_THRESHOLD        [Your Ag minimum]
#define KARAT_24_MAX        [Your 24K total peak]
#define KARAT_22_MAX        [Your 22K total peak]
#define KARAT_18_MAX        [Your 18K total peak]
```

Example:
```cpp
// If your data shows:
// 24K: 0.3 µA, 22K: 1.5 µA, 18K: 4.2 µA, 14K: 8.1 µA
#define CU_THRESHOLD        0.5   // Set above noise floor
#define AG_THRESHOLD        0.3
#define KARAT_24_MAX        0.5   // Slightly above 24K reading
#define KARAT_22_MAX        2.0   // Between 22K and 18K
#define KARAT_18_MAX        5.5   // Between 18K and 14K
```

#### Step 5: Validation

1. Re-upload firmware with new thresholds
2. Test unknown sample
3. Compare estimated karat to known value
4. If within ±1 karat: Calibration successful!
5. If not: Repeat calibration or adjust thresholds

---

## 📊 Understanding Results

### Peak Analysis Output

```
═══════════════════════════════════════
PEAK ANALYSIS
═══════════════════════════════════════

Scanning for copper peak (0.00V to 0.25V):
✓ COPPER detected: 6.23 µA peak at 0.060V
  → Indicates copper-containing alloy

Scanning for silver peak (0.30V to 0.55V):
✓ SILVER detected: 2.45 µA peak at 0.420V
  → Indicates silver-containing alloy
```

**Interpretation:**
- **✓ COPPER detected** → Sample contains copper alloy
- **Peak height (6.23 µA)** → Amount of copper (higher = more Cu)
- **Peak voltage (0.060V)** → Confirms copper oxidation potential
- Same logic applies for silver

### Karat Estimation Output

```
───────────────────────────────────────
KARAT ESTIMATION
───────────────────────────────────────

Estimated purity: 18K (75.0% gold)
Classification: Medium Purity Gold
Confidence level: 70%

Reasoning:
  Moderate alloy peaks (8.68 µA total) indicate ~25% alloy
  content, consistent with 18K gold. Typical jewelry grade.
```

**Key Metrics:**
- **Estimated purity:** Best guess karat value
- **Classification:** Quality category
- **Confidence:** How reliable the estimate is (calibration improves this)
- **Reasoning:** Why the system chose this karat

### Peak Height Guidelines (After Calibration)

| Total Peak | Likely Karat | Gold % | Notes |
|-----------|--------------|--------|-------|
| <0.5 µA | 24K | 99.9% | Pure gold, no alloy peaks |
| 0.5-2.0 µA | 22K | 91.6% | High purity, small peaks |
| 2.0-5.0 µA | 18K | 75.0% | Jewelry grade, medium peaks |
| 5.0-10 µA | 14K | 58.3% | Lower purity, large peaks |
| >10 µA | 10K or less | <50% | Very high alloy content |

---

## 🔍 Troubleshooting

### Problem: "ADS1115 not detected at 0x48"

**Possible Causes:**
- I2C wiring incorrect
- ADS1115 not powered
- Wrong I2C address

**Solutions:**
1. Check SDA → GPIO21, SCL → GPIO22
2. Verify VDD → 3.3V, GND → GND
3. Run I2C scanner sketch to detect address
4. Check solder joints on ADS1115 module

---

### Problem: "MCP4725 not detected at 0x60"

**Solutions:**
- Same as above for MCP4725
- Verify A0 pin on MCP4725 is grounded (sets address to 0x60)

---

### Problem: All current readings are zero

**Possible Causes:**
- Working electrode not connected
- Electrodes not in electrolyte
- Circuit connection error

**Solutions:**
1. Check WE alligator clip is attached to gold sample
2. Verify all electrodes submerged 2-3 cm
3. Test with copper wire as WE (should see ~5-10 µA peak)
4. Check 100kΩ feedback resistor on LM358 #2

---

### Problem: Very high baseline current (>1 µA)

**Possible Causes:**
- Electrodes touching each other
- Short circuit in connections
- Contaminated electrolyte

**Solutions:**
1. Separate electrodes by 1-2 cm
2. Check for bare wire touching
3. Use fresh electrolyte solution
4. Clean electrodes with isopropyl alcohol

---

### Problem: Unrealistic peak heights (>100 µA)

**Possible Causes:**
- Wrong feedback resistor value
- Op-amp saturating
- ADC gain incorrect

**Solutions:**
1. Verify Rf = 100kΩ (brown-black-yellow-gold)
2. Check ADC_GAIN = GAIN_TWO in config.h
3. Reduce voltage scan range

---

### Problem: No peaks detected on known alloy gold

**Possible Causes:**
- Poor electrode contact with sample
- Electrolyte too weak
- Sample is actually pure gold (24K)

**Solutions:**
1. Ensure good electrical contact to gold sample
2. Increase electrolyte concentration
3. Try H₂SO₄ instead of KCl (3-4× higher peaks)
4. Verify sample is not pure gold

---

### Problem: Peaks in wrong voltage range

**Possible Causes:**
- Reference electrode potential shifted
- Using different RE material than Cu

**Solutions:**
1. Use fresh copper wire as RE
2. Clean RE with sandpaper (remove oxide)
3. Adjust peak windows in config.h if using Ag/AgCl or other RE

---

## 📚 Theory of Operation

### Electrochemical Principles

**Linear Sweep Voltammetry (LSV)** is an electrochemical technique where:

1. **Voltage is swept linearly** from start to end potential
2. **Current is measured** at each voltage step
3. **Oxidation reactions occur** at characteristic voltages
4. **Current peaks indicate** which metals are present

### Oxidation Reactions in Gold Alloys

Gold alloys contain various metals that oxidize at different potentials:

| Metal | Oxidation Reaction | Potential (vs Cu) |
|-------|-------------------|-------------------|
| Copper (Cu) | Cu → Cu²⁺ + 2e⁻ | ~0.0 to +0.2V |
| Silver (Ag) | Ag → Ag⁺ + e⁻ | ~0.3 to +0.5V |
| Nickel (Ni) | Ni → Ni²⁺ + 2e⁻ | ~-0.1 to +0.1V |
| Zinc (Zn) | Zn → Zn²⁺ + 2e⁻ | ~-0.8 to -0.6V |
| **Gold (Au)** | **Au → Au³⁺ + 3e⁻** | **~+1.5V** |

**Key Insight:** Gold oxidizes at much higher voltage (~1.5V) than our scan range (0-1.2V), so pure gold shows NO peaks. Alloy metals oxidize in our range, creating peaks proportional to their concentration.

### Circuit Operation

#### Voltage Control (Potentiostat):

```
MCP4725 (DAC) → Sets target voltage (Vset)
                ↓
LM358 #1 (Control Op-Amp) → Forces: V_WE - V_RE = Vset
                ↓
Counter Electrode (CE) → Drives current to maintain voltage
```

The control op-amp ensures the voltage difference between working and reference electrodes matches the DAC output.

#### Current Measurement (Transimpedance Amplifier):

```
Working Electrode → Current flows (I_WE)
                ↓
LM358 #2 (I-to-V) → Converts current to voltage: Vout = I × Rf
                ↓
ADS1115 (ADC) → Measures voltage, calculates I = Vout / Rf
```

The 100kΩ feedback resistor converts microamp currents to measurable voltages.

### Why Peak Height Indicates Concentration

**Faraday's Law:** Current is proportional to the rate of electrochemical reaction.

**More alloy metal** → More atoms available to oxidize → Larger current → Higher peak

This is why:
- **24K gold (pure):** No alloy → No peaks
- **14K gold (42% alloy):** Lots of alloy → Large peaks

### Limitations and Accuracy

**This is NOT a replacement for:**
- X-ray fluorescence (XRF) spectrometry
- Fire assay (destructive gold testing)
- Professional hallmarking

**Accuracy depends on:**
- ✅ Proper calibration with known standards
- ✅ Consistent testing conditions
- ✅ Clean electrodes and fresh electrolyte
- ✅ Good electrical connections

**Expected accuracy after calibration:** ±1-2 karats

---

## 📖 References

### Academic Papers

1. Bard, A. J., & Faulkner, L. R. (2001). *Electrochemical Methods: Fundamentals and Applications*. John Wiley & Sons.

2. Scholz, F. (Ed.). (2010). *Electroanalytical Methods: Guide to Experiments and Applications*. Springer.

3. Wang, J. (2006). *Analytical Electrochemistry* (3rd ed.). Wiley-VCH.

### Online Resources

- **Electrochemistry Tutorial:** https://chem.libretexts.org/Bookshelves/Analytical_Chemistry/Supplemental_Modules_(Analytical_Chemistry)/Electrochemistry
- **LSV Explained:** https://www.basinc.com/products/ec/techniques/lsv/
- **Gold Alloy Compositions:** https://en.wikipedia.org/wiki/Colored_gold

### Component Datasheets

- [MCP4725 DAC Datasheet](https://www.microchip.com/wwwproducts/en/MCP4725)
- [ADS1115 ADC Datasheet](https://www.ti.com/product/ADS1115)
- [LM358 Op-Amp Datasheet](https://www.ti.com/product/LM358)
- [ESP32 Datasheet](https://www.espressif.com/en/products/socs/esp32)

---

## 🛡️ Safety Warnings

### Electrical Safety

- ⚠️ Do NOT connect mains voltage to this circuit
- ⚠️ Keep electrolyte away from electronics
- ⚠️ Use proper insulation on all connections
- ⚠️ ESP32 operates at 3.3V - do not exceed!

### Chemical Safety

**If using H₂SO₄ (Sulfuric Acid):**
- 🧪 Wear safety goggles and gloves
- 🧪 Always add acid to water (NEVER reverse!)
- 🧪 Work in ventilated area
- 🧪 Have baking soda nearby (neutralizes spills)
- 🧪 Dispose of properly per local regulations

**If using KCl (Potassium Chloride):**
- ✅ Much safer alternative for beginners
- ✅ Still wear gloves (avoid contamination)
- ✅ Dispose of properly (do not pour down drain)

---

## 🏆 Project Status

**Version:** 1.0.0
**Last Updated:** 2025-12-09
**Status:** Production Ready for Hackathon Demo

### Future Enhancements

- [ ] Cyclic Voltammetry (CV) mode
- [ ] Differential Pulse Voltammetry (DPV) for higher sensitivity
- [ ] Bluetooth/WiFi data logging
- [ ] Mobile app for real-time plotting
- [ ] Automatic peak fitting algorithms
- [ ] Machine learning karat classification
- [ ] LCD display for standalone operation

---

## 👥 Contributors

**Smart India Hackathon 2025 Team**

For questions, issues, or contributions, please contact the team or open an issue on the project repository.

---

## 📄 License

This project is open-source and available for educational purposes.

**For commercial use or professional testing applications, please consult with electrochemistry experts and ensure compliance with local regulations.**

---

## 🎓 Educational Note

This project demonstrates:
- ✅ Electrochemical analysis techniques
- ✅ ESP32 microcontroller programming
- ✅ I2C communication protocols
- ✅ Op-amp circuit design
- ✅ Data acquisition and signal processing
- ✅ Scientific instrumentation principles

Perfect for students learning about:
- Analytical chemistry
- Embedded systems
- Sensor design
- Materials science

---

**Happy Testing! 🧪⚗️🔬**

*Remember: Always calibrate with known standards for accurate results!*
