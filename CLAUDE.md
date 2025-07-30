# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a **real-time cryptocurrency price display application** for ESP32-based "Cheap Yellow Display" (CYD) boards from Sunton. The project uses PlatformIO with the Arduino framework and LVGL graphics library to create a modern touchscreen interface for monitoring cryptocurrency prices with live updates from CoinMarketCap API.

### Key Features

- Real-time crypto price updates with CoinMarketCap API integration
- Modern LVGL-based touchscreen GUI with color-coded trend indicators
- Multi-currency support for simultaneous cryptocurrency monitoring
- Hardware integration with RGB LED status indicators and automatic brightness
- Compatible with 40+ different Sunton ESP32 display board variants
- Power-optimized for battery operation with memory and size optimizations

## Build and Development Commands

### Prerequisites

- **PlatformIO** must be installed (`python -m platformio` or `pio` command)
- **CoinMarketCap API Key** for cryptocurrency data (get free key at coinmarketcap.com/api)
- Project uses ESP32 Arduino framework
- Configuration is handled through built-in HTML web interface

### Core Commands

```bash
# Build the project
platformio run

# Build for specific environment (default: esp32-cyd)
platformio run -e esp32-cyd

# Upload to device
platformio run -t upload

# Upload and monitor serial output
platformio run -t upload -t monitor

# Monitor serial output only
platformio run -t monitor

# Clean build files
platformio run -t clean

# Update dependencies
platformio lib update
```

### Configuration

The project uses a built-in web interface for configuration. On first boot, the device creates a WiFi access point for initial setup where you can configure WiFi credentials, API key, and cryptocurrency selections through an HTML interface.

### Development Workflow

1. The project uses `esp32-2432S028R` board definition by default (configured as `esp32-cyd` environment)
2. Serial monitor runs at 115200 baud with RTS/DTR disabled
3. ESP32 exception decoder is enabled for debugging crashes
4. Configuration is handled through built-in web interface
5. Use `find_coin_id.py` utility to discover CoinMarketCap IDs for web configuration

## Architecture Overview

### Hardware Platform

- **Target Board**: ESP32-2432S028R (CYD - Cheap Yellow Display)
- **Display**: 240x320 ILI9341 TFT with SPI interface
- **Touch**: XPT2046 resistive touchscreen controller
- **Additional Features**: RGB LED, CdS light sensor, speaker output, TF card slot

### Key Components

#### Display System

- **LVGL**: Modern embedded GUI library (v9.2.2) for creating the user interface
- **esp32_smartdisplay**: Hardware abstraction layer that provides unified API for various ESP32 display boards
- **Frame Buffer**: 32KB LVGL memory pool optimized for ESP32 memory constraints

#### Hardware Abstraction

- **Board Definitions**: JSON-based board configurations in `boards/` directory from `platformio-espressif32-sunton` submodule
- **Pin Mapping**: All hardware pins (display, touch, LEDs, etc.) defined via preprocessor macros in board JSON
- **Multi-Board Support**: Single codebase supports 40+ different Sunton ESP32 display variants

#### Code Structure

```
src/
├── main.cpp                    - Application entry point with LVGL setup
├── ApplicationController.cpp   - Main application lifecycle management
├── CryptoDataManager.cpp       - CoinMarketCap API communication and data handling
├── NetworkManager.cpp          - WiFi connection, web configuration interface, and reconnection logic
├── DisplayManager.cpp          - LVGL-based user interface management
├── HardwareController.cpp      - RGB LED and sensor hardware control
├── ApplicationStateManager.cpp - Application state and timing management
└── StatusCalculator.cpp        - Status calculation and monitoring

include/
├── lv_conf.h                   - LVGL configuration (color depth, features, memory)
├── constants.h                 - Hardware constants, colors, timing intervals, and pin definitions
└── [component headers...]      - Header files for application modules

boards/                         - Git submodule with JSON hardware definitions
find_coin_id.py                 - Utility to discover CoinMarketCap cryptocurrency IDs
platformio.ini                  - Build configuration and board selection
```

### LVGL Configuration Highlights

- **Color Depth**: 16-bit RGB565 for optimal ESP32 performance
- **Memory**: 32KB heap allocation for graphics operations
- **Refresh Rate**: 33ms display refresh (30 FPS)
- **Features Enabled**: All major LVGL widgets, themes, and layouts
- **Performance Monitoring**: CPU/memory usage displays available

### Configuration Architecture

#### Hardware Constants (`include/constants.h`)

- **Color Definitions**: UI theme colors (dark background, green/red indicators, text colors)
- **Hardware Pins**: LED pins (RGB), light sensor pin, other GPIO assignments
- **Timing Intervals**: WiFi update, crypto update, brightness update intervals
- **Thresholds**: Data staleness detection, reconnection timeouts

#### User Configuration (Web Interface via NetworkManager)

- **API Settings**: CoinMarketCap API key and selected cryptocurrency IDs
- **Network Settings**: WiFi credentials and connection parameters
- **Storage**: Settings saved to ESP32 flash memory using Preferences library

### Hardware Features Available

- **Display**: 240x320 pixel TFT with backlight control (GPIO 21)
- **Touch Input**: Resistive touchscreen with calibration support
- **RGB LED**: 3-color LED for status indication (GPIOs 4, 16, 17)
- **Audio**: Speaker output on GPIO 26
- **Storage**: MicroSD card slot support
- **Sensors**: Light sensor (CdS) on ADC channel
- **Connectivity**: WiFi and Bluetooth via ESP32

## Development Notes

### Board Selection

- Default environment targets `esp32-2432S028R` board
- To use different CYD variant, modify `[env:esp32-cyd]` board setting in `platformio.ini`
- All supported boards listed in `boards/README.md` with detailed specifications

### Memory Considerations

- ESP32 has ~315KB usable RAM after boot
- LVGL uses 32KB for graphics operations
- Consider PSRAM boards for complex UIs (800x480+ displays require it)

### Display Orientation

- Default is portrait mode (240x320)
- Uncomment rotation lines in `main.cpp` for landscape modes
- Touch coordinates automatically adjust with rotation

### Debugging

- Serial output at 115200 baud with ESP32 exception decoder
- LVGL performance monitors can be enabled in `lv_conf.h`
- Hardware RGB LED available for status indication

## Cryptocurrency ID Discovery

Use the included `find_coin_id.py` utility to discover CoinMarketCap IDs for cryptocurrencies:

```bash
# Find a specific cryptocurrency
python find_coin_id.py --api-key YOUR_API_KEY ETH

# Find multiple cryptocurrencies
python find_coin_id.py --api-key YOUR_API_KEY BTC SOL ADA DOGE

# List top 20 cryptocurrencies by market cap
python find_coin_id.py --api-key YOUR_API_KEY --list-top 20
```

The utility outputs coin IDs that can be entered in the web configuration interface.

## Important Development Guidelines

**DO NOT MODIFY**: Never modify, read, or interact with the `.pio/libdeps/` directory or any files within it. This directory contains compiled library dependencies managed by PlatformIO and should not be touched by Claude Code.

**CONFIGURATION**:

- Configuration is managed through the built-in web interface (handled by NetworkManager)
- Settings are stored securely in ESP32 flash memory
- Web configuration portal is accessible on first boot or when device can't connect to WiFi
- Use `find_coin_id.py` to discover cryptocurrency IDs for web configuration
