# ESP-IDF Build Instructions for ESP32-WROOM-32E

**Native ESP32 Development - No Arduino Bullshit!**

This project uses **ESP-IDF** (Espressif IoT Development Framework) - the official native C framework for ESP32 development.

---

## 🔧 Prerequisites

### Install ESP-IDF

**Linux/MacOS:**
```bash
# Install dependencies
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# Clone ESP-IDF
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.1  # Use stable version

# Install ESP-IDF tools
./install.sh esp32

# Set up environment variables (add to ~/.bashrc or ~/.zshrc)
. ~/esp/esp-idf/export.sh
```

**Windows:**
- Download ESP-IDF installer from: https://dl.espressif.com/dl/esp-idf/
- Run installer and follow wizard
- ESP-IDF will be installed in `C:\Espressif\`

---

## 🚀 Build and Flash

### Step 1: Navigate to Project

```bash
cd /Users/vrushtee/ThomasAndFriends/LSV_Potentiostat
```

### Step 2: Set ESP-IDF Environment

```bash
# Run this in every new terminal session
. ~/esp/esp-idf/export.sh

# Or on Windows:
# %userprofile%\esp\esp-idf\export.bat
```

### Step 3: Configure Project (First Time Only)

```bash
idf.py set-target esp32

# Optional: Configure project settings
idf.py menuconfig
```

### Step 4: Build

```bash
idf.py build
```

### Step 5: Flash to ESP32-WROOM-32E

```bash
# Auto-detect port and flash
idf.py flash

# Or specify port manually:
# idf.py -p /dev/ttyUSB0 flash  (Linux)
# idf.py -p COM3 flash          (Windows)
```

### Step 6: Monitor Output

```bash
idf.py monitor

# Or combine flash + monitor:
idf.py flash monitor

# Exit monitor: Ctrl+]
```

---

## 📁 Project Structure

```
LSV_Potentiostat/
├── CMakeLists.txt           # Main build configuration
├── main/
│   ├── CMakeLists.txt       # Component build configuration
│   ├── main.c               # Entry point (app_main)
│   ├── config.h             # All configuration parameters
│   ├── i2c_devices.c/h      # MCP4725 & ADS1115 drivers
│   ├── lsv_scan.c/h         # LSV scanning logic
│   ├── peak_analysis.c/h    # Peak detection
│   └── karat_estimation.c/h # Karat estimation
└── build/                   # Build output (generated)
```

---

## ⚙️ Configuration

### Edit Hardware Settings

Open `main/config.h` and modify:

```c
// GPIO pins
#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21

// Scan parameters
#define START_VOLTAGE               0.0f
#define END_VOLTAGE                 1.2f
#define STEP_SIZE                   0.01f
#define SETTLE_TIME_MS              80

// Calibration thresholds
#define KARAT_24_MAX                0.5f
#define KARAT_22_MAX                2.0f
#define KARAT_18_MAX                5.0f
```

After editing, rebuild and reflash:
```bash
idf.py build flash
```

---

## 🔍 Monitoring and Debugging

### View Logs

```bash
idf.py monitor
```

### Change Log Level

In `main/main.c`, change:
```c
esp_log_level_set("*", ESP_LOG_INFO);  // Default
esp_log_level_set("*", ESP_LOG_DEBUG); // More verbose
esp_log_level_set("*", ESP_LOG_WARN);  // Less verbose
```

### Erase Flash (Clean Start)

```bash
idf.py erase-flash
idf.py flash
```

---

## 🐛 Troubleshooting

### "Command not found: idf.py"

Run the export script:
```bash
. ~/esp/esp-idf/export.sh
```

### "Port not found" or "Permission denied"

**Linux:**
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER
# Log out and log back in
```

**macOS:**
```bash
# Port is usually /dev/cu.usbserial-*
ls /dev/cu.*
idf.py -p /dev/cu.usbserial-XXXX flash
```

**Windows:**
- Check Device Manager for COM port number
- Use `idf.py -p COMX flash`

### Build Errors

```bash
# Clean build
idf.py fullclean
idf.py build
```

### I2C Device Not Found

- Check GPIO pin connections (GPIO21=SDA, GPIO22=SCL)
- Verify 3.3V power and GND connections
- Run I2C scanner to detect devices:
  ```bash
  # Add to code and rebuild
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  // ... scan I2C bus from 0x00 to 0x7F
  ```

---

## 📊 Expected Output

```
I (123) MAIN: ╔═══════════════════════════════════════╗
I (123) MAIN: ║  LSV POTENTIOSTAT - ESP32-WROOM-32E   ║
I (123) MAIN: ║  Gold Purity Analyzer                 ║
I (123) MAIN: ║  Smart India Hackathon 2025           ║
I (123) MAIN: ╚═══════════════════════════════════════╝
I (234) LSV_SCAN: Initializing I2C bus...
I (235) I2C_DEVICES: I2C initialized on GPIO21 (SDA) and GPIO22 (SCL)
I (245) I2C_DEVICES: MCP4725 found at address 0x60
I (256) I2C_DEVICES: ADS1115 found at address 0x48
I (267) LSV_SCAN: System ready!
...
```

---

## 🎯 Quick Commands Reference

| Command | Purpose |
|---------|---------|
| `idf.py build` | Compile project |
| `idf.py flash` | Upload to ESP32 |
| `idf.py monitor` | View serial output |
| `idf.py flash monitor` | Flash and monitor |
| `idf.py clean` | Clean build files |
| `idf.py fullclean` | Deep clean |
| `idf.py erase-flash` | Erase entire flash |
| `idf.py menuconfig` | Configure project settings |
| `idf.py set-target esp32` | Set chip target |

---

## 🔬 Hardware Requirements

- **ESP32-WROOM-32E** module (or any ESP32 variant)
- MCP4725 12-bit DAC module
- ADS1115 16-bit ADC module
- LM358 dual op-amp
- 100kΩ resistor (feedback)
- Breadboard + wires
- Electrochemical cell (3-electrode)

**Connections:**
- GPIO21 → SDA (both MCP4725 and ADS1115)
- GPIO22 → SCL (both MCP4725 and ADS1115)
- 3.3V → VDD for all modules
- GND → GND for all modules

See `README.md` for complete circuit details.

---

## 📚 ESP-IDF Documentation

- Official Docs: https://docs.espressif.com/projects/esp-idf/
- I2C Driver API: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html
- GPIO API: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html
- FreeRTOS: https://www.freertos.org/

---

## ✅ Success Checklist

After flashing:

☐ Serial monitor shows "System ready!" message
☐ MCP4725 detected at address 0x60
☐ ADS1115 detected at address 0x48
☐ Scan starts after 10-second countdown
☐ Voltage/current data displayed during scan
☐ Peak analysis results shown
☐ Karat estimation printed

---

**Now you have a proper ESP-IDF project with pure C code!** 🚀

No Arduino framework bullshit - just native ESP32 development with full control!
