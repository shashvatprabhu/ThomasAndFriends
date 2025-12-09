# Quick Start Guide - LSV Potentiostat

**Get your gold purity analyzer running in 30 minutes!**

---

## ⚡ Speed Run: Minimal Setup

### What You Need (Bare Minimum)

- ✅ ESP32 board + USB cable
- ✅ MCP4725 DAC module
- ✅ ADS1115 ADC module
- ✅ LM358 op-amp chip
- ✅ 100kΩ resistor (brown-black-yellow-gold)
- ✅ Breadboard + jumper wires
- ✅ 3× copper wires (~10cm each)
- ✅ Small beaker + water + salt (yes, table salt works!)

---

## 📝 Step 1: Breadboard Assembly (10 minutes)

### Power Rails First

```
1. Connect ESP32 3.3V pin → Breadboard RED rail (+)
2. Connect ESP32 GND pin → Breadboard BLUE rail (-)
```

### Place Components

```
3. Insert LM358 chip straddling center gap (Pin 1 on left)
4. Insert MCP4725 module in left section
5. Insert ADS1115 module in left section
```

### I2C Connections

```
6. ESP32 GPIO21 → MCP4725 SDA → ADS1115 SDA (shared)
7. ESP32 GPIO22 → MCP4725 SCL → ADS1115 SCL (shared)
```

### Power All Modules

```
8. RED rail (+3.3V) → MCP4725 VDD, ADS1115 VDD, LM358 Pin 8
9. BLUE rail (GND) → MCP4725 GND, ADS1115 GND, LM358 Pin 4
```

### Critical Connections

```
10. MCP4725 VOUT → LM358 Pin 3
11. LM358 Pin 2 → One end of 100kΩ resistor
12. LM358 Pin 1 → Other end of 100kΩ resistor (feedback loop)
13. LM358 Pin 1 → ADS1115 A0
```

### Electrodes (use alligator clips)

```
14. Copper wire #1 → LM358 Pin 2 (Reference Electrode - RE)
15. Copper wire #2 → LM358 Pin 1 (Counter Electrode - CE)
16. Copper wire #3 → LM358 Pin 2 (Working Electrode - WE)
    Note: WE connects to SAME node as RE (the inverting input)
```

**Double-check:** LM358 Pin 2 should connect to BOTH the feedback resistor AND the electrode connection point.

---

## 💻 Step 2: Software Upload (5 minutes)

### Arduino IDE Setup

```
1. File → Preferences → Additional Boards Manager URLs
   Add: https://dl.espressif.com/dl/package_esp32_index.json

2. Tools → Board → Boards Manager → Search "ESP32" → Install

3. Tools → Manage Libraries → Install:
   - "Adafruit ADS1X15"
   - "Adafruit MCP4725"

4. Tools → Board → ESP32 Arduino → "ESP32 Dev Module"

5. File → Open → LSV_Potentiostat.ino

6. Click Upload (→ button)

7. Open Serial Monitor (Ctrl+Shift+M)
   Set baud rate: 115200
```

---

## 🧪 Step 3: Make Electrolyte (5 minutes)

### Easy Option: Salt Water

```
1. Mix 1 teaspoon table salt in 100mL water
   (This makes ~1M NaCl - stronger than needed, but works!)

2. Pour into small beaker or cup

3. Stir until dissolved
```

### Better Option: Potassium Chloride (if available)

```
1. Mix 0.75g KCl in 100mL distilled water → 0.1M solution

2. More consistent results than table salt

3. Available at pharmacy (salt substitute) or chemistry supplier
```

---

## 🔬 Step 4: First Test (5 minutes)

### Copper Wire Test (Verify Circuit Works)

```
1. Submerge all 3 copper wires in electrolyte (2-3cm deep)

2. Make sure wires DON'T touch each other!

3. Watch Serial Monitor - system starts automatically

4. After scan completes, you should see:
   ✓ Copper peak at ~0.05V to 0.15V
   ✓ Peak height: 3-10 µA (depends on wire thickness)

5. If you see this peak: Circuit works! ✅
```

**Troubleshooting:**
- ❌ No peak? Check wire connections and electrolyte
- ❌ "Device not found" error? Check I2C wiring
- ❌ All zeros? Check that wires are in liquid

---

## 🏅 Step 5: Test Gold Sample (5 minutes)

### Attach Your Sample

```
1. Working Electrode (WE):
   - Attach gold jewelry/coin with alligator clip
   - Ensure good electrical contact (scrape surface if needed)
   - Must be partially submerged in electrolyte

2. Reference Electrode (RE):
   - Keep copper wire as-is in electrolyte

3. Counter Electrode (CE):
   - Keep copper wire as-is in electrolyte

4. Run scan and observe:
   - Small peaks (<1µA): High purity (22K-24K)
   - Medium peaks (2-5µA): Medium purity (18K)
   - Large peaks (>5µA): Lower purity (14K or less)
```

---

## 📊 Reading Your Results

### Example Output 1: Pure Gold (24K)

```
═══════════════════════════════════════
PEAK ANALYSIS
═══════════════════════════════════════

Scanning for copper peak (0.00V to 0.25V):
✗ No significant copper peak detected

Scanning for silver peak (0.30V to 0.55V):
✗ No significant silver peak detected

Total peaks detected: 0
Combined peak magnitude: 0.15 µA

KARAT ESTIMATION: 24K (99.9% gold)
```

**Interpretation:** No alloy peaks → Pure gold

---

### Example Output 2: 18K Gold

```
═══════════════════════════════════════
PEAK ANALYSIS
═══════════════════════════════════════

Scanning for copper peak (0.00V to 0.25V):
✓ COPPER detected: 4.5 µA peak at 0.080V

Scanning for silver peak (0.30V to 0.55V):
✓ SILVER detected: 1.8 µA peak at 0.420V

Total peaks detected: 2
Combined peak magnitude: 6.3 µA

KARAT ESTIMATION: 18K (75.0% gold)
Confidence level: 72%
```

**Interpretation:** Moderate Cu+Ag peaks → 18K gold (typical jewelry)

---

### Example Output 3: 14K Gold

```
═══════════════════════════════════════
PEAK ANALYSIS
═══════════════════════════════════════

Scanning for copper peak (0.00V to 0.25V):
✓ COPPER detected: 9.2 µA peak at 0.065V

Scanning for silver peak (0.30V to 0.55V):
✓ SILVER detected: 3.1 µA peak at 0.400V

Total peaks detected: 2
Combined peak magnitude: 12.3 µA

KARAT ESTIMATION: 14K (58.3% gold)
Confidence level: 68%
```

**Interpretation:** Large Cu+Ag peaks → 14K or lower purity

---

## 🎯 Common Issues and 2-Second Fixes

### Issue: "ADS1115 not found"
```
Fix: Check SDA/SCL connections (GPIO21 and GPIO22)
```

### Issue: All current readings are zero
```
Fix: Electrodes not in liquid, or WE disconnected
```

### Issue: Very noisy readings (jumping around)
```
Fix: Make sure electrode wires aren't touching
```

### Issue: Baseline current >1µA at 0V
```
Fix: Electrodes might be touching - separate them
```

### Issue: No copper peak with copper wire test
```
Fix:
1. Check that copper wire is bare (remove insulation)
2. Sand wire to remove oxide layer
3. Use fresh electrolyte
```

---

## ⚙️ Quick Tuning (Optional)

### Make Scans Faster

Edit `include/config.h`:
```cpp
#define STEP_SIZE           0.02    // Was 0.01 (fewer points)
#define SETTLE_TIME_MS      50      // Was 80 (less wait time)

// Result: Scan takes ~8 seconds instead of ~15
```

### Make Scans More Detailed

```cpp
#define STEP_SIZE           0.005   // Was 0.01 (more points)
#define SETTLE_TIME_MS      100     // Was 80 (better equilibration)

// Result: Scan takes ~30 seconds, better resolution
```

### Change Scan Voltage Range

```cpp
#define START_VOLTAGE       -0.5    // Detect zinc (negative voltage)
#define END_VOLTAGE         1.5     // Detect gold oxidation

// Result: Wider detection range, longer scan time
```

---

## 🚀 Next Steps

### Improve Accuracy

1. **Calibrate with known gold samples:**
   - Get 14K, 18K, 22K, 24K certified samples
   - Run 3 scans on each
   - Update thresholds in `config.h`
   - See README.md → Calibration Procedure

2. **Use better electrolyte:**
   - 0.1M KCl (instead of table salt)
   - 0.5M H₂SO₄ (3-4× higher peaks, but handle carefully!)

3. **Upgrade electrodes:**
   - Graphite rods (~$5 for RE/CE)
   - More stable than copper wire

### Add Features

1. **Export data to Excel:**
   ```cpp
   #define ENABLE_CSV_OUTPUT   true    // In config.h
   ```
   Copy CSV data from Serial Monitor → Paste into Excel

2. **Average multiple scans:**
   ```cpp
   #define ENABLE_MULTI_SCAN   true
   #define NUM_SCANS_AVG       3
   ```
   Automatically runs 3 scans and averages results

3. **Smooth noisy data:**
   ```cpp
   #define ENABLE_SMOOTHING    true
   ```
   5-point moving average filter

---

## 📦 Shopping List

### Amazon/Local Electronics Store

```
☐ ESP32 development board ($8)
☐ MCP4725 DAC module ($5)
☐ ADS1115 ADC module ($8)
☐ Breadboard + wires ($8)
☐ 100kΩ resistor pack ($3)

Total: ~$32
```

### Hardware Store

```
☐ Copper wire (electrical wire, bare) ($2)
☐ Small glass/plastic container ($2)
☐ Table salt (already have!)

Total: ~$4
```

### Optional Upgrades

```
☐ Potassium chloride (salt substitute, pharmacy) ($5)
☐ Graphite rods (art supply store) ($5)
☐ Alligator clip test leads ($3)
☐ Multimeter for debugging ($15)

Total: ~$28
```

**Grand Total: $35-65** depending on what you have

---

## 🏆 Success Criteria

You know it's working when:

✅ Copper wire test shows peak at 0.05-0.15V
✅ Peak height is 3-10 µA with copper wire
✅ 24K gold shows NO peaks or very tiny peaks (<0.5µA)
✅ 14K gold shows LARGE peaks (>5µA total)
✅ Consistent results: Same sample gives similar peaks each scan

---

## 🆘 Emergency Troubleshooting

### Nothing Works At All

```
1. Check ESP32 is powered (LED on?)
2. Check USB cable is data cable (not charge-only)
3. Try different USB port
4. Re-upload sketch
5. Open Serial Monitor at 115200 baud
```

### Circuit Works But No Electrochemical Signal

```
1. Verify electrolyte has salt (taste it - should be VERY salty)
2. Check that wires are submerged 2-3cm
3. Sand copper wires to remove oxide
4. Make sure wires aren't touching each other
5. Try copper wire test first (easier than gold)
```

### Readings Are Crazy (Random Numbers)

```
1. Electrodes probably touching - separate them
2. Add 100nF capacitors next to IC power pins
3. Keep electrode wires away from ESP32
4. Disable WiFi in code (already done)
```

### Still Stuck?

Check:
1. README.md (detailed guide)
2. CIRCUIT_DETAILS.md (circuit theory)
3. Troubleshooting section in README.md
4. Re-verify all connections against pin tables

---

## 🎓 Understanding Your Results

### Peak Height vs Karat Relationship

```
Peak Height         Karat       Gold %
═════════════════════════════════════════
<0.5 µA       →     24K    →   99.9%
0.5-2.0 µA    →     22K    →   91.6%
2.0-5.0 µA    →     18K    →   75.0%
5.0-10 µA     →     14K    →   58.3%
>10 µA        →     10K    →   41.7%
```

**Why?**
- More alloy metal → More oxidation → Larger current peak
- Gold doesn't oxidize in our voltage range (needs ~1.5V)
- Pure gold = No alloy = No peaks

### Typical Gold Alloy Compositions

**Yellow Gold:**
- 18K: 75% Au + 15% Cu + 10% Ag
- 14K: 58% Au + 25% Cu + 17% Ag

**White Gold:**
- 18K: 75% Au + 25% Ni + Zn
- Shows Ni peak at ~0V (may overlap with Cu)

**Rose Gold:**
- 18K: 75% Au + 25% Cu (high copper!)
- Very large copper peak

---

## ✨ Pro Tips

1. **Clean electrodes between samples:**
   - Rinse with water
   - Wipe with tissue
   - Prevents contamination

2. **Use consistent sample size:**
   - Same surface area in electrolyte
   - Peak height scales with area!

3. **Temperature matters:**
   - Room temperature preferred (20-25°C)
   - Hot/cold affects reaction rates

4. **Fresh electrolyte:**
   - Replace after 10-20 scans
   - Old solution accumulates metal ions

5. **Sample surface prep:**
   - Clean sample with alcohol
   - Lightly sand oxidized surfaces
   - Better electrical contact

---

**You're ready to start! Connect the circuit, upload the code, and run your first scan. Good luck! 🚀**

**For detailed information, see README.md**
