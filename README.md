# 🪙 CYD Crypto Ticker

A real-time cryptocurrency price display application for ESP32-based "Cheap Yellow Display" (CYD) boards from Sunton. Features a modern touchscreen interface built with LVGL to monitor your favorite cryptocurrencies with live price updates, trend indicators, and customizable display options.

## ✨ Features

- **Real-time Crypto Prices**: Live price updates from CoinMarketCap API
- **Multi-Currency Support**: Display multiple cryptocurrencies simultaneously
- **Touch Interface**: Modern LVGL-based GUI with touch controls
- **Visual Indicators**: Color-coded price trends and status indicators
- **Hardware Integration**: RGB LED status indicators and automatic brightness adjustment
- **WiFi Connectivity**: Automatic connection management with retry logic
- **Multi-Board Support**: Compatible with 40+ different Sunton ESP32 display variants
- **Power Efficient**: Optimized for battery operation with sleep modes

## 🛠️ Hardware Requirements

### Supported Boards

- **Primary Target**: ESP32-2432S028R (CYD - Cheap Yellow Display)
- **Display**: 240x320 ILI9341 TFT with SPI interface
- **Touch**: XPT2046 resistive touchscreen controller
- **Additional Hardware**: RGB LED, CdS light sensor, speaker output, TF card slot

### Complete Compatibility List

This project supports all Sunton ESP32 display boards including:

- ESP32-1732S019C/N (1.9" round display)
- ESP32-2424S012C/N (1.2" square display)
- ESP32-2432S022C/N (2.2" display)
- ESP32-2432S024C/N/R (2.4" display)
- ESP32-2432S028R/Rv2/Rv3 (2.8" CYD display)
- ESP32-2432S032C/N/R (3.2" display)
- ESP32-3248S035C/R (3.5" display)
- ESP32-4827S043C/N/R (4.3" display)
- ESP32-4848S040CIY1/CIY3 (4.0" square display)
- ESP32-8048S043C/N/R (4.3" high-res display)
- ESP32-8048S050C/N/R (5.0" display)
- ESP32-8048S070C/N/R (7.0" display)
- ESP32-8048S550C (5.5" display)
- ESP32-S3 Touch LCD variants

## 📦 Installation

### Prerequisites

1. **PlatformIO**: Install PlatformIO Core or IDE

   ```bash
   # Via pip
   pip install platformio

   # Or install PlatformIO IDE for VS Code
   ```

2. **CoinMarketCap API Key**: Get a free API key from [CoinMarketCap](https://coinmarketcap.com/api/)

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

   The application provides a web-based configuration interface. After first boot, connect to the device's WiFi configuration portal to set up your WiFi credentials and CoinMarketCap API settings through an HTML interface.

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

Use the included Python utility to find CoinMarketCap IDs for your favorite cryptocurrencies:

```bash
# Find a specific coin
python find_coin_id.py --api-key YOUR_API_KEY ETH

# Find multiple coins
python find_coin_id.py --api-key YOUR_API_KEY BTC SOL ADA DOGE

# List top 20 cryptocurrencies
python find_coin_id.py --api-key YOUR_API_KEY --list-top 20
```

The utility will output the coin IDs that you can enter in the web configuration interface.

### Board Configuration

To use a different CYD board variant, modify the `board` setting in `platformio.ini`:

```ini
[env:esp32-cyd]
board = esp32-2432S028R  ; Change this to your board model
```

Available board models are listed in the `boards/` directory.

### Display Orientation

The default orientation is portrait (240x320). To change to landscape mode, modify the rotation settings in your display configuration.

### Code Examples

#### Basic Setup

```cpp
#include <Arduino.h>
#include <lvgl.h>
#include "ApplicationController.h"

ApplicationController* app_controller;

void setup() {
    app_controller = new ApplicationController();
    app_controller->initialize();
}

void loop() {
    app_controller->update();
    lv_timer_handler();
}
```

#### Web Configuration Interface

The application provides a built-in web interface for configuration:

1. **First Boot**: Device creates a WiFi access point for initial setup
2. **Configuration Portal**: Access the HTML configuration interface
3. **Settings**: Configure WiFi credentials, CoinMarketCap API key, and coin IDs
4. **Save & Restart**: Settings are saved and the device connects to your WiFi network

## 🔧 Development

### Project Structure

```
├── src/                           # Main application source code
│   ├── main.cpp                   # Application entry point
│   ├── ApplicationController.cpp  # Main app controller
│   ├── CryptoDataManager.cpp      # API data handling
│   ├── NetworkManager.cpp         # WiFi and web config management
│   ├── DisplayManager.cpp         # LVGL UI management
│   └── HardwareController.cpp     # Hardware control and sensors
├── include/                       # Header files
│   ├── lv_conf.h                  # LVGL configuration
│   └── constants.h                # Hardware constants, colors, and timing intervals
├── boards/                        # Hardware board definitions (submodule)
├── find_coin_id.py                # Utility to find cryptocurrency IDs
└── platformio.ini                 # Build configuration
```

### Key Components

- **ApplicationController**: Main application lifecycle management
- **CryptoDataManager**: Handles API communication with CoinMarketCap
- **NetworkManager**: WiFi connection, web configuration interface, and reconnection logic
- **DisplayManager**: LVGL-based user interface management using constants from constants.h
- **HardwareController**: RGB LED and sensor management using hardware pin constants

### Configuration Architecture

- **Hardware Constants**: Pin definitions, colors, and timing intervals defined in `include/constants.h`
- **User Configuration**: API keys and cryptocurrency selections managed through web interface
- **Settings Storage**: User configurations stored in ESP32 flash memory via NetworkManager

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

- Keep your CoinMarketCap API key secure in the web configuration interface
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
- **Board Definitions**: Various licenses (see `boards/` directory)

## 🙏 Acknowledgments

- [Sunton](http://www.lcdwiki.com/) for the affordable ESP32 display boards
- [LVGL](https://lvgl.io/) for the excellent embedded graphics library
- [rzeldent](https://github.com/rzeldent) for the esp32_smartdisplay library
- [CoinMarketCap](https://coinmarketcap.com/) for the cryptocurrency API
- [Claude Code](https://claude.ai/code) for significant contributions to the code and documentation
- The ESP32 and Arduino communities for excellent documentation and support

## 📞 Support

- **Documentation**: Check the `CLAUDE.md` file for detailed development notes
- **Issues**: Report bugs and feature requests on GitHub Issues
- **Discussions**: Join the community discussions for help and ideas
- **Hardware**: Refer to `boards/README.md` for board-specific information

---

**Happy crypto tracking!** 🚀📈
