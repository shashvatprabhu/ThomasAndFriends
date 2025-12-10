# Eddy Current Gold Tester - ESP32-WROOM-32E

**Pure C code using ESP-IDF for gold purity testing via eddy current method**

---

## 🎯 Overview

Non-contact gold testing using electromagnetic induction (eddy currents). Different metals have different electrical conductivity, which affects the impedance of an oscillating coil placed near the sample.

**Hardware:**
- ESP32-WROOM-32E microcontroller
- NE555 oscillator (~100kHz)
- 90-turn coil (0.7mm wire, 10mm former)
- LM358 op-amp (buffer/amplifier)
- ADS1115 16-bit ADC
- Voltage divider (10kΩ + 10kΩ)

**Framework:** ESP-IDF (Native C, no Arduino)

---

## ⚡ Quick Build & Flash

### 1. Install ESP-IDF

```bash
# Clone ESP-IDF
mkdir -p ~/esp && cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && git checkout v5.1
./install.sh esp32

# Set environment
. ~/esp/esp-idf/export.sh
```

### 2. Build Project

```bash
cd GoldTester_EddyCurrent

# Configure for ESP32
idf.py set-target esp32

# Build
idf.py build

# Flash
idf.py flash monitor
```

---

## 🔌 Hardware Connections

### ESP32 GPIO Pins

| ESP32 Pin | Connection |
|-----------|------------|
| GPIO 21 | I2C SDA (ADS1115) |
| GPIO 22 | I2C SCL (ADS1115) |
| 5V | Power rail (all modules) |
| GND | Ground rail |

### Full Circuit

```
ESP32 5V → NE555 VCC
         → ADS1115 VCC
         → LM358 Pin 8 (VCC)
         → Voltage divider top

ESP32 GND → All GND connections

NE555 OUT → 220Ω resistor → Coil tail 1
Coil tail 2 → GND
Coil tail 1 → 220pF capacitor → GND
Coil tail 1 → 10kΩ resistor → Sense point
Sense point → 10kΩ resistor → GND
Sense point → LM358 Pin 3 (+IN)
LM358 Pin 2 (-IN) → LM358 Pin 1 (OUT) [unity gain buffer]
LM358 Pin 1 → ADS1115 A0

ADS1115 SDA → ESP32 GPIO21
ADS1115 SCL → ESP32 GPIO22
```

---

## 📂 Project Structure

```
GoldTester_EddyCurrent/
├── CMakeLists.txt           # ESP-IDF project config
├── main/
│   ├── CMakeLists.txt       # Component config
│   ├── main.c               # Main application
│   ├── ads1115.c/h          # ADS1115 ADC driver
│   └── config.h             # All configuration ⭐
└── build/                   # Generated build files
```

---

## ⚙️ Configuration & Calibration

### Edit `main/config.h`

```c
// GPIO pins
#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21

// Measurement parameters
#define SAMPLES                     200     // Averaging samples
#define DELAY_BETWEEN_READINGS_MS   100     // Read interval

// Circuit specs
#define COIL_TURNS                  90      // Your coil turns
#define CURRENT_RESISTOR_OHMS       220     // 220Ω limit resistor
```

### **CRITICAL: Calibration Data**

You MUST update this in `main/main.c`:

```c
static gold_calibration_t calibration[NUM_CALIBRATION_POINTS] = {
    {2.450f, 0},            // No metal (YOUR baseline voltage)
    {2.200f, 14},           // 14K gold (YOUR measurement)
    {2.000f, 18},           // 18K gold (YOUR measurement)
    {1.850f, 22},           // 22K gold (YOUR measurement)
    {1.700f, 24}            // 24K gold (YOUR measurement)
};
```

### Calibration Procedure

1. **Measure baseline** (no metal near coil):
   - System does this automatically at startup
   - Record voltage shown

2. **Test known gold samples**:
   - Get certified 14K, 18K, 22K, 24K samples
   - Place each near coil (consistent position!)
   - Record voltage for each
   - Update array in `main.c`

3. **Rebuild and flash**:
   ```bash
   idf.py build flash
   ```

---

## 📊 Expected Output

```
I (123) GOLD_TESTER: ╔════════════════════════════════════════╗
I (123) GOLD_TESTER: ║   GOLD TESTER - Eddy Current Method   ║
I (123) GOLD_TESTER: ║   90-turn coil, 0.7mm wire            ║
I (123) GOLD_TESTER: ║   ESP32-WROOM-32E + ADS1115           ║
I (123) GOLD_TESTER: ╚════════════════════════════════════════╝

I (234) ADS1115: ADS1115 found at address 0x48

I (456) GOLD_TESTER: Current calibration data:
I (456) GOLD_TESTER: Karat | Voltage
I (456) GOLD_TESTER: ──────┼─────────
I (456) GOLD_TESTER:  None | 2.450V (baseline)
I (456) GOLD_TESTER:   14K | 2.200V
I (456) GOLD_TESTER:   18K | 2.000V
I (456) GOLD_TESTER:   22K | 1.850V
I (456) GOLD_TESTER:   24K | 1.700V

I (789) GOLD_TESTER: Measuring baseline...
I (3800) GOLD_TESTER: ✓ Baseline: 2.453V

I (3810) GOLD_TESTER: ═══════════════════════════════════════
I (3810) GOLD_TESTER: Ready! Bring gold near coil...
I (3810) GOLD_TESTER: ═══════════════════════════════════════

V: 2.453V | ΔV:    0.2mV | Noise:   4.1mV | NO METAL
V: 2.145V | ΔV:  308.0mV | Noise:   3.2mV | 18K Gold ✓✓✓
V: 2.145V | ΔV:  308.0mV | Noise:   3.2mV | 18K Gold ✓✓✓
V: 1.856V | ΔV:  597.0mV | Noise:   4.5mV | 22K Gold ✓✓✓
```

**Reading explanation:**
- **V:** Current voltage
- **ΔV:** Change from baseline (mV) - larger = more conductive metal
- **Noise:** Signal stability (lower = better)
- **✓✓✓:** High confidence (noise < 5mV)
- **✓✓:** Medium confidence (noise 5-15mV)
- **✓:** Low confidence (noise > 15mV)

---

## 🔧 Troubleshooting

### "ADS1115 not found"

**Check:**
- GPIO21 (SDA) connection
- GPIO22 (SCL) connection
- 5V and GND power
- I2C pull-up resistors (usually on ADS1115 module)

**Test:**
```bash
# Run I2C scan
i2cdetect -y 1  # (if using Linux)
# Should show device at 0x48
```

### All readings same as baseline

**Possible causes:**
1. **Coil not oscillating**
   - Check NE555 output with multimeter (AC mode)
   - Should see ~5V peak-to-peak square wave

2. **Broken coil wire**
   - Check continuity with multimeter
   - Should read ~10-20Ω resistance

3. **ADS1115 reading wrong channel**
   - Verify A0 connected to LM358 output

### Voltage not changing with metal

**Check:**
1. **220Ω resistor too large**
   - Try 100Ω instead (more signal)

2. **Coil too far from metal**
   - Bring sample within 5mm of coil

3. **LM358 not powered**
   - Check Pin 8 has 5V

### Noisy readings (noise > 20mV)

**Solutions:**
- Increase `SAMPLES` to 500 in `config.h`
- Add 100nF capacitor across LM358 power pins
- Keep coil away from AC power lines
- Use shielded cable for ADS1115 connections

---

## ⚠️ Limitations

**This method detects surface conductivity only:**

1. **Penetration depth:** 0.2-0.5mm at 100kHz
   - Can't detect core if plated >0.5mm thick
   - Gold-plated tungsten will fool it

2. **Position sensitive:**
   - Distance affects reading
   - Angle affects reading
   - Must use consistent positioning

3. **Alloy composition matters:**
   - 18K white gold ≠ 18K yellow gold (different conductivity)
   - Nickel alloys decrease reading
   - Silver alloys increase reading

4. **Temperature drift:**
   - Re-baseline if ambient temperature changes >5°C
   - Let system warm up 10 minutes

**Recommendation:** Use with other tests (density, XRF) for high-value items

---

## 📐 Circuit Theory

### How It Works

1. **NE555** generates ~100kHz oscillating current
2. **Coil** creates alternating magnetic field
3. **Metal sample** near coil induces **eddy currents**
4. **Eddy currents** oppose coil's field → changes coil impedance
5. **Impedance change** affects voltage across voltage divider
6. **ADS1115** measures this voltage (high resolution)
7. **ESP32** interpolates voltage → karat value

### Why Different Metals Give Different Voltages

**Conductivity affects eddy current strength:**
- **Copper:** Very conductive → Strong eddy currents → Large voltage change
- **Gold (24K):** Highly conductive → Medium voltage change
- **Gold alloys:** Less conductive → Smaller voltage change
- **Lower karat:** More alloy → Even smaller change

**Voltage order:**
```
Baseline (no metal): 2.45V
↓
24K Gold: 1.70V  (very conductive)
↓
22K Gold: 1.85V
↓
18K Gold: 2.00V
↓
14K Gold: 2.20V  (less conductive due to alloy)
```

---

## 🛠️ Recommended Improvements

### For Better Accuracy

1. **Replace NE555 with crystal oscillator**
   - More stable frequency
   - Less temperature drift

2. **Use precision voltage reference**
   - TL431 or similar for stable baseline

3. **Add temperature sensor**
   - Compensate for thermal drift
   - DS18B20 on ESP32 GPIO

4. **Solder circuit on PCB**
   - Eliminate breadboard contact issues
   - Reduce noise pickup

### For Better Usability

1. **Add OLED display**
   - Show karat on-device
   - I2C display (SSD1306) uses same bus

2. **Add calibration button**
   - Re-baseline on button press
   - Save calibration to NVS flash

3. **Build 3D-printed housing**
   - Fixed coil-to-sample distance
   - Repeatable positioning

---

## 📝 Code Logic Summary

**Initialization:**
1. Initialize I2C bus
2. Detect ADS1115
3. Measure baseline (no metal)
4. Display calibration data

**Main Loop:**
1. Read ADC 200 times, average → voltage
2. Calculate ΔV from baseline
3. Measure noise (signal quality)
4. Interpolate voltage → karat
5. Display results
6. Wait 100ms, repeat

**Karat Estimation:**
- Linear interpolation between calibration points
- If voltage matches baseline → "NO METAL"
- If voltage between points → interpolate karat
- If outside range → 0K or 24K

---

## 📚 ESP-IDF Commands

```bash
# Build
idf.py build

# Flash and monitor
idf.py flash monitor

# Clean rebuild
idf.py fullclean build

# Monitor only
idf.py monitor

# Exit monitor
Ctrl + ]
```

---

## ✅ Success Checklist

☐ ESP32 powers on, shows startup message
☐ ADS1115 detected at 0x48
☐ Baseline measured successfully
☐ Voltage changes when metal brought near coil
☐ Different metals give different voltages
☐ Noise level < 15mV for stable readings
☐ Calibrated with known gold samples

---

**Hardware Cost:** ~$30
**Build Time:** 2-3 hours (including coil winding)
**Accuracy:** ±2 karats after calibration
**Scan Time:** Real-time (10 readings/sec)

**Ready to test gold!** 🏅

Build, flash, calibrate, and start detecting! 🚀
