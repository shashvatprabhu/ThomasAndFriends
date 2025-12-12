# LSV Potentiostat for Gold Purity Analysis

![LSV Potentiostat](https://images.unsplash.com/photo-1553874386-b86f115c18c7?ixlib=rb-4.0.3&auto=format&fit=crop&w=800&q=80)

## Overview

This project implements a Linear Sweep Voltammetry (LSV) potentiostat using an ESP32-WROOM-32E microcontroller for gold purity analysis. The system performs electrochemical measurements to determine the karat value of gold samples by analyzing their voltammetric response.

## Table of Contents
- [Theory](#theory)
- [Hardware Components](#hardware-components)
- [Circuit Design](#circuit-design)
- [Software Architecture](#software-architecture)
- [Setup and Usage](#setup-and-usage)
- [Results and Analysis](#results-and-analysis)
- [Troubleshooting](#troubleshooting)

## Theory

Linear Sweep Voltammetry (LSV) is an electrochemical technique that measures the current at a working electrode while sweeping the potential linearly between two values. The resulting current-voltage curve (voltammogram) reveals information about redox reactions occurring at the electrode surface.

![Voltammetry Graph](https://upload.wikimedia.org/wikipedia/commons/thumb/f/fd/Cyclic_voltammetry.svg/800px-Cyclic_voltammetry.svg.png)

In gold purity analysis, LSV detects metal oxidation and reduction peaks that are characteristic of different alloy compositions. The peak currents and potentials correlate with the concentration of different metals in the sample, allowing for karat determination.

## Hardware Components

### Microcontroller
- **ESP32-WROOM-32E**: Main controller with Wi-Fi connectivity for remote monitoring

### I2C Configuration
- **SCL Pin**: GPIO 22
- **SDA Pin**: GPIO 21
- **Clock Speed**: 400kHz (fast mode)
- **Timeout**: 1000ms

### Sensor Components
- **MCP4725**: 12-bit Digital-to-Analog Converter for applying controlled voltages
  - I2C Address: 0x60
- **ADS1115**: 16-bit Analog-to-Digital Converter for measuring current responses
  - I2C Address: 0x48

### Operational Amplifiers
- **LM358**: Dual operational amplifier for:
  - Voltage follower circuit
  - Transimpedance amplifier (TIA) for converting current to voltage

### Electrodes
- **Working Electrode (WE)**: Sample under test (gold item)
- **Reference Electrode (RE)**: Stable reference potential
- **Counter Electrode (CE)**: Completes the electrochemical cell circuit

![Electrochemical Cell](https://upload.wikimedia.org/wikipedia/commons/thumb/7/77/Three_electrode_setup_for_electrochemistry.svg/800px-Three_electrode_setup_for_electrochemistry.svg.png)

## Circuit Design

The potentiostat follows a three-electrode configuration with operational amplifiers arranged for:

1. **Voltage Follower**: Maintains the reference electrode potential
2. **Transimpedance Amplifier (TIA)**: Converts current from the working electrode to voltage

### Critical Connections & Setup
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

**Critical**: The virtual ground at pin 3 must be connected to GND (0V), otherwise the transimpedance amplifier will not function correctly and will produce incorrect readings.

### Expected Readings for Copper Test
When testing with copper electrodes in electrolyte solution (0.1M KCl or 0.5M H₂SO₄), the system should detect characteristic redox peaks that confirm proper operation.

## Software Architecture

### Main Components
- **I2C Communication Layer**: Manages MCP4725 DAC and ADS1115 ADC communication
- **LSV Scan Module**: Controls voltage sweeps and data acquisition
- **Peak Analysis**: Detects electrochemical peaks in voltammograms
- **Karat Estimation**: Calculates gold purity based on peak analysis
- **HTTP Server**: Web interface for remote monitoring and control with mDNS discovery

### Key Parameters
- Start Voltage: 0.0V
- End Voltage: 1.8V (limited by LM358 input range)
- Step Size: 0.01V
- Settle Time: 1000ms
- Scan Interval: 6000ms
- Maximum Data Points: 300

### Karat Classification
- **24K Gold**: Up to 99.9% purity
- **22K Gold**: 91.7% purity
- **18K Gold**: 75% purity
- **14K Gold**: 58.3% purity

![ESP32 Circuit](https://electronicsforu.com/wp-content/uploads/2021/02/ESP32-Development-Board.jpg)

## Setup and Usage

### Prerequisites
- ESP-IDF v4.1 or higher
- CMake 3.16 or higher

### Installation Steps
1. Configure Wi-Fi credentials in `main/Kconfig.projbuild`
2. Build the project:
   ```
   cd LSV_FINAL
   idf.py build
   ```
3. Flash to ESP32:
   ```
   idf.py flash
   ```
4. Access the web interface at the displayed IP address

### Operating Procedure
1. Prepare electrochemical cell with appropriate electrolyte (0.1M KCl or 0.5M H₂SO₄)
2. Connect electrodes (Working, Reference, Counter) to the circuit
3. Start scan via web interface
4. Monitor results in real-time

## Results and Analysis

The system performs real-time peak analysis to detect:
- **Copper peaks**: Indicate presence of copper in gold alloys
- **Silver peaks**: Indicate presence of silver in gold alloys

Based on peak magnitudes, the software estimates:
- Karat value (24K, 22K, 18K, 14K)
- Gold percentage
- Confidence level

![Peak Detection](https://www.researchgate.net/profile/Luiz-Magnago-2/publication/282046862/figure/fig1/AS:669315335954432@1536603159655/Typical-cyclic-voltammetry-response-of-FeCN-4-3-ferrocene-mixture-The-anodic-oxidation.png)

## Troubleshooting

### Common Issues

#### Op-Amp Saturation
- **Problem**: Constant readings (e.g., A0 = 1.97V, A1 = 1.66V)
- **Cause**: Op-amps saturated, not responding to input changes
- **Solution**: Check electrode connections and electrolyte contact

#### Virtual Ground Issue
- **Problem**: Incorrect current measurements
- **Cause**: TIA pin 3 not connected to GND (should be 0V)
- **Solution**: Verify pin 3 of LM358 #2 is connected to ground

#### No Response from I2C Devices
- **Solution**: Run I2C scan to verify MCP4725 (0x60) and ADS1115 (0x48) are detected

## Technical Specifications

- **Microcontroller Platform**: ESP-IDF for ESP32-WROOM-32E
- **ADC Resolution**: 16-bit (ADS1115)
- **DAC Resolution**: 12-bit (MCP4725)
- **Scan Range**: 0.0V to 1.8V
- **Web Interface**: Bootstrap-based with real-time charting
- **Communication Protocol**: I2C at 400kHz

## Applications

- Gold jewelry authentication
- Purity verification in precious metal trading
- Quality control in gold processing
- Educational electrochemistry demonstrations

## Credits

Developed by the Smart India Hackathon 2025 Team.

## License

For educational and research purposes.