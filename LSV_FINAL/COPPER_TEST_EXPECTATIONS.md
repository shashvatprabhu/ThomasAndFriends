# Expected Readings for 3 Copper Wires in Solution

## ⚠️ CRITICAL: Virtual Ground Issue

**Your virtual ground at 1.67V is WRONG!**

The TIA (LM358 #2) requires:
- **Pin 3 (non-inverting input) → MUST be connected to GND (0V)**
- **Pin 2 (inverting input) → Should be ~0V (virtual ground)**

If Pin 3 is not at GND, the virtual ground won't work and you'll get wrong readings.

---

## ✅ Correct Circuit Setup

```
LM358 #2 (TIA):
  Pin 3 (+) → GND (0V) ⚠️ MUST BE 0V!
  Pin 2 (-) → WE (Working Electrode)
  Pin 1 (OUT) → ADS1115 A0
  Pin 4 → GND
  Pin 8 → 3.3V
  
Feedback Resistor (40kΩ):
  Between Pin 1 and Pin 2
```

**Check:** Measure Pin 3 to GND - should be 0V!

---

## 📊 Expected Readings for 3 Copper Wires

### Setup:
- **WE (Working):** Copper wire
- **RE (Reference):** Copper wire  
- **CE (Counter):** Copper wire
- **Electrolyte:** 0.1M KCl or 0.5M H₂SO₄
- **Volume:** 50-100 mL

### Expected Current Profile:

| Voltage (V) | Expected Current (µA) | Notes |
|-------------|----------------------|-------|
| 0.00 | 0.1 - 0.5 | Baseline (very small) |
| 0.05 | 0.2 - 1.0 | Starting to rise |
| **0.10** | **2.0 - 8.0** | **🔺 COPPER PEAK** |
| 0.15 | 1.5 - 6.0 | After peak |
| 0.20 | 1.0 - 4.0 | Declining |
| 0.30 | 0.5 - 2.0 | Back to baseline |
| 0.50+ | 0.2 - 1.0 | Stable baseline |

### Key Characteristics:

1. **Copper Oxidation Peak:**
   - **Voltage:** 0.08V - 0.15V (typically ~0.10V)
   - **Current:** 2-10 µA (depends on surface area, concentration)
   - **Shape:** Sharp peak, then decline

2. **Baseline Current:**
   - **0V:** 0.1 - 0.5 µA (very small)
   - **After peak:** Returns to 0.2 - 1.0 µA
   - **Should be stable**, not continuously increasing

3. **What You Should See:**
   ```
   Voltage: 0.00V → Current: 0.3 µA
   Voltage: 0.05V → Current: 0.8 µA
   Voltage: 0.10V → Current: 5.2 µA  ← PEAK
   Voltage: 0.15V → Current: 2.1 µA
   Voltage: 0.20V → Current: 0.9 µA
   Voltage: 0.30V → Current: 0.4 µA
   Voltage: 0.50V → Current: 0.3 µA
   ```

---

## ❌ What's WRONG: Continuously Increasing Current

If current continuously increases, this indicates:

### Problem 1: Unstable Reference Electrode
- **Symptom:** Current keeps rising, no peak
- **Cause:** RE potential drifting
- **Fix:** 
  - Use fresh, clean copper wire for RE
  - Sand RE to remove oxide
  - Ensure RE is stable in solution

### Problem 2: Circuit Not Properly Configured
- **Symptom:** Current increases linearly with voltage
- **Cause:** TIA not working (virtual ground wrong)
- **Fix:**
  - **Pin 3 MUST be at GND (0V)**
  - Check all connections
  - Verify feedback resistor

### Problem 3: Charging/Discharging Effect
- **Symptom:** Current increases then stabilizes
- **Cause:** Electrochemical cell charging
- **Fix:**
  - Increase settle time (SETTLE_TIME_MS)
  - Wait longer at each voltage step
  - Use fresh electrolyte

### Problem 4: No Proper Feedback
- **Symptom:** Current doesn't follow expected pattern
- **Cause:** Control op-amp not working correctly
- **Fix:**
  - Check RE connection to LM358 #1 Pin 2
  - Check CE connection to LM358 #1 Pin 1
  - Verify feedback loop

---

## 🔧 Troubleshooting Steps

### Step 1: Fix Virtual Ground
```
1. Measure LM358 #2 Pin 3 to GND
   → Should be 0V
   → If not 0V: Connect Pin 3 to GND!

2. Measure LM358 #2 Pin 2 to GND
   → Should be < 0.1V (virtual ground)
   → If > 0.5V: Pin 3 not at GND or op-amp broken
```

### Step 2: Test with Known Setup
```
1. Use 3 clean copper wires
2. Submerge 2-3cm in 0.1M KCl solution
3. Space electrodes 1-2cm apart
4. Run scan from 0V to 0.5V
5. Should see peak at ~0.10V
```

### Step 3: Check Current Behavior
```
✅ GOOD: Current peaks at 0.10V, then decreases
❌ BAD: Current continuously increases
❌ BAD: Current stays constant
❌ BAD: Current is zero
```

---

## 📈 Typical Copper LSV Curve

```
Current (µA)
    |
 10 |                    ╱╲
    |                  ╱  ╲
  5 |                ╱    ╲
    |              ╱        ╲
  2 |            ╱            ╲
    |          ╱                ╲
  1 |        ╱                    ╲
    |      ╱                        ╲
  0 |____╱____________________________╲___
    0.0  0.1  0.2  0.3  0.4  0.5  Voltage (V)
              ↑
          Peak at ~0.10V
```

---

## 🎯 Summary

**For 3 copper wires, you should see:**
- ✅ Peak at 0.08-0.15V (typically 0.10V)
- ✅ Peak current: 2-10 µA
- ✅ Current decreases after peak
- ✅ Stable baseline (~0.3 µA)

**If current continuously increases:**
- ❌ Virtual ground not working (Pin 3 not at GND)
- ❌ Reference electrode unstable
- ❌ Circuit not properly configured

**FIX FIRST:** Connect LM358 #2 Pin 3 to GND (0V)!

