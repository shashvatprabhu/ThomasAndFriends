# Data Flow Explanation

## Physical Signal Path (Hardware)

```
1. NE555 Oscillator (~100kHz)
   └─> Generates AC signal
   
2. Coil (90 turns, 0.7mm wire)
   └─> When metal approaches, eddy currents change coil impedance
   
3. Current Sense Resistor (100Ω)
   └─> Voltage drop = I × 100Ω
   └─> This voltage represents coil current
   
4. Voltage Divider (R1=10kΩ, R2=10kΩ)
   └─> Divides voltage by 2 (attenuation)
   └─> Midpoint = V_input × (R2/(R1+R2)) = V_input × 0.5
   
5. LM358 Op-Amp (optional buffer/amplifier)
   └─> May buffer or amplify the signal
   
6. ADS1115 ADC (AIN0 pin)
   └─> Reads voltage at divider midpoint
   └─> 16-bit resolution (±4.096V range)
```

## Software Data Flow

### Step 1: Initialization (`app_main()`)
```
app_main() starts
  ├─> Initialize NVS (non-volatile storage)
  ├─> Print header info
  └─> Call ads1115_init()
      ├─> Configure I2C (GPIO21=SDA, GPIO22=SCL, 400kHz)
      ├─> Scan addresses 0x48-0x4B (find ADS1115)
      └─> Test read config register
```

### Step 2: Main Loop (`while(1)` in `app_main()`)

**Each iteration:**

```
1. measure_voltage(SAMPLES=200)
   │
   ├─> Loop 200 times:
   │   ├─> ads1115_read_single(0, &adc_value)
   │   │   │
   │   │   ├─> Write config register:
   │   │   │   - Channel: AIN0 vs GND
   │   │   │   - Gain: ±4.096V
   │   │   │   - Mode: Single-shot
   │   │   │   - Start conversion
   │   │   │
   │   │   ├─> Wait 10ms (conversion time)
   │   │   │
   │   │   └─> Read conversion register:
   │   │       - I2C write: register address (0x00)
   │   │       - I2C read: 2 bytes (MSB, LSB)
   │   │       - Combine: (MSB << 8) | LSB
   │   │       - Return as int16_t (-32768 to +32767)
   │   │
   │   ├─> Add to sum
   │   └─> Delay 1ms
   │
   └─> Calculate average:
       - avg_counts = sum / 200
       - Convert to volts: ads1115_counts_to_volts()
         - LSB = 4.096V / 32768 = 0.000125V (0.125mV)
         - voltage = counts × LSB
       - Return v_adc (voltage at ADC input)

2. measure_noise()
   │
   ├─> Loop 30 times:
   │   ├─> Read single sample (same as above)
   │   ├─> Track min and max values
   │   └─> Delay 1ms
   │
   └─> Return (max - min) = peak-to-peak noise

3. Calculate derived values:
   │
   ├─> v_input = v_adc × (R1+R2)/R2
   │   = v_adc × (10000+10000)/10000
   │   = v_adc × 2.0
   │   (Reconstructs voltage BEFORE divider)
   │
   └─> current_a = v_input / CURRENT_RESISTOR_OHMS
       = v_input / 100
       (Ohm's law: I = V/R)

4. display_results()
   │
   └─> printf("ADC: %.3fV | Input: %.3fV | I: %.3fmA | Noise: %.1fmV\n")
       Prints to serial console

5. vTaskDelay(100ms)
   └─> Wait 100ms before next iteration
```

## Detailed I2C Communication

### Reading a Single Sample:

```
ESP32 (Master)                    ADS1115 (Slave @ 0x48)
─────────────────                 ──────────────────────

1. Write Config Register:
   START
   ────> [0x90] (0x48 << 1 | WRITE)
   ────> [0x01] (CONFIG register)
   ────> [0x8X] (Config MSB)
   ────> [0xXX] (Config LSB)
   STOP
   
   ADS1115 starts conversion...

2. Wait 10ms (conversion time)

3. Read Conversion Register:
   START
   ────> [0x90] (0x48 << 1 | WRITE)
   ────> [0x00] (CONVERSION register)
   START (repeated start)
   ────> [0x91] (0x48 << 1 | READ)
   <──── [MSB]  (first byte)
   <──── [LSB]  (second byte, NACK)
   STOP
   
   ESP32 combines: value = (MSB << 8) | LSB
```

## Why Readings Are Consecutive

The `while(1)` loop in `app_main()` runs continuously:

```
Time →
│
├─> Measure (200 samples × ~11ms each = ~2.2s)
├─> Measure noise (30 samples × ~11ms = ~0.33s)
├─> Calculate & print
├─> Wait 100ms
│
├─> Measure (200 samples × ~11ms each = ~2.2s)
├─> Measure noise (30 samples × ~11ms = ~0.33s)
├─> Calculate & print
├─> Wait 100ms
│
└─> ... repeats forever
```

**Total time per reading:** ~2.2s + 0.33s + 0.1s ≈ **2.6 seconds**

But you're seeing faster prints because:
- The 200 samples happen quickly (each read is ~11ms)
- The delays are minimal
- Serial output is buffered

## What Each Value Means

1. **ADC: 0.012V**
   - Voltage directly at ADS1115 AIN0 pin
   - This is the divider midpoint voltage

2. **Input: 0.025V**
   - Reconstructed voltage BEFORE the divider
   - = ADC × 2.0 (because divider divides by 2)

3. **I: 0.248mA**
   - Coil current through 100Ω sense resistor
   - = Input / 100Ω

4. **Noise: 0.4mV**
   - Peak-to-peak variation in readings
   - Lower = more stable signal

## Why You Might Not See Changes

If readings don't change when metal approaches:

1. **Signal too small** - Try higher gain (PGA_0_256V instead of PGA_4_096V)
2. **Wrong channel** - Signal might be on AIN1, AIN2, or AIN3
3. **Circuit issue** - Coil/oscillator not working, or wrong connection
4. **Divider values wrong** - R1/R2 might not be 10kΩ each
5. **Sense resistor wrong** - Might not be 100Ω

