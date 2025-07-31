# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a **real-time cryptocurrency price display application** for ESP32-based "Cheap Yellow Display" (CYD) boards from Sunton. The project uses PlatformIO with the Arduino framework and LVGL graphics library to create a modern touchscreen interface for monitoring cryptocurrency prices with live updates from Binance WebSocket API.

### Key Features

- **Real-time WebSocket Integration**: Live cryptocurrency price updates from Binance WebSocket API (no API key required)
- **Connection Management**: Automatic WebSocket reconnection with exponential backoff and visual status indicators
- **Modern LVGL Interface**: Touchscreen GUI with color-coded trend indicators and muted colors for disconnected state
- **Multi-currency Support**: Up to 6 simultaneous cryptocurrency pairs with automatic uppercase conversion
- **RGB LED Status Indicators**: Visual connection status (red=disconnected, yellow=reconnecting, green=connected)
- **Hardware Integration**: Automatic brightness control and BOOT button configuration reset
- **Standardized Logging**: Configurable log levels (TRACE, DEBUG, INFO, WARN, ERROR, FATAL) with serial timeout protection
- **Board Compatibility**: Works with 40+ different Sunton ESP32 display board variants
- **Power Optimization**: Memory and size optimizations for battery operation
- **Fixed Layout**: Clean, scrollbar-free interface perfectly sized for 320px screen height

## Build and Development Commands

### Prerequisites

- **PlatformIO** must be installed (`python -m platformio` or `pio` command)
- **No API Key Required** - Uses free Binance WebSocket API for real-time data
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

The project uses a built-in web interface for configuration:

- **Initial Setup**: Device creates WiFi access point for first-time configuration
- **WiFi Credentials**: Configure through captive portal web interface
- **Cryptocurrency Selection**: Choose up to 6 Binance trading pairs (automatically converted to uppercase)
- **Persistent Storage**: Settings saved to ESP32 flash memory
- **Factory Reset**: Hold BOOT button for 10+ seconds to clear all configuration
- **Reconfiguration**: Hold BOOT button for 5-9 seconds to restart configuration mode

#### Web Interface Features

- **Automatic Uppercase Conversion**: Cryptocurrency symbols are automatically converted to uppercase as you type
- **Real-time Validation**: Input validation for Binance USDT trading pairs
- **WiFi Network Scanning**: Automatic detection and display of available WiFi networks
- **Configuration Persistence**: Settings stored securely in ESP32 flash memory

#### Connection Status Indicators

**LED Status Patterns:**

- **Disconnected**: Continuous red blinking (500ms on/off)
- **Reconnecting**: Continuous yellow blinking (300ms on/off)
- **Connected**: 3 rapid green blinks (100ms on/off), then resume normal operation
- **Normal Operation**: Standard brightness-based LED behavior

**Display Visual Feedback:**

- **Connected**: Full brightness colors for prices and change indicators
- **Disconnected**: Muted colors for all price elements to indicate stale data
- Automatic color switching based on WebSocket connection status

### Development Workflow

1. **Board Configuration**: Uses `esp32-2432S028R` board definition (configured as `esp32-cyd` environment)
2. **Serial Communication**: 115200 baud with timeout protection to prevent display freezing
3. **Debugging**: ESP32 exception decoder enabled with standardized logging levels
4. **Configuration**: Web interface handles all user settings
5. **Symbol Discovery**: Use included Python utilities to find valid Binance trading pairs
6. **Real-time Updates**: WebSocket connection provides live price data without polling

### Display Layout

The application displays exactly 6 cryptocurrency pairs in a clean, fixed-height layout optimized for the 320px screen:

#### Each Cryptocurrency Item Shows:

**Left Side:**

- **Base Symbol** (top-left): Main cryptocurrency (e.g., "BTC", "ETH") in grey text
- **Quote Symbol** (bottom-left): Quote currency (e.g., "USDT") in muted grey text

**Center:**

- **Current Price**: Large, prominent white text with comma separators (e.g., "116,983.99")

**Right Side:**

- **24h Price Change** (top-right): Raw dollar amount change (e.g., "180.00", "-1,250.00")
  - Green for positive changes, red for negative changes
  - Formatted with comma separators for large amounts
  - Shows minus sign for negative values, no sign for positive values
- **24h Percentage Change** (bottom-right): Percentage change (e.g., "0.15%", "-2.34%")
  - Green for positive changes, red for negative changes
  - Formatted to 2 decimal places
  - Shows minus sign for negative values, no sign for positive values

#### Container Layout:

- **First 2 items**: 54px height each
- **Last 4 items**: 53px height each
- **Total height**: 320px (perfectly fills screen)
- **No scrolling**: Fixed display for exactly 6 items
- **No separators**: Clean, continuous display
- **Background colors**: Green tint for positive 24h change, red tint for negative 24h change

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
├── ApplicationController.cpp   - Main application lifecycle and WebSocket integration
├── BinanceDataManager.cpp      - Real-time cryptocurrency data management
├── WebSocketManager.cpp        - Binance WebSocket connection and reconnection logic
├── NetworkManager.cpp          - WiFi connection and web configuration interface
├── DisplayManager.cpp          - LVGL-based UI with connection status indicators
├── HardwareController.cpp      - RGB LED patterns and sensor hardware control
├── ApplicationStateManager.cpp - Application state management
└── StatusCalculator.cpp        - Cryptocurrency performance calculations

include/
├── lv_conf.h                   - LVGL configuration optimized for ESP32
├── constants.h                 - Hardware constants, colors, logging configuration
└── [component headers...]      - Header files for all application modules

boards/                         - Git submodule with JSON hardware definitions for 40+ boards
find_binance_symbols.py         - Utility to discover valid Binance trading pairs
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

- **Color Definitions**: UI theme colors including muted colors for disconnected state
- **Hardware Pins**: RGB LED pins, light sensor pin, BOOT button assignments
- **Connection Timeouts**: WebSocket reconnection intervals, WiFi timeout values
- **Logging Configuration**: Standardized log levels with serial timeout protection

#### WebSocket Configuration

- **Reconnection Logic**: Exponential backoff from 5 seconds to 60 seconds maximum
- **Connection Monitoring**: Heartbeat detection with 60-second stale connection timeout
- **Symbol Management**: Automatic uppercase conversion and validation of trading pairs

#### User Configuration (Web Interface via NetworkManager)

- **Binance Symbols**: Select up to 6 cryptocurrency trading pairs (no API key required)
- **Network Settings**: WiFi credentials with automatic connection management
- **Persistent Storage**: All settings saved to ESP32 flash memory using Preferences library
- **Reset Options**: Configuration reset (5s button) and factory reset (10s button)

### Hardware Features

- **Display**: 240x320 pixel TFT with automatic brightness control
- **Touch Input**: Resistive touchscreen (currently unused, WebSocket provides real-time updates)
- **RGB LED**: Connection status indicators with specific patterns:
  - Red blinking (500ms): WebSocket disconnected
  - Yellow blinking (300ms): WebSocket reconnecting
  - Green 3x blinks (100ms): WebSocket connected
  - Normal operation: Green/red based on cryptocurrency performance
- **Light Sensor**: Automatic backlight adjustment based on ambient light
- **BOOT Button**: Configuration management (5s=reconfigure, 10s=factory reset)
- **Audio**: Speaker output available (GPIO 26)
- **Storage**: MicroSD card slot support
- **Connectivity**: WiFi with automatic reconnection management

## Development Notes

### Board Selection

- Default environment targets `esp32-2432S028R` board
- To use different CYD variant, modify `[env:esp32-cyd]` board setting in `platformio.ini`
- All supported boards listed in `boards/README.md` with detailed specifications

### Memory Considerations

- ESP32 has ~315KB usable RAM after boot (24.6% usage with current implementation)
- LVGL uses 32KB for graphics operations
- WebSocket connection adds minimal memory overhead
- Real-time updates eliminate need for data caching, reducing memory usage

### Real-time Architecture Benefits

- **No Polling**: WebSocket eliminates periodic API requests
- **Instant Updates**: Price changes appear immediately on display
- **Power Efficient**: Reduces WiFi activity and CPU usage
- **Connection Resilient**: Automatic reconnection handles network interruptions
- **Visual Feedback**: Always know connection status through LED and display

### Debugging and Logging

- **Standardized Logging**: 6 log levels (TRACE, DEBUG, INFO, WARN, ERROR, FATAL)
- **Serial Protection**: Timeout protection prevents display freezing when monitor disconnected
- **Connection Debugging**: Detailed WebSocket connection status and reconnection attempts
- **LED Status**: Hardware visual debugging through RGB LED patterns
- **Log Level Control**: Adjust `CURRENT_LOG_LEVEL` in `constants.h` to control verbosity

#### Logging System

The project includes a comprehensive logging system with multiple log levels:

**Log Levels (in order of severity):**

- `LOG_TRACE(x)` / `LOG_TRACEF(format, ...)` - Detailed execution flow, raw data
- `LOG_DEBUG(x)` / `LOG_DEBUGF(format, ...)` - Function entry/exit, data processing
- `LOG_INFO(x)` / `LOG_INFOF(format, ...)` - Connection status, major state changes
- `LOG_WARN(x)` / `LOG_WARNF(format, ...)` - Retry attempts, temporary failures
- `LOG_ERROR(x)` / `LOG_ERRORF(format, ...)` - Connection failures, API errors
- `LOG_FATAL(x)` / `LOG_FATALF(format, ...)` - System crashes, unrecoverable errors

**Configuration:**

- `CURRENT_LOG_LEVEL` in `constants.h` controls verbosity (default: `LOG_LEVEL_INFO`)
- `ENABLE_LOGGING` flag can disable all log output
- 10ms timeout prevents indefinite blocking when no monitor is connected
- All logging is non-blocking and safe to use without serial monitor

**Legacy Support:**

- `SAFE_SERIAL_*` macros are preserved for backward compatibility (map to `LOG_DEBUG`)

## Binance Symbol Discovery

Use the included `find_binance_symbols.py` utility to discover valid Binance trading pairs:

```bash
# Search for specific trading pairs
python find_binance_symbols.py BTC ETH SOL

# List all USDT pairs
python find_binance_symbols.py --quote USDT

# Find pairs with specific base currency
python find_binance_symbols.py --base BTC
```

The utility outputs valid trading pair symbols that can be entered in the web configuration interface. Common pairs include:

- Major coins: BTCUSDT, ETHUSDT, SOLUSDT, ADAUSDT
- Popular altcoins: DOGEUSDT, MATICUSDT, LINKUSDT
- All symbols are automatically converted to uppercase by the device

## Important Development Guidelines

**DO NOT MODIFY**: Never modify, read, or interact with the `.pio/libdeps/` directory or any files within it. This directory contains compiled library dependencies managed by PlatformIO and should not be touched by Claude Code.

**REAL-TIME ARCHITECTURE**:

- **WebSocket Integration**: Direct connection to Binance WebSocket API for live price feeds
- **No API Keys Required**: Uses free Binance WebSocket streams
- **Automatic Reconnection**: Exponential backoff strategy handles network interruptions
- **Connection Status**: RGB LED indicators and muted display colors show connection state
- **Configuration**: Web interface for WiFi and cryptocurrency pair selection
- **Symbol Validation**: Use `find_binance_symbols.py` to discover valid trading pairs

**LOGGING SYSTEM**:

- **Standardized Levels**: TRACE, DEBUG, INFO, WARN, ERROR, FATAL with configurable output
- **Serial Safety**: Timeout protection prevents display freezing when monitor disconnected
- **Backward Compatibility**: `SAFE_SERIAL_*` macros preserved (map to `LOG_DEBUG`)
- **Development**: Adjust `CURRENT_LOG_LEVEL` in `constants.h` for debugging verbosity

**ARCHITECTURE PRINCIPLES**:

- **Real-time First**: WebSocket integration provides instant price updates
- **Connection Resilience**: Automatic reconnection with visual status feedback
- **User Experience**: Clear status indicators and muted colors for disconnected state
- **Power Efficiency**: No polling, optimized memory usage, adaptive brightness
- **Hardware Integration**: RGB LED patterns, BOOT button configuration management
- **Robust Logging**: Standardized log levels with serial timeout protection

**DEVELOPMENT GUIDELINES**:

Do what has been asked; nothing more, nothing less.
NEVER create files unless they're absolutely necessary for achieving your goal.
ALWAYS prefer editing an existing file to creating a new one.
NEVER proactively create documentation files (\*.md) or README files. Only create documentation files if explicitly requested by the User but update them automatically when they're there. Always write and keep up-to-date all doxygen comments.
