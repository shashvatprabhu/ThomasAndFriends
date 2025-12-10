# TAF - Testing and Analysis Framework

**ESP32-Based Density Measurement System with WebSocket Interface**

Smart India Hackathon 2025 Project

---

## Overview

TAF (Testing and Analysis Framework) is an ESP32-based platform for density measurement using Archimedes' Principle. The system features a web-based interface with real-time WebSocket updates for interactive testing and visualization.

## Project Structure

```
taf/
├── README.md                        # This file
├── CMakeLists.txt                   # Main ESP-IDF build configuration
├── sdkconfig.defaults               # Default ESP32 configuration
│
├── main/                            # Main application code
│   ├── CMakeLists.txt              # Main component build config
│   ├── taf_main.c                  # Application entry point
│   ├── wifi_handler.c              # WiFi connection management
│   ├── taf_websocket_handler.c     # WebSocket server implementation
│   ├── Kconfig.projbuild           # Configuration menu
│   ├── idf_component.yml           # Component dependencies
│   └── include/                    # Header files
│       ├── taf_websocket_handler.h
│       └── wifi_handler.h
│
├── frontend/                        # Web interface
│   └── index.html                  # Web UI (embedded in firmware)
│
├── components/                      # Local shared components
│   └── websocket/                  # WebSocket server library
│       ├── CMakeLists.txt
│       ├── Kconfig
│       ├── README.md
│       ├── include/
│       │   ├── websocket.h
│       │   └── websocket_server.h
│       ├── websocket.c
│       └── websocket_server.c
│
└── managed_components/              # ESP-IDF managed dependencies
    └── espressif__mdns/            # mDNS service discovery
```

---

## Features

- **Density Measurement:** Uses Archimedes' Principle (comparing weight in air vs water)
- **Web Interface:** Modern, responsive HTML5 frontend
- **Real-time Updates:** WebSocket-based communication
- **WiFi Connectivity:** Configurable SSID and password
- **mDNS Support:** Access via `taf.local` hostname
- **Embedded Frontend:** No external web server required

---

## Hardware Requirements

- ESP32 Development Board
- Load cells or digital scale (for weight measurements)
- USB cable for programming and power
- WiFi network for web interface access

---

## Quick Start

### 1. Configure WiFi Settings

```bash
idf.py menuconfig
```

Navigate to `TAF Configuration` and set:
- WiFi SSID
- WiFi Password

### 2. Build and Flash

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### 3. Access Web Interface

After ESP32 connects to WiFi, open browser to:
- `http://taf.local` (if mDNS is supported)
- Or use the IP address shown in serial monitor

---

## Building the Project

This project follows the standard ESP-IDF build process:

```bash
# Clean build (optional)
idf.py fullclean

# Configure project
idf.py menuconfig

# Build
idf.py build

# Flash to ESP32
idf.py -p PORT flash

# Monitor serial output
idf.py -p PORT monitor

# Flash and monitor in one command
idf.py -p PORT flash monitor
```

---

## Project Integration

This project is part of the Wall-E framework. It uses shared components from `../components/` including:
- SRA Board components (if applicable)
- Common utilities

The project structure follows the Wall-E convention where each numbered directory (e.g., `5_line_following`, `6_self_balancing`) is a standalone ESP-IDF project.

---

## Other Wall-E Projects

For other ESP32 projects in the Wall-E framework, see:

- `../1_led_matrix/` - LED Matrix Game of Life
- `../2_LSA/` - Line Sensor Array
- `../3_MPU/` - MPU6050 IMU Integration
- `../4_PWM/` - PWM Motor Control
- `../5_line_following/` - Line Following Robot
- `../6_self_balancing/` - Self-Balancing Robot

---

## Components

### Local Components

- **websocket/** - WebSocket server library for real-time bidirectional communication

### Shared Components (from ../components/)

This project uses shared components from the Wall-E framework including SRA board drivers and utilities.

---

## ESP-IDF Setup

This project requires ESP-IDF. Follow the [official installation guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/):

```bash
# Quick setup (Linux/Mac)
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32

# Activate environment (run this in every new terminal)
. ~/esp/esp-idf/export.sh
```

---

## Troubleshooting

### WiFi Connection Issues
- Double-check SSID and password in `idf.py menuconfig`
- Ensure ESP32 is within WiFi range
- Check serial monitor for error messages
- Verify WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)

### WebSocket Connection Fails
- Verify ESP32 IP address from serial monitor
- Ensure computer is on same network as ESP32
- Check firewall settings
- Try disabling VPN if active

### Build Errors
- Ensure ESP-IDF environment is activated: `. ~/esp/esp-idf/export.sh`
- Clean build directory: `idf.py fullclean`
- Update ESP-IDF: `git pull` in esp-idf directory
- Check that you're in the correct directory

---

## License

Open-source for educational purposes. Part of the Smart India Hackathon 2025 project.

---

## Acknowledgments

- ESP-IDF framework by Espressif
- WebSocket library for ESP32
- Smart India Hackathon 2025 organizers
