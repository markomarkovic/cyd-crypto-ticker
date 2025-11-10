# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a **real-time cryptocurrency price display application** for ESP32-based "Cheap Yellow Display" (CYD) boards from Sunton. The project uses PlatformIO with the Arduino framework and LVGL graphics library to create a modern touchscreen interface for monitoring cryptocurrency prices with live updates from Binance WebSocket API.

### Key Features

- **Advanced Candlestick Charts**: Interactive OHLCV charts with real-time data from Binance API
- **Dynamic Interval Selection**: Touch-enabled interval selector with 14 timeframes (1m to 1M) and automatic refresh
- **Technical Analysis**: 7-period moving averages with smooth line rendering
- **Interactive Price Inspection**: Click-to-show-price with blue indicator line and precise price display
- **Real-time WebSocket Integration**: Live cryptocurrency price updates from Binance WebSocket API (no API key required)
- **Dual Data Sources**: WebSocket for real-time prices + REST API for historical candlestick data
- **Connection Management**: Automatic WebSocket reconnection with exponential backoff and visual status indicators
- **Professional Chart Features**: Highest/lowest point indicators, optimal screen space utilization (~31 candles)
- **Modern LVGL Interface**: Touchscreen GUI with color-coded trend indicators and muted colors for disconnected state
- **Multi-currency Support**: Up to 6 simultaneous cryptocurrency pairs with automatic uppercase conversion
- **RGB LED Status Indicators**: Visual connection status (red=disconnected, yellow=reconnecting, green=connected)
- **Hardware Integration**: Automatic brightness control and BOOT button configuration reset
- **Standardized Logging**: Configurable log levels (TRACE, DEBUG, INFO, WARN, ERROR, FATAL) with serial timeout protection
- **Board Compatibility**: Works with 40+ different Sunton ESP32 display board variants
- **Memory-Safe Implementation**: Proper LVGL object lifecycle management preventing crashes
- **Fixed Layout**: Clean, scrollbar-free interface perfectly sized for 320px screen height
- **Modern Web Interface**: PicoCSS v2-based configuration portal with responsive design and automatic dark/light theme detection

## Build and Development Commands

### Prerequisites

- **uv** must be installed for running PlatformIO commands
- **No API Key Required** - Uses free Binance WebSocket API for real-time data
- Project uses ESP32 Arduino framework
- Configuration is handled through built-in HTML web interface

### Makefile Commands (Recommended)

The project includes a Makefile to simplify common development tasks:

```bash
# Show all available commands
make help

# Build the project
make build

# Upload firmware to device
make upload

# Monitor serial output
make monitor

# Upload and monitor (most common)
make upload-monitor

# Clean build files
make clean

# Update library dependencies
make update-deps

# Complete workflow: clean, build, upload, and monitor
make all

# Build PicoCSS styles for web configuration interface
make web-build

# Update embedded CSS in NetworkManager.cpp
make web-update

# Test Binance symbol discovery
make test-symbols

# Generate Doxygen documentation
make docs

# Clean generated documentation
make docs-clean

# Generate and open documentation in browser
make docs-open
```

### Direct PlatformIO Commands (using uv)

If you prefer to use PlatformIO commands directly:

```bash
# Build the project
uv tool run platformio run

# Build for specific environment (default: esp32-cyd)
uv tool run platformio run -e esp32-cyd

# Upload to device
uv tool run platformio run -t upload

# Upload and monitor serial output
uv tool run platformio run -t upload -t monitor

# Monitor serial output only
uv tool run platformio run -t monitor

# Clean build files
uv tool run platformio run -t clean

# Update dependencies
uv tool run platformio lib update
```

### Configuration

The project uses a built-in web interface for configuration:

- **Initial Setup**: Device creates WiFi access point for first-time configuration
- **WiFi Credentials**: Configure through captive portal web interface
- **Cryptocurrency Selection**: Choose up to 6 Binance trading pairs (automatically converted to uppercase)
- **Persistent Storage**: Settings saved to ESP32 flash memory
- **Factory Reset**: Hold BOOT button for 10+ seconds to clear all configuration
- **Reconfiguration**: Hold BOOT button for 5-9 seconds to restart configuration mode
- **Cancel Configuration**: Short press (< 5 seconds) BOOT button during configuration mode to cancel and return to normal operation

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
- **Normal Operation**: Solid green when majority of coins are up, solid red when majority are down, off when balanced

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

### Interactive Candlestick Chart View

Touching any cryptocurrency in the list view switches to a detailed candlestick chart with the following features:

#### Chart Display Elements:

**Coin Information Header (54px height):**

- Same layout as list view items with real-time WebSocket updates

**Candlestick Chart Area (266px height):**

- **Chart Data**: Up to ~31 visible candlesticks optimally spaced for 240px width
- **Dynamic Time Intervals**: Configurable timeframes from 1m to 1M with automatic refresh
- **OHLCV Display**: Open/High/Low/Close prices with volume data
- **Candle Styling**: Rectangular bodies (no rounded corners) with matching wick colors
- **Moving Average**: 7-period smooth moving average line overlay
- **Price Range**: Automatic scaling with 5% padding for optimal visibility

**Chart Labels & Indicators:**

- **Price Labels**: Min/max price labels in top/bottom-right corners (14pt font)
- **Interval Selector**: Dark transparent button in bottom-left corner for timeframe selection
- **Timestamp**: Oldest visible candle date in bottom-left corner
- **Highest/Lowest Points**: White lines extending from extreme values to screen edges
- **Extended View**: 1-2 additional candles past left edge for historical context

**Interactive Features:**

- **Click-to-Show-Price**: Touch anywhere on chart to display price at clicked height
- **Price Indicator**: Blue horizontal line (50px) at exact click coordinates
- **Price Display**: Calculated price shown in top-left corner with dark background
- **Interval Selection**: Touch bottom-left button to access timeframe selector overlay
- **Real-time Updates**: Coin info updates live via WebSocket while viewing chart
- **Return Navigation**: Touch coin info area to return to main list view

**Interval Selection System:**

- **Access Method**: Touch dark transparent button (35x28px) in absolute bottom-left corner
- **Available Intervals**: 14 timeframes - 1m, 3m, 5m, 15m, 30m, 1h, 2h, 4h, 6h, 8h, 12h, 1d, 1w, 1M
- **Overlay Interface**: 3x5 grid of dark transparent buttons with subtle white borders
- **Instant Response**: Buttons hide immediately when selected, data refreshes automatically
- **Automatic Refresh**: Charts update at selected interval rate (1m = 1 minute, 1h = 1 hour, etc.)
- **Persistent Selection**: Chosen interval persists when switching between different coin charts
- **Silent Updates**: Background refresh without loading indicators after initial chart load

#### Technical Implementation:

- **Data Source**: Binance REST API for historical candlestick data (40 candles fetched)
- **Memory Optimization**: Efficient LVGL object management preventing crashes
- **Error Handling**: Automatic fallback to test data if API calls fail
- **Performance**: Sub-3-second chart load times with proper caching

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

picocss/                        - PicoCSS v2 build system for web interface styling
├── scss/minimal.scss           - Source SCSS with minimal PicoCSS v2 build
├── package.json                - Node.js build tools (SASS, cssnano) and dependencies  
└── update-cpp.js               - Automated CSS embedding script for NetworkManager

boards/                         - Git submodule with JSON hardware definitions for 40+ boards
find_binance_symbols.py         - Utility to discover valid Binance trading pairs
platformio.ini                  - Build configuration and board selection
```

### LVGL Configuration Highlights

- **Color Depth**: 16-bit RGB565 for optimal ESP32 performance
- **Memory**: 32KB heap allocation for graphics operations
- **Refresh Rate**: 33ms display refresh (30 FPS)
- **Optimized Configuration**: Pruned LVGL build with only essential features enabled
- **Memory Efficient**: Disabled unused widgets, themes, GPU acceleration, and advanced layouts
- **Flash Savings**: ~11KB reduction from standard LVGL configuration

**Note**: The LVGL configuration (`include/lv_conf.h`) has been optimized for this specific cryptocurrency ticker application. Many widgets, themes, and advanced features have been disabled to reduce memory usage and flash size. If you need additional LVGL features for modifications, you can re-enable them in `lv_conf.h` by changing the corresponding `#define` from `0` to `1`.

### PicoCSS Web Interface Integration

The project includes a minimal PicoCSS v2 build system for the captive portal configuration interface, providing modern styling with minimal overhead suitable for ESP32 constraints.

#### PicoCSS Build System (`picocss/`)

- **Source File**: `scss/minimal.scss` - Custom minimal build with only essential PicoCSS v2 components
- **Size Optimization**: 7.4KB minified CSS (vs 71KB full PicoCSS) with only button/input elements
- **Automatic Build**: `npm run build` compiles SCSS → CSS → minified → embedded in NetworkManager.cpp
- **Theme Support**: Automatic dark/light theme detection based on user's system preferences
- **Responsive Design**: Mobile-optimized layout with full-width buttons and proper spacing

#### Key Features

- **Semantic HTML**: Uses classless semantic approach (no CSS classes needed in HTML)
- **Form Styling**: Professional button and input styling with hover/focus states
- **Password Toggle**: Custom eye/no-eye password visibility toggle with text-based icons
- **Validation Styling**: Error messages with proper color coding and spacing
- **Network Selection**: Styled WiFi network selection with hover effects
- **Memory Efficient**: Embedded directly in C++ avoiding external file dependencies

#### Build Commands

```bash
# Navigate to PicoCSS directory
cd picocss/

# Install dependencies
npm install

# Build CSS and update NetworkManager.cpp
npm run build

# Individual build steps
sass scss/minimal.scss css/minimal.css --style=expanded --no-source-map
cssnano css/minimal.css css/minimal.min.css
node update-cpp.js
```

#### Customization

- **Colors**: Modify PicoCSS color variables in `scss/minimal.scss`
- **Components**: Add additional PicoCSS components by importing them in the SCSS file
- **Automation**: The `update-cpp.js` script automatically embeds new CSS builds into NetworkManager.cpp
- **Size Monitoring**: Build output shows final CSS size for ESP32 memory planning

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
- **Touch Input**: Resistive touchscreen for chart interaction and navigation
- **RGB LED**: Status indicators with specific patterns:
  - Connection status: Red blinking (disconnected), yellow blinking (reconnecting), green 3x blinks (connected)
  - Market status: Solid green (majority coins up), solid red (majority coins down), off (balanced)
- **Light Sensor**: Automatic display brightness adjustment based on ambient light (separate from LED status)
- **BOOT Button**: Configuration management (5s=reconfigure, 10s=factory reset)
- **Audio**: Speaker output available (GPIO 26)
- **Storage**: MicroSD card slot support
- **Connectivity**: WiFi with automatic reconnection management

## Documentation

### Automatic GitHub Pages Deployment

The project includes automatic documentation generation and deployment using GitHub Actions:

- **Workflow**: `.github/workflows/deploy-docs.yml`
- **Trigger**: Automatically runs on every push to the `main` branch
- **Documentation Tool**: Doxygen with custom configuration
- **Output**: HTML documentation deployed to GitHub Pages

#### Setting Up GitHub Pages

To enable GitHub Pages for your repository:

1. Go to your GitHub repository settings
2. Navigate to **Settings** → **Pages**
3. Under **Build and deployment**:
   - **Source**: Select "GitHub Actions"
4. Push a commit to the `main` branch to trigger the workflow
5. Once the workflow completes, your documentation will be available at:
   - `https://<username>.github.io/<repository-name>/`

#### Local Documentation Generation

You can generate documentation locally using the Makefile commands:

```bash
# Generate documentation
make docs

# Clean generated documentation
make docs-clean

# Generate and open in browser
make docs-open
```

Or use the documentation generation script directly:

```bash
# Generate documentation
./generate_docs.sh

# Clean only
./generate_docs.sh clean

# Generate and open in browser
./generate_docs.sh open
```

The generated documentation will be available in the `docs/html/` directory (gitignored).

## Development Notes

### Board Selection

- Default environment targets `esp32-2432S028R` board
- To use different CYD variant, modify `[env:esp32-cyd]` board setting in `platformio.ini`
- All supported boards listed in `boards/README.md` with detailed specifications

### Memory Considerations

- ESP32 has ~315KB usable RAM after boot (25.1% usage with current implementation including charts)
- LVGL uses 32KB for graphics operations plus additional memory for chart rendering
- WebSocket + REST API connections managed efficiently with proper cleanup
- Candlestick data cached in memory (max 50 candles per symbol)
- Interactive chart elements use memory-safe LVGL object lifecycle management

### Dual Data Source Architecture Benefits

**WebSocket Stream (Real-time Prices):**

- **No Polling**: WebSocket eliminates periodic API requests for live prices
- **Instant Updates**: Price changes appear immediately on display
- **Power Efficient**: Reduces WiFi activity and CPU usage for price updates
- **Connection Resilient**: Automatic reconnection handles network interruptions
- **Visual Feedback**: Always know connection status through LED and display

**REST API (Historical Data):**

- **Chart Data**: Binance Klines API provides OHLCV candlestick data
- **On-Demand Fetching**: Historical data loaded only when viewing chart details
- **Memory Efficient**: HTTPS requests with insecure mode to reduce RAM usage
- **Fallback Support**: Automatic fallback to test data if API calls fail
- **Optimized Requests**: Fetches exact number of candles needed for display

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

**DUAL DATA ARCHITECTURE**:

- **WebSocket Integration**: Direct connection to Binance WebSocket API for live price feeds
- **REST API Integration**: Binance Klines API for historical candlestick chart data
- **No API Keys Required**: Uses free Binance public APIs (both WebSocket and REST)
- **Interactive Charts**: Touch-enabled candlestick charts with technical analysis features
- **Automatic Reconnection**: Exponential backoff strategy handles network interruptions
- **Connection Status**: RGB LED indicators and muted display colors show connection state
- **Configuration**: Web interface for WiFi and cryptocurrency pair selection
- **Symbol Validation**: Use `find_binance_symbols.py` to discover valid trading pairs

**LOGGING SYSTEM**:

- **Standardized Levels**: TRACE, DEBUG, INFO, WARN, ERROR, FATAL with configurable output
- **Serial Safety**: Timeout protection prevents display freezing when monitor disconnected
- **Migration Complete**: All `SAFE_SERIAL_*` macros have been migrated to `LOG_*` system
- **Development**: Adjust `CURRENT_LOG_LEVEL` in `constants.h` for debugging verbosity
- **System Monitoring**: Set log level to DEBUG or lower to display system statistics (uptime, memory usage, connection status) every 30 seconds

**ARCHITECTURE PRINCIPLES**:

- **Dual Data Strategy**: WebSocket for real-time prices + REST API for historical charts
- **Interactive Design**: Touch-enabled candlestick charts with price inspection features
- **Connection Resilience**: Automatic reconnection with visual status feedback
- **User Experience**: Clear status indicators, muted colors for disconnected state, professional chart UI
- **Memory Safety**: Proper LVGL object lifecycle management preventing crashes
- **Power Efficiency**: Optimized API usage, memory management, adaptive brightness
- **Hardware Integration**: RGB LED patterns, BOOT button configuration management
- **Robust Logging**: Standardized log levels with serial timeout protection

**DEVELOPMENT GUIDELINES**:

Do what has been asked; nothing more, nothing less.
NEVER create files unless they're absolutely necessary for achieving your goal.
ALWAYS prefer editing an existing file to creating a new one.
NEVER proactively create documentation files (\*.md) or README files. Only create documentation files if explicitly requested by the User but update them automatically when they're there. Always write and keep up-to-date all doxygen comments.
