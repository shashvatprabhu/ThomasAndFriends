# LSV Potentiostat Project Structure

## Directory Layout

```
LSV_Potentiostat/
│
├── LSV_Potentiostat.ino          # Main Arduino sketch (entry point)
│
├── include/                       # Header files
│   ├── config.h                   # Hardware config and tunable parameters
│   ├── lsv_scan.h                 # LSV scan module header
│   ├── peak_analysis.h            # Peak detection module header
│   └── karat_estimation.h         # Karat estimation module header
│
├── src/                           # Source implementation files
│   ├── lsv_scan.cpp               # LSV scan implementation
│   ├── peak_analysis.cpp          # Peak detection implementation
│   └── karat_estimation.cpp       # Karat estimation implementation
│
├── platformio.ini                 # PlatformIO configuration (optional)
│
├── README.md                      # Comprehensive documentation
├── QUICK_START.md                 # Quick setup guide (30 min)
├── CIRCUIT_DETAILS.md             # Detailed circuit theory & design
└── PROJECT_STRUCTURE.md           # This file
```

---

## File Descriptions

### Main Sketch

**`LSV_Potentiostat.ino`**
- Arduino entry point
- Contains `setup()` and `loop()` functions
- Orchestrates scan → analysis → estimation workflow
- Handles timing and user interface (Serial Monitor)
- **Lines of code:** ~250
- **Purpose:** High-level program flow and user interaction

---

### Configuration Header

**`include/config.h`**
- All tunable parameters in one place
- Hardware pin definitions (I2C, GPIO)
- Scan parameters (voltage range, step size, timing)
- Calibration constants (resistor values, ADC/DAC settings)
- Peak detection thresholds
- Feature flags (enable/disable advanced features)
- **Lines of code:** ~150
- **Purpose:** Easy customization without editing core code

**Key sections:**
```cpp
// I2C Configuration
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22

// LSV Scan Parameters
#define START_VOLTAGE       0.0
#define END_VOLTAGE         1.2
#define STEP_SIZE           0.01

// Calibration Constants
#define R_FEEDBACK          100000.0
#define ADC_GAIN            GAIN_TWO

// Peak Detection
#define CU_THRESHOLD        0.5
#define AG_THRESHOLD        0.3
```

---

### LSV Scan Module

**`include/lsv_scan.h`** + **`src/lsv_scan.cpp`**

Core electrochemical scanning functionality.

**Key functions:**
- `initializeHardware()` - Setup I2C, DAC, ADC
- `performLSVScan()` - Execute voltage sweep and measure currents
- `setDACVoltage(float v)` - Control applied voltage
- `readCurrent()` - Measure current via ADC
- `measureBaseline()` - Record baseline offset
- `exportCSV()` - Output data in CSV format

**Data structure:**
```cpp
typedef struct {
    float voltage[MAX_DATA_POINTS];
    float current[MAX_DATA_POINTS];
    int numPoints;
    float baselineCurrent;
    bool scanComplete;
} LSVData;
```

**Lines of code:** ~400
**Purpose:** Hardware control and raw data acquisition

---

### Peak Analysis Module

**`include/peak_analysis.h`** + **`src/peak_analysis.cpp`**

Processes LSV data to detect oxidation peaks.

**Key functions:**
- `analyzePeaks(PeakAnalysisResult* result)` - Main analysis routine
- `findPeakInRange(vMin, vMax, peakInfo)` - Detect peak in voltage window
- `calculatePeakArea(vMin, vMax)` - Integrate peak (quantitative)
- `applySmoothingFilter(windowSize)` - Noise reduction
- `printInterpretation(result)` - Electrochemistry explanation

**Data structure:**
```cpp
typedef struct {
    bool detected;
    float peakHeight;
    float peakVoltage;
    float peakArea;
} PeakInfo;

typedef struct {
    PeakInfo copper;
    PeakInfo silver;
    PeakInfo nickel;
    PeakInfo zinc;
    float totalMagnitude;
    int numPeaksDetected;
} PeakAnalysisResult;
```

**Lines of code:** ~350
**Purpose:** Extract meaningful features from raw current data

---

### Karat Estimation Module

**`include/karat_estimation.h`** + **`src/karat_estimation.cpp`**

Estimates gold purity from peak analysis.

**Key functions:**
- `estimateKarat(peaks, estimate)` - Main estimation routine
- `classifyByTotalPeak(totalPeakHeight)` - Simple threshold method
- `classifyByMetalRatio(cuPeak, agPeak)` - Advanced ratio method
- `calculateConfidence(peaks)` - Reliability metric
- `printKaratEstimation(estimate)` - Detailed report
- `printCalibrationNotes()` - User guidance

**Data structure:**
```cpp
typedef struct {
    int estimatedKarat;           // 24, 22, 18, 14, etc.
    float goldPercentage;         // 99.9%, 75.0%, etc.
    float confidence;             // 0-100%
    float alloyContent;
    char classification[50];
    char reasoning[200];
} KaratEstimate;
```

**Lines of code:** ~300
**Purpose:** Convert electrochemical data to practical gold purity estimate

---

## Code Flow Diagram

```
┌─────────────────────────────────────────────────────┐
│  LSV_Potentiostat.ino                               │
│  ┌────────────────────────────────────────────┐    │
│  │ setup()                                     │    │
│  │  • Serial.begin(115200)                     │    │
│  │  • initializeHardware() ───────────┐        │    │
│  │  • Print welcome & checklist       │        │    │
│  │  • Countdown timer                 │        │    │
│  └────────────────────────────────────┼────────┘    │
│                                       │             │
│  ┌────────────────────────────────────▼────────┐    │
│  │ loop()                                      │    │
│  │  ┌──────────────────────────────────────┐  │    │
│  │  │ Every 30 seconds:                    │  │    │
│  │  │  1. performLSVScan() ────────┐       │  │    │
│  │  │  2. errorCheck()             │       │  │    │
│  │  │  3. analyzePeaks()  ─────────┼──┐    │  │    │
│  │  │  4. estimateKarat() ─────────┼──┼─┐  │  │    │
│  │  │  5. returnToZero()           │  │ │  │  │    │
│  │  └──────────────────────────────┼──┼─┼──┘  │    │
│  └───────────────────────────────────┼──┼─┼─────┘    │
└──────────────────────────────────────┼──┼─┼──────────┘
                                       │  │ │
         ┌─────────────────────────────┘  │ │
         │                                │ │
┌────────▼──────────────────┐  ┌──────────▼─▼─────────────────┐
│  lsv_scan.cpp              │  │  peak_analysis.cpp           │
│  • Hardware init           │  │  • Find peaks in ranges      │
│  • Voltage sweep           │  │  • Peak area calculation     │
│  • Current measurement     │  │  • Smoothing filters         │
│  • Data storage            │  │  • Electrochemistry interp.  │
│  • CSV export              │  │                              │
└────────────────────────────┘  └──────────┬───────────────────┘
         ▲                                 │
         │ Uses: ads, dac objects          │
         │ Hardware: MCP4725, ADS1115      │
         │                                 │
         │                  ┌──────────────▼────────────────┐
         │                  │  karat_estimation.cpp         │
         │                  │  • Total peak classification  │
         │                  │  • Metal ratio classification │
         │                  │  • Confidence calculation     │
         │                  │  • Karat → % gold conversion  │
         │                  │  • Calibration notes          │
         │                  └───────────────────────────────┘
         │                                 │
         │                                 │
         └─────────────────┬───────────────┘
                           │
                    ┌──────▼──────┐
                    │  config.h   │
                    │  All params │
                    └─────────────┘
```

---

## Compilation Details

### Arduino IDE Compilation

1. **Pre-processing:** `.ino` → `.cpp` conversion
2. **Header includes:** All `include/*.h` files processed
3. **Compilation:** Each `.cpp` file → `.o` object file
4. **Linking:** All `.o` files + libraries → `.bin` firmware
5. **Upload:** `.bin` flashed to ESP32 via USB

### Dependencies

**External libraries (auto-downloaded by Arduino IDE):**
- `Wire.h` - I2C communication (built-in)
- `Adafruit_ADS1X15.h` - ADS1115 ADC driver
- `Adafruit_MCP4725.h` - MCP4725 DAC driver
- `Adafruit_BusIO.h` - I2C/SPI abstraction (dependency)

**Internal dependencies:**
```
LSV_Potentiostat.ino
  ↓ includes
  ├── include/config.h
  ├── include/lsv_scan.h
  │     ↓ includes
  │     └── include/config.h
  ├── include/peak_analysis.h
  │     ↓ includes
  │     ├── include/config.h
  │     └── include/lsv_scan.h
  └── include/karat_estimation.h
        ↓ includes
        ├── include/config.h
        └── include/peak_analysis.h
```

### Memory Usage (Typical)

**ESP32 has plenty of resources:**
- **Flash:** ~150KB used / 4MB available (~3.75% usage)
- **SRAM:** ~25KB used / 520KB available (~4.8% usage)
- **Stack:** ~8KB typical usage

**Data arrays:**
- `float voltage[300]` = 1200 bytes
- `float current[300]` = 1200 bytes
- **Total:** ~2.4KB for scan data (trivial for ESP32)

---

## Modular Design Benefits

### Easy Customization

Want to change scan parameters?
→ Edit **config.h** only

Want to improve peak detection?
→ Edit **peak_analysis.cpp** only

Want different karat thresholds?
→ Edit **config.h** or **karat_estimation.cpp**

### Code Reusability

**Use LSV module for other applications:**
- Heavy metal detection in water
- Battery electrolyte analysis
- Corrosion studies
- Other voltammetry techniques (CV, SWV, DPV)

**Just include the module:**
```cpp
#include "include/lsv_scan.h"
// Now use performLSVScan() in your own project
```

### Testing and Debugging

**Test each module independently:**

1. Test hardware:
   ```cpp
   initializeHardware(); // Should print success messages
   ```

2. Test DAC:
   ```cpp
   setDACVoltage(1.0); // Measure with multimeter
   ```

3. Test ADC:
   ```cpp
   float current = readCurrent(); // Apply known voltage
   ```

4. Test full scan:
   ```cpp
   performLSVScan(); // Check data collection
   ```

5. Test analysis:
   ```cpp
   analyzePeaks(&results); // Verify peak detection
   ```

---

## Documentation Files

### README.md (Comprehensive Guide)
- **Lines:** ~1200
- **Audience:** All users (beginners to advanced)
- **Contents:**
  - Hardware requirements
  - Circuit diagrams
  - Detailed setup instructions
  - Calibration procedure
  - Result interpretation
  - Troubleshooting
  - Theory of operation

### QUICK_START.md (30-Minute Setup)
- **Lines:** ~400
- **Audience:** Beginners, hackathon demos
- **Contents:**
  - Minimal component list
  - Step-by-step assembly (10 min)
  - Software upload (5 min)
  - First test (5 min)
  - Gold sample test (5 min)
  - Quick troubleshooting

### CIRCUIT_DETAILS.md (Engineering Deep Dive)
- **Lines:** ~800
- **Audience:** Advanced users, circuit designers
- **Contents:**
  - Circuit theory (potentiostat, transimpedance amp)
  - Component selection rationale
  - Design equations
  - Advanced modifications
  - Testing procedures
  - PCB layout guidelines

### PROJECT_STRUCTURE.md (This File)
- **Lines:** ~300
- **Audience:** Developers, contributors
- **Contents:**
  - File organization
  - Code architecture
  - Module descriptions
  - Compilation details

---

## Development Workflow

### For Hackathon Demo

1. **Use defaults in config.h** (already optimized)
2. **Upload LSV_Potentiostat.ino**
3. **Follow QUICK_START.md** (30 minutes)
4. **Calibrate with 1-2 known samples** (optional but recommended)
5. **Demo with unknown samples**

### For Research/Production

1. **Read CIRCUIT_DETAILS.md** (understand theory)
2. **Modify config.h** (customize for your setup)
3. **Extensive calibration** (10+ known samples)
4. **Statistical validation** (multiple scans, error analysis)
5. **Consider hardware upgrades** (precision op-amps, platinum electrodes)

### For Education

1. **Read README.md** (full documentation)
2. **Study code modules** (well-commented)
3. **Run experiments** (vary electrolyte, voltage range, etc.)
4. **Modify peak_analysis.cpp** (implement new algorithms)
5. **Learn electrochemistry** (references in README.md)

---

## Version History

**v1.0.0** (2025-12-09) - Initial Release
- Complete LSV functionality
- Peak detection (Cu, Ag, Ni, Zn)
- Karat estimation (24K, 22K, 18K, 14K)
- Comprehensive documentation
- Production-ready for hackathon demo

**Future Versions (Planned):**
- v1.1: Cyclic Voltammetry (CV) mode
- v1.2: Bluetooth data export
- v1.3: Machine learning classification
- v2.0: Differential Pulse Voltammetry (DPV)

---

## Contributing

### Code Style

- **Naming:** camelCase for functions, UPPER_CASE for defines
- **Comments:** Explain WHY, not WHAT
- **Functions:** One function = one purpose
- **Headers:** Document all parameters and return values

### Adding New Features

1. **Update config.h** if new parameters needed
2. **Create new module** (header + implementation) if major feature
3. **Update main .ino** to call new functions
4. **Update README.md** with usage instructions
5. **Test thoroughly** before committing

### Bug Reports

Include:
- Hardware configuration (ESP32 model, modules used)
- Software version
- Serial Monitor output (full error messages)
- Expected vs actual behavior
- Steps to reproduce

---

## License and Credits

**Project:** LSV Potentiostat Gold Purity Analyzer
**Team:** Smart India Hackathon 2025
**Purpose:** Educational and hackathon demonstration
**Status:** Open-source, free to use and modify

**Libraries Used:**
- Adafruit ADS1X15 (BSD License)
- Adafruit MCP4725 (BSD License)
- Arduino Core for ESP32 (LGPL 2.1)

**References:**
- Bard & Faulkner: "Electrochemical Methods"
- Scholz: "Electroanalytical Methods"
- Various gold alloy composition standards

---

## Contact and Support

For questions about this codebase:
- Read documentation files first (likely has answer!)
- Check troubleshooting sections
- Review code comments (extensively documented)

For electrochemistry questions:
- See References section in README.md
- Consult textbooks on voltammetry
- Online resources: LibreTexts Chemistry

---

**Total Project Stats:**

| Metric | Value |
|--------|-------|
| Total files | 13 |
| Code files (.ino, .h, .cpp) | 7 |
| Documentation files (.md) | 5 |
| Config files (.ini) | 1 |
| Total lines of code | ~1450 |
| Total lines of documentation | ~2700 |
| Documentation:Code ratio | 1.86:1 (very well documented!) |
| Estimated development time | 12-16 hours |
| Setup time for new user | 30 minutes |
| Calibration time | 1-2 hours |

---

**This modular architecture makes the code:**
✅ Easy to understand
✅ Easy to modify
✅ Easy to debug
✅ Easy to extend
✅ Professional quality
✅ Educational value
✅ Hackathon-ready

**Happy coding and testing! 🚀⚗️**
