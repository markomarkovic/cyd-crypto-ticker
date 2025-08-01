# 🪙 CYD Crypto Ticker

A real-time cryptocurrency price display application for ESP32-based "Cheap Yellow Display" (CYD) boards from Sunton. Features a modern touchscreen interface built with LVGL to monitor your favorite cryptocurrencies with live price updates, trend indicators, and customizable display options.

## ✨ Features

- **Advanced Candlestick Charts**: Interactive OHLCV charts with real-time data from Binance API
- **Dynamic Interval Selection**: Touch-enabled interval selector with 14 timeframes (1m to 1M) and automatic refresh
- **Technical Analysis**: 7-period moving averages with smooth line rendering
- **Interactive Price Inspection**: Click-to-show-price with blue indicator line and precise price display
- **Real-time Crypto Prices**: Live price updates from Binance WebSocket API (no API key required)
- **Multi-Currency Support**: Display multiple cryptocurrencies simultaneously
- **Touch Interface**: Modern LVGL-based GUI with touch controls
- **Visual Indicators**: Color-coded price trends and status indicators
- **Hardware Integration**: RGB LED status indicators and automatic brightness adjustment
- **WiFi Connectivity**: Automatic connection management with retry logic
- **Modern Web Interface**: PicoCSS v2-styled configuration portal with responsive design
- **Multi-Board Support**: Compatible with 40+ different Sunton ESP32 display variants
- **Memory Efficient**: Optimized for ESP32's limited RAM with 25% usage (82KB/327KB)

## 🛠️ Hardware Requirements

### Supported Boards

This project should supports all Sunton ESP32 display boards, you can check the [boards documentation](https://github.com/rzeldent/platformio-espressif32-sunton/blob/main/README.md).

- **Primary Target**: ESP32-2432S028R (CYD - Cheap Yellow Display)
- **Display**: 240x320 ILI9341 TFT with SPI interface
- **Touch**: XPT2046 resistive touchscreen controller
- **Additional Hardware**: RGB LED, CdS light sensor, speaker output, TF card slot

## 📦 Installation

### Prerequisites

1. **PlatformIO**: Install PlatformIO Core or IDE

   ```bash
   # Via pip
   pip install platformio

   # Or install PlatformIO IDE for VS Code
   ```

2. **No API Key Required**: Uses free Binance WebSocket API for real-time cryptocurrency data

### Setup Instructions

1. **Clone the repository**:

   ```bash
   git clone https://github.com/your-username/cyd-crypto-ticker.git
   cd cyd-crypto-ticker
   ```

2. **Initialize submodules** (for board definitions):

   ```bash
   git submodule update --init --recursive
   ```

3. **Configure the application**:

   The application provides a web-based configuration interface. After first boot, connect to the device's WiFi configuration portal to set up your WiFi credentials and select up to 6 Binance USDT trading pairs through an HTML interface.

4. **Build and upload**:

   ```bash
   # Build the project
   platformio run

   # Upload to your device
   platformio run -t upload

   # Monitor serial output
   platformio run -t monitor
   ```

## 🚀 Usage

### Finding Cryptocurrency IDs

Use the included Python utility to discover available Binance trading pairs:

```bash
# Find specific trading pairs
python find_binance_symbols.py BTC ETH SOL

# List top 20 pairs by trading volume
python find_binance_symbols.py --list-top 20

# Search by cryptocurrency name
python find_binance_symbols.py --search bitcoin

# Show all USDT pairs
python find_binance_symbols.py --usdt-pairs
```

The utility will show available trading pairs that you can enter in the web configuration interface.

### Board Configuration

To use a different CYD board variant, modify the `board` setting in `platformio.ini`:

```ini
[env:esp32-cyd]
board = esp32-2432S028R  ; Change this to your board model
```

Available board models are listed in the `boards/` directory.

### Display Orientation

The default orientation is portrait (240x320). To change to landscape mode, modify the rotation settings in your display configuration.

#### Web Configuration Interface

The application provides a built-in web interface for configuration:

1. **First Boot**: Device creates a WiFi access point for initial setup
2. **Configuration Portal**: Access the HTML configuration interface
3. **Settings**: Configure WiFi credentials and select up to 6 Binance USDT trading pairs
4. **Save & Restart**: Settings are saved and the device connects to your WiFi network

#### Button Controls

The BOOT button provides several configuration options:

- **Short Press (< 5 seconds)**: Cancel configuration mode and return to normal operation
- **Medium Press (5-9 seconds)**: Enter configuration mode to change WiFi/crypto settings
- **Long Press (10+ seconds)**: Factory reset - clear all stored data

## 🔧 Development

### Project Structure

```
├── src/                           # Main application source code
│   ├── main.cpp                   # Application entry point
│   ├── ApplicationController.cpp  # Main app controller
│   ├── BinanceDataManager.cpp     # Cryptocurrency data handling
│   ├── NetworkManager.cpp         # WiFi and web config management
│   ├── DisplayManager.cpp         # LVGL UI management
│   └── HardwareController.cpp     # Hardware control and sensors
├── include/                       # Header files
│   ├── lv_conf.h                  # LVGL configuration
│   └── constants.h                # Hardware constants, colors, and timing intervals
├── picocss/                       # PicoCSS v2 build system for web interface
│   ├── scss/minimal.scss          # Source SCSS with minimal PicoCSS build
│   ├── package.json               # Node.js build tools and dependencies
│   └── update-cpp.js              # Automated CSS embedding script
├── boards/                        # Hardware board definitions (submodule)
├── find_binance_symbols.py        # Utility to find Binance trading pairs
└── platformio.ini                 # Build configuration
```

### Key Components

- **ApplicationController**: Main application lifecycle management and WebSocket coordination
- **BinanceDataManager**: Real-time cryptocurrency data management and chart data fetching
- **NetworkManager**: WiFi connection, web configuration interface, and reconnection logic
- **DisplayManager**: LVGL-based user interface and chart rendering using constants from constants.h
- **HardwareController**: RGB LED status indicators and sensor management using hardware pin constants
- **WebSocketManager**: Real-time WebSocket connections to Binance API with automatic reconnection

### Configuration Architecture

- **Hardware Constants**: Pin definitions, colors, timing intervals, and system limits defined in `include/constants.h`
- **User Configuration**: WiFi credentials and cryptocurrency selections managed through web interface
- **Settings Storage**: User configurations stored in ESP32 flash memory via NetworkManager
- **Unified Constants**: All system limits (MAX_COINS, MAX_CANDLESTICKS) centralized in constants.h for consistency

### Build Commands

```bash
# Clean build
platformio run -t clean

# Build for specific environment
platformio run -e esp32-cyd

# Upload and monitor
platformio run -t upload -t monitor

# Update dependencies
platformio lib update
```

### Memory Optimization

The project is optimized for ESP32's limited memory:

- Size optimization enabled (`-Os`)
- Function/data sections (`-ffunction-sections`, `-fdata-sections`)
- Unused section removal (`-Wl,--gc-sections`)
- Disabled C++ exceptions and RTTI
- 32KB LVGL memory pool
- Reduced logging levels

### LVGL Configuration

Key LVGL settings in `include/lv_conf.h`:

- 16-bit RGB565 color depth
- 32KB heap allocation
- 33ms refresh rate (30 FPS)
- All major widgets enabled
- Performance monitoring available

## 🤝 Contributing

We welcome contributions! Please follow these guidelines:

### Development Setup

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Follow the existing code style and conventions
4. Test your changes on actual hardware when possible
5. Update documentation as needed

### Code Style

- Follow existing C++ conventions in the codebase
- Use meaningful variable and function names
- Add comments for complex logic
- Keep functions focused and modular
- Use `const` where appropriate

### Submitting Changes

1. Ensure your code builds without warnings
2. Test on actual hardware if possible
3. Update README.md if you add new features
4. Commit with clear, descriptive messages
5. Submit a pull request with detailed description

### Reporting Issues

Please include:

- Hardware board model
- PlatformIO version
- Complete error messages
- Steps to reproduce
- Expected vs actual behavior

## 🛡️ Security

### Configuration Security

- Cryptocurrency pairs are automatically converted to uppercase in the web interface
- Access the configuration portal only when needed
- Monitor API usage to avoid rate limits
- Use HTTPS for all API communications

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

### Third-Party Licenses

- **LVGL**: MIT License
- **esp32_smartdisplay**: MIT License
- **ArduinoJson**: MIT License
- **ESPAsyncWebServer**: LGPL-3.0 License
- **PicoCSS**: MIT License
- **Board Definitions**: Various licenses (see `boards/` directory)

## 🙏 Acknowledgments

- [Sunton](http://www.lcdwiki.com/) for the affordable ESP32 display boards
- [LVGL](https://lvgl.io/) for the excellent embedded graphics library
- [rzeldent](https://github.com/rzeldent) for the esp32_smartdisplay library
- [Binance](https://binance.com/) for the free WebSocket cryptocurrency API
- [PicoCSS](https://picocss.com/) for the minimal, semantic CSS framework
- [Claude Code](https://claude.ai/code) for significant contributions to the code and documentation
- The ESP32 and Arduino communities for excellent documentation and support

## 📞 Support

- **Documentation**: Check the `CLAUDE.md` file for detailed development notes
- **Issues**: Report bugs and feature requests on GitHub Issues
- **Discussions**: Join the community discussions for help and ideas
- **Hardware**: Refer to `boards/README.md` for board-specific information

---

**Happy crypto tracking!** 🚀📈
