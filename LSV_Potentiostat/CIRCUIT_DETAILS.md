# Circuit Details and Component Selection Guide

## Circuit Theory and Design Rationale

### System Architecture

The LSV potentiostat consists of two main functional blocks:

1. **Voltage Control System (Potentiostat)**
2. **Current Measurement System (Amperometry)**

---

## 1. Voltage Control System

### Purpose
Maintain a precise, controlled voltage difference between the Working Electrode (WE) and Reference Electrode (RE).

### Circuit Design

```
         MCP4725 DAC
         ┌─────────┐
         │  VOUT   │──────┐
         └─────────┘      │
                          │ Vset (0 to 1.2V)
                          │
                          ▼
         ┌────────────────────────────┐
         │     LM358 #1 (Op-Amp)      │
         │                            │
    ┌────┤ Pin 3 (+)    Pin 1 (OUT)  │────► Counter Electrode (CE)
    │    │                            │
    │    │ Pin 2 (-)                  │
    │    └────────────────────────────┘
    │           ▲
    │           │
    └───────────┘ Feedback from Reference Electrode (RE)
```

### How It Works

1. **DAC sets target voltage**: MCP4725 outputs Vset (0-1.2V)

2. **Op-amp compares**:
   - Non-inverting input (+): Vset from DAC
   - Inverting input (-): Actual voltage at RE

3. **Feedback control**:
   - If V_RE < Vset: Op-amp output increases
   - If V_RE > Vset: Op-amp output decreases
   - This drives current through CE to adjust V_RE

4. **Result**: V_WE - V_RE = Vset (precisely controlled)

### Component Selection

**MCP4725 DAC:**
- **Resolution**: 12-bit (4096 steps)
- **Why chosen**: I2C interface, single supply, good accuracy
- **Alternatives**: MCP4728 (quad DAC), AD5693 (16-bit)

**LM358 Op-Amp:**
- **Type**: Dual operational amplifier
- **Why chosen**: Low cost, rail-to-rail output, single supply
- **Key specs**:
  - Supply voltage: 3V to 32V (works with 3.3V)
  - Output current: ±40mA (sufficient for electrochemistry)
  - Bandwidth: 1MHz (adequate for LSV scan speeds)
- **Alternatives**:
  - LM324 (quad op-amp)
  - TL072 (JFET input, lower offset)
  - OPA2340 (precision, higher cost)

---

## 2. Current Measurement System

### Purpose
Convert microamp-scale currents from the Working Electrode into measurable voltages for the ADC.

### Circuit Design (Transimpedance Amplifier)

```
Working Electrode (WE)
        │
        │ I_WE (current to measure)
        │
        ▼
   ┌────────────────────────────┐
   │     LM358 #2 (Op-Amp)      │      Rf = 100kΩ
   │                            │   ┌──────────┐
   │ Pin 2 (-)    Pin 1 (OUT)   │───┤          │
   │                            │   │          │
   │ Pin 3 (+)                  │   └──────────┘
   └────────────────────────────┘        │
        │                                 │
       GND                                │
                                          ▼
                                    ADS1115 A0
                                    (Voltage ADC)
```

### How It Works

1. **Current input**: WE injects current I_WE into inverting input (-)

2. **Virtual ground**: Op-amp forces Pin 2 to ~0V (same as Pin 3)

3. **Current-to-Voltage conversion**:
   - All current flows through Rf (feedback resistor)
   - Output voltage: Vout = -I_WE × Rf
   - Negative sign because it's inverting configuration

4. **Example calculation**:
   - If I_WE = 5 µA and Rf = 100kΩ
   - Vout = -5µA × 100kΩ = -0.5V
   - ADC measures 0.5V, we flip sign in software

### Component Selection

**Feedback Resistor (Rf):**
- **Value**: 100kΩ (brown-black-yellow-gold)
- **Why chosen**: Good sensitivity (100µA gives 10V)
- **Tolerance**: 1% recommended (5% acceptable)
- **Power rating**: 1/4W (more than sufficient)

**Sensitivity analysis:**
| Rf Value | Current Range | Output Voltage | Notes |
|----------|---------------|----------------|-------|
| 10kΩ | 0-200µA | 0-2V | Lower sensitivity |
| **100kΩ** | **0-20µA** | **0-2V** | **Optimal for gold** |
| 1MΩ | 0-2µA | 0-2V | High sensitivity, more noise |

**ADS1115 ADC:**
- **Resolution**: 16-bit (65536 steps)
- **Why chosen**: I2C interface, programmable gain, high accuracy
- **Gain setting**: GAIN_TWO (±2.048V range)
  - Resolution: 2.048V / 32768 = 0.0625mV per bit
  - Current resolution: 0.0625mV / 100kΩ = 0.625nA
- **Alternatives**:
  - ADS1015 (12-bit, faster but lower resolution)
  - External ADC on ESP32 (only 12-bit, noisy)

---

## 3. Electrochemical Cell

### Three-Electrode Configuration

```
        Electrolyte Solution
        ══════════════════════════════
              │        │        │
             WE       RE       CE
              │        │        │
          ┌───▼───┐ ┌──▼──┐ ┌──▼──┐
          │ Gold  │ │ Cu  │ │ Cu  │
          │Sample │ │Wire │ │Wire │
          └───────┘ └─────┘ └─────┘
```

### Electrode Roles

**Working Electrode (WE):**
- **Function**: Site of electrochemical reaction (oxidation)
- **Material**: Gold sample being tested
- **Current**: I_WE flows out (oxidation reaction)
- **Connection**: To transimpedance amplifier (LM358 #2)

**Reference Electrode (RE):**
- **Function**: Stable voltage reference point
- **Material**: Copper wire (or graphite, Ag/AgCl)
- **Current**: Ideally zero (high impedance input to op-amp)
- **Connection**: To control op-amp inverting input (LM358 #1 Pin 2)

**Counter Electrode (CE):**
- **Function**: Completes the circuit, returns current
- **Material**: Copper wire or graphite (inert, non-reactive)
- **Current**: I_CE = -I_WE (current return path)
- **Connection**: From control op-amp output (LM358 #1 Pin 1)

### Why Three Electrodes?

**Two-electrode system problems:**
- Voltage drop across RE due to current flow
- Polarization of RE changes its potential
- Can't measure true WE potential

**Three-electrode solution:**
- RE carries no current (high impedance op-amp input)
- CE handles all current flow
- RE remains at stable potential
- Accurate voltage control at WE

### Electrode Material Selection

| Electrode | Material Options | Pros | Cons |
|-----------|-----------------|------|------|
| **WE** | Gold sample | - | Must test sample |
| **RE** | Copper wire | Cheap, available | Potential drifts slowly |
| | Graphite rod | Stable, inert | Less predictable potential |
| | Ag/AgCl | Very stable (professional) | Expensive, needs maintenance |
| **CE** | Copper wire | Cheap, good conductor | Can oxidize over time |
| | Graphite rod | Inert, won't oxidize | Lower conductivity |
| | Platinum wire | Best (professional) | Very expensive |

**Recommendation for hackathon:**
- **RE & CE**: Copper wire (cheap, works well)
- **Upgrade**: Graphite rods (~$5) for better stability

---

## 4. Power Supply Considerations

### ESP32 Power Budget

| Component | Current Draw | Notes |
|-----------|-------------|-------|
| ESP32 | 80-160mA | WiFi disabled, typical operation |
| MCP4725 | 0.4mA | Low power DAC |
| ADS1115 | 0.15mA | Continuous conversion mode |
| LM358 | 1-2mA | Quiescent current (both op-amps) |
| Electrochemical cell | 1-50µA | Typical LSV currents |
| **Total** | **~85-165mA** | USB power (500mA) sufficient |

### Voltage Regulation

```
USB 5V ──► ESP32 on-board regulator ──► 3.3V ──► All components
```

- ESP32 dev boards include 3.3V LDO regulator
- Output current: Typically 500-800mA (sufficient)
- No external regulator needed

### Decoupling Capacitors

**Why needed:** Filter power supply noise, stabilize op-amp operation

**Placement:**
```
ESP32 3.3V ──┬─── 100µF (bulk) ───┬─── GND
             │                     │
             ├─── 100nF ─────────┤
             │    (MCP4725)       │
             ├─── 100nF ─────────┤
             │    (ADS1115)       │
             ├─── 100nF ─────────┤
             │    (LM358 Pin 8)   │
             └─── 100nF ─────────┘
```

- **100µF electrolytic**: Low frequency noise, bulk energy storage
- **100nF ceramic**: High frequency noise, close to IC power pins

---

## 5. I2C Bus Configuration

### Shared I2C Bus

```
ESP32 GPIO21 (SDA) ─┬──── MCP4725 SDA
                    └──── ADS1115 SDA

ESP32 GPIO22 (SCL) ─┬──── MCP4725 SCL
                    └──── ADS1115 SCL
```

### Pull-Up Resistors

- **Required**: I2C is open-drain, needs pull-ups
- **Value**: 4.7kΩ recommended (2.2kΩ to 10kΩ acceptable)
- **Often included**: Most breakout modules have on-board pull-ups
- **Check**: If both modules have pull-ups, total resistance = R1 || R2

**Verification:**
- Use multimeter to measure SDA/SCL to 3.3V: Should read ~4-5kΩ
- If >10kΩ: Add external pull-ups
- If <2kΩ: May need to remove one set

### I2C Addresses

| Device | Default Address | ADDR Pin Configuration |
|--------|----------------|------------------------|
| MCP4725 | 0x60 | A0 = GND (default) |
| | 0x61 | A0 = VCC |
| ADS1115 | 0x48 | ADDR = GND or Float |
| | 0x49 | ADDR = VCC |
| | 0x4A | ADDR = SDA |
| | 0x4B | ADDR = SCL |

**Our configuration:**
- MCP4725: 0x60 (A0 to GND)
- ADS1115: 0x48 (ADDR floating)
- No address conflicts

### I2C Speed

```cpp
Wire.setClock(400000); // 400kHz fast mode
```

- **Standard mode**: 100kHz (slower, more compatible)
- **Fast mode**: 400kHz (default in our code, faster data transfer)
- **Fast mode plus**: 1MHz (not needed, may be less stable)

---

## 6. Breadboard Layout Tips

### Component Placement Strategy

```
[Top Section]
ESP32 Development Board (straddles center gap)

[Left Side]
- MCP4725 DAC module
- ADS1115 ADC module
- Power rails (3.3V and GND)

[Right Side]
- LM358 DIP-8 chip (straddles center gap)
- 100kΩ resistor (Rf)
- Decoupling capacitors

[Bottom Section]
- Electrode connection points
- Alligator clip connections
```

### Critical Connections

**Keep short and direct:**
1. ESP32 to MCP4725/ADS1115 I2C lines
2. Feedback resistor (LM358 Pin 1 to Pin 2)
3. Power and ground connections

**Can be longer:**
- Electrode wires (use shielded if possible)
- DAC output to op-amp input

### Noise Reduction

**Best practices:**
1. ✅ Twist electrode wires together (reduces EMI pickup)
2. ✅ Keep analog and digital grounds together at one point (star ground)
3. ✅ Use short, thick wire for power connections
4. ✅ Add decoupling caps close to IC power pins
5. ✅ Keep ESP32 WiFi disabled (reduces noise)

**If experiencing noise:**
- Add 10µF capacitor across electrochemical cell
- Use shielded cable for WE connection
- Add RC low-pass filter on ADC input (10Ω + 1µF)

---

## 7. Component Sourcing

### Verified Compatible Modules

**ESP32:**
- ESP32-DevKitC (Espressif official)
- NodeMCU-32S
- DOIT ESP32 DevKit V1
- Any ESP32 board with I2C pins broken out

**MCP4725:**
- Adafruit MCP4725 Breakout
- Generic MCP4725 modules (purple PCB common)
- Verify I2C pull-ups included

**ADS1115:**
- Adafruit ADS1115 Breakout
- Generic ADS1115 modules (blue PCB common)
- Verify I2C pull-ups included

**LM358:**
- Texas Instruments LM358P (DIP-8)
- ON Semiconductor LM358N
- Any LM358 in DIP-8 package

### Bill of Materials (BOM)

| Item | Quantity | Unit Price | Total | Supplier |
|------|----------|-----------|-------|----------|
| ESP32 Dev Board | 1 | $8 | $8 | Amazon, AliExpress |
| MCP4725 Module | 1 | $5 | $5 | Adafruit, eBay |
| ADS1115 Module | 1 | $8 | $8 | Adafruit, eBay |
| LM358 DIP-8 | 1 | $0.50 | $0.50 | Digi-Key, Mouser |
| 100kΩ Resistor 1% | 1 | $0.10 | $0.10 | Digi-Key, local |
| 100µF Capacitor | 1 | $0.20 | $0.20 | Digi-Key, local |
| 100nF Capacitor | 3 | $0.10 | $0.30 | Digi-Key, local |
| Breadboard | 1 | $5 | $5 | Amazon, local |
| Jumper Wires | 1 set | $3 | $3 | Amazon, local |
| Copper Wire | 2m | $2 | $2 | Hardware store |
| Alligator Clips | 3 | $1 | $3 | Amazon, local |
| Beaker (100mL) | 1 | $5 | $5 | Lab supplier, Amazon |
| **TOTAL** | | | **~$40** | |

**Cheaper alternatives:**
- Use ESP32 clones (~$4)
- Generic I2C modules (~$2 each from AliExpress)
- Skip breadboard, solder directly to perfboard
- **Total cost can be <$25**

---

## 8. Testing and Verification

### Step-by-Step Circuit Testing

#### Test 1: Power Supply Check
```
Tool: Multimeter
1. Measure ESP32 3.3V pin to GND: Should read 3.3V ± 0.1V
2. Measure LM358 Pin 8 to Pin 4: Should read 3.3V
3. Measure MCP4725 VDD to GND: Should read 3.3V
4. Measure ADS1115 VDD to GND: Should read 3.3V
```

#### Test 2: I2C Communication
```
Upload: I2C Scanner sketch
Expected output:
  I2C device found at address 0x48 (ADS1115)
  I2C device found at address 0x60 (MCP4725)
```

#### Test 3: DAC Output
```
Upload: Simple sketch setting DAC to 1.0V
Measure: MCP4725 VOUT pin with multimeter
Expected: 1.0V ± 0.05V
```

#### Test 4: ADC Input
```
Apply: 1.0V from DAC to ADS1115 A0
Read: ADC value in code
Expected: ~1.0V reading (within 1%)
```

#### Test 5: Transimpedance Amplifier
```
Apply: 10µA current source to LM358 #2 Pin 2
Measure: LM358 #2 Pin 1 with multimeter
Expected: 10µA × 100kΩ = 1.0V
```

#### Test 6: Electrochemical Cell
```
Setup: WE = copper wire, RE = copper wire, CE = copper wire
Submerge: In 0.1M KCl solution
Run: LSV scan
Expected: Copper oxidation peak at ~0.1V with 5-10µA current
```

---

## 9. Troubleshooting Circuit Issues

### Issue: Op-amp output saturates at rail

**Symptoms:**
- LM358 output stuck at 3.3V or 0V
- No variation with input voltage

**Causes:**
- Incorrect feedback connection
- Electrode disconnected
- Supply voltage too low

**Solutions:**
1. Verify feedback resistor connected (Pin 1 to Pin 2)
2. Check electrode connections
3. Increase supply voltage to 5V if possible

---

### Issue: Noisy current measurements

**Symptoms:**
- Current readings fluctuate rapidly (±0.5µA or more)
- Spiky peaks in LSV scan

**Causes:**
- Poor grounding
- EMI pickup on electrode wires
- Missing decoupling capacitors

**Solutions:**
1. Add 100nF caps on all IC power pins
2. Twist electrode wires together
3. Keep electrode wires away from ESP32 antenna
4. Enable smoothing filter in config.h

---

### Issue: Offset voltage (baseline not zero)

**Symptoms:**
- Current reading at 0V is not zero (>0.3µA)
- All readings shifted by constant value

**Causes:**
- Op-amp input offset voltage
- Thermal drift
- Electrode potential difference

**Solutions:**
1. Enable baseline subtraction in config.h
2. Use precision op-amp (OPA2340) instead of LM358
3. Measure baseline and subtract in software (already implemented)

---

## 10. Advanced Modifications

### Upgrade 1: Bipolar Voltage Range (-1V to +1.5V)

**Why:** Detect zinc (negative voltage) and gold (positive voltage)

**Hardware changes:**
- Add voltage level shifter circuit
- Use bipolar DAC (DAC8568) or dual DAC
- Modify feedback to handle negative voltages

---

### Upgrade 2: Multiple Working Electrodes

**Why:** Test multiple samples simultaneously

**Hardware changes:**
- Add analog multiplexer (CD4051)
- Switch between WE connections
- Run scans sequentially on each sample

---

### Upgrade 3: Cyclic Voltammetry (CV) Mode

**Why:** Scan forward then backward (check reversibility)

**Software changes:**
- Implement reverse scan after forward scan
- Compare forward and reverse peak positions
- Indicates reversible vs irreversible reactions

---

### Upgrade 4: Battery Power + LCD Display

**Why:** Portable, standalone operation

**Hardware additions:**
- 18650 Li-ion battery (3.7V)
- TP4056 charging module
- 16×2 LCD display (I2C)
- Push buttons for control

**Total cost:** +$15

---

## Summary

This circuit provides a functional, low-cost LSV potentiostat suitable for educational purposes and hackathon demonstrations. While not research-grade precision, it successfully demonstrates electrochemical analysis principles and provides useful gold purity estimates with proper calibration.

**Key Strengths:**
✅ Simple, well-documented design
✅ Low cost (<$50 total)
✅ Uses common, available components
✅ Modular code for easy modification
✅ Educational value for learning electrochemistry

**Limitations:**
⚠️ Not suitable for regulatory/commercial gold testing
⚠️ Requires calibration with known standards
⚠️ Limited voltage range (0-1.2V)
⚠️ Moderate accuracy (±1-2 karats after calibration)

**For production use, consider:**
- Professional potentiostat (e.g., PalmSens, BioLogic)
- X-ray fluorescence (XRF) spectrometry
- Fire assay (destructive but highest accuracy)
