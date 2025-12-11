# Op-Amp Saturation Diagnostic Guide

## Problem: A0 = 1.97V (constant), A1 = 1.66V (constant)

These constant voltages indicate **op-amp saturation**. The op-amps are stuck at fixed output voltages and not responding to input changes.

---

## 🔍 Root Causes & Solutions

### 1. **NO ELECTROCHEMICAL CELL CONNECTED** ⚠️ MOST COMMON
**Symptoms:**
- A0 and A1 constant regardless of voltage
- No current flow (op-amps have nothing to measure)

**Check:**
- Are electrodes (WE, RE, CE) connected?
- Are electrodes in electrolyte solution?
- Is there a complete circuit path?

**Solution:**
- Connect all three electrodes
- Submerge in electrolyte (0.1M KCl or 0.5M H₂SO₄)
- Ensure electrodes are NOT touching each other

---

### 2. **OP-AMP POWER SUPPLY ISSUES**
**Symptoms:**
- Op-amps not getting proper power
- Outputs stuck at mid-rail or ground

**Check:**
```
LM358 Pin 8 (VCC) → Should be 3.3V
LM358 Pin 4 (GND) → Should be 0V (connected to ESP32 GND)
```

**Measure:**
- Pin 8 to Pin 4: Should read 3.3V
- If 0V: Power not connected
- If wrong voltage: Check power supply

**Solution:**
- Verify ESP32 3.3V → LM358 Pin 8
- Verify ESP32 GND → LM358 Pin 4
- Check for loose connections

---

### 3. **BROKEN FEEDBACK LOOP (LM358 #1 - Control Op-Amp)**
**Symptoms:**
- A1 constant at 1.66V
- Voltage changes don't affect output

**Circuit Check:**
```
MCP4725 VOUT → LM358 #1 Pin 3 (+) ✓
RE (Reference Electrode) → LM358 #1 Pin 2 (-) ✓
LM358 #1 Pin 1 → CE (Counter Electrode) ✓
```

**Problems:**
- RE not connected to Pin 2
- CE not connected to Pin 1
- Feedback broken (no connection from RE to Pin 2)

**Solution:**
- Verify RE wire → LM358 Pin 2
- Verify CE wire → LM358 Pin 1
- Check continuity with multimeter

---

### 4. **BROKEN TRANSIMpedance AMPLIFIER (LM358 #2 - Current Measurement)**
**Symptoms:**
- A0 constant at 1.97V
- No current measurement

**Circuit Check:**
```
WE (Working Electrode) → LM358 #2 Pin 2 (-) ✓
LM358 #2 Pin 3 (+) → GND ✓
LM358 #2 Pin 1 → Feedback Resistor (50kΩ) → Pin 2 ✓
LM358 #2 Pin 1 → ADS1115 A0 ✓
```

**Problems:**
- WE not connected to Pin 2
- Feedback resistor (50kΩ) missing or wrong value
- Pin 3 not connected to GND
- Pin 1 not connected to ADS1115 A0

**Solution:**
- Verify WE wire → LM358 Pin 2
- Check feedback resistor: Should be 50kΩ (or 2×100kΩ in parallel)
- Verify Pin 3 → GND
- Verify Pin 1 → ADS1115 A0

---

### 5. **OP-AMP RAIL SATURATION**
**Symptoms:**
- Output stuck near power rails
- A0 at 1.97V (close to 3.3V rail)
- A1 at 1.66V (mid-rail)

**Causes:**
- Input voltage exceeds op-amp range
- No negative feedback (broken connection)
- Op-amp trying to drive beyond capability

**Check:**
- Measure voltage at op-amp inputs:
  - LM358 #1 Pin 2 (should follow Pin 3)
  - LM358 #2 Pin 2 (should be near 0V due to virtual ground)

**Solution:**
- Verify all connections
- Check if inputs are within 0-3.3V range
- Ensure feedback paths are complete

---

### 6. **WRONG FEEDBACK RESISTOR VALUE**
**Symptoms:**
- TIA output saturated
- Current readings don't make sense

**Check:**
- Measure feedback resistor: Should be 50kΩ
- If using 2×100kΩ in parallel: Verify both connected correctly
- If wrong value: Replace with correct resistor

**Solution:**
- Use 50kΩ resistor (or 2×100kΩ in parallel)
- Verify resistor is connected between Pin 1 and Pin 2 of LM358 #2

---

### 7. **ELECTROLYTE/ELECTRODE ISSUES**
**Symptoms:**
- Circuit connected but no current flow
- Op-amps idle (no signal to amplify)

**Check:**
- Electrolyte present? (at least 50-100mL)
- Electrodes submerged? (2-3cm deep)
- Electrodes clean? (no oxidation coating)
- Electrodes spaced? (1-2cm apart, not touching)

**Solution:**
- Use fresh electrolyte (0.1M KCl recommended)
- Clean electrodes (sandpaper for copper wires)
- Ensure proper spacing
- Verify electrodes are conductive

---

### 8. **SHORT CIRCUIT OR OPEN CIRCUIT**
**Symptoms:**
- Unexpected constant voltages
- No response to changes

**Check:**
- Short circuit: Measure resistance between electrodes (should NOT be 0Ω)
- Open circuit: Check continuity of all wires
- Verify no accidental shorts (wires touching)

**Solution:**
- Fix any shorts (separate touching wires)
- Fix any opens (reconnect broken wires)
- Verify all connections with multimeter

---

## 🔧 Quick Diagnostic Steps

### Step 1: Check Power
```
Measure: LM358 Pin 8 to Pin 4
Expected: 3.3V
If 0V: Power not connected
```

### Step 2: Check Control Op-Amp (LM358 #1)
```
Measure: Pin 1 (output) - should vary with DAC voltage
Measure: Pin 2 (RE input) - should follow Pin 3
Measure: Pin 3 (DAC input) - should match MCP4725 VOUT
```

### Step 3: Check TIA (LM358 #2)
```
Measure: Pin 1 (output = A0) - should vary with current
Measure: Pin 2 (WE input) - should be near 0V (virtual ground)
Measure: Pin 3 (GND) - should be 0V
Measure: Feedback resistor - should be 50kΩ
```

### Step 4: Check Electrodes
```
Measure: Continuity between electrodes and circuit
Expected: Connected (low resistance)
If open: Wire broken or not connected
```

### Step 5: Check Complete Circuit
```
Verify: WE → LM358 #2 Pin 2
Verify: RE → LM358 #1 Pin 2  
Verify: CE → LM358 #1 Pin 1
Verify: All in electrolyte solution
```

---

## 📊 Expected Behavior

**When Working Correctly:**
- A0 should vary with current (typically 0-2V range)
- A1 should vary with applied voltage (follows DAC output)
- Differential (A0-A1) should change with voltage scan
- Current should show peaks at specific voltages

**When Saturated:**
- A0 constant (stuck at fixed voltage)
- A1 constant (stuck at fixed voltage)
- No response to voltage changes
- No current variation

---

## 🎯 Most Likely Issues (Priority Order)

1. **No electrochemical cell connected** (90% of cases)
2. **Op-amp power not connected** (Pin 8 not at 3.3V)
3. **Feedback resistor missing/wrong** (TIA not working)
4. **Electrodes not connected** (open circuit)
5. **Short circuit** (wires touching)

---

## ✅ Fix Checklist

- [ ] LM358 Pin 8 = 3.3V (power connected)
- [ ] LM358 Pin 4 = 0V (ground connected)
- [ ] Feedback resistor = 50kΩ (connected between Pin 1 and Pin 2 of LM358 #2)
- [ ] WE connected to LM358 #2 Pin 2
- [ ] RE connected to LM358 #1 Pin 2
- [ ] CE connected to LM358 #1 Pin 1
- [ ] All electrodes in electrolyte solution
- [ ] Electrodes not touching each other
- [ ] MCP4725 VOUT connected to LM358 #1 Pin 3
- [ ] ADS1115 A0 connected to LM358 #2 Pin 1
- [ ] ADS1115 A1 connected to LM358 #1 Pin 1 (optional, for reference)

---

## 🔬 Test Procedure

1. **Disconnect everything** - Start fresh
2. **Check power first** - Verify 3.3V on Pin 8
3. **Connect one op-amp at a time** - Test individually
4. **Add electrodes last** - After circuit verified
5. **Test with multimeter** - Before relying on ADC readings

---

## 📝 Notes

- LM358 can work with 3.3V supply but has limited output swing
- Output range: ~0.1V to ~3.2V (not true rail-to-rail)
- Saturation voltages: Near 0V (negative saturation) or near 3.3V (positive saturation)
- Mid-rail voltages (like 1.66V) suggest no signal or input offset

