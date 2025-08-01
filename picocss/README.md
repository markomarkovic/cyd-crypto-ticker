# PicoCSS Build System for CYD Crypto Ticker

This directory contains the PicoCSS v2 build system for the CYD Crypto Ticker captive portal interface.

## Overview

The system builds a minimal PicoCSS v2 CSS file and automatically updates the embedded CSS in the C++ NetworkManager code.

## File Structure

```
picocss/
├── package.json          # Build dependencies and scripts
├── scss/minimal.scss     # Source SASS file (edit this)
├── css/minimal.css       # Generated expanded CSS
├── css/minimal.min.css   # Generated minified CSS
├── update-cpp.js         # Script to update NetworkManager.cpp
└── README.md            # This file
```

## Build Process

### 1. Edit SASS Source
Edit `scss/minimal.scss` to modify the CSS styles.

### 2. Build and Update
```bash
npm run build
```

This command:
1. Compiles SASS → CSS
2. Minifies CSS 
3. Automatically updates `../src/NetworkManager.cpp` with the new CSS

### 3. Manual Update (if needed)
```bash
npm run update-cpp
```

Updates NetworkManager.cpp with the current CSS without rebuilding.

## Available Scripts

- `npm run build` - Full build + update C++ file
- `npm run build-dev` - Development build (no minification)
- `npm run watch` - Watch SASS files for changes
- `npm run update-cpp` - Update C++ file only

## How It Works

The `update-cpp.js` script:
1. Reads `css/minimal.min.css`
2. Finds the `<style>...</style>` section in `NetworkManager.cpp`
3. Replaces the CSS content between the style tags
4. Saves the updated C++ file

## CSS Integration

The CSS is embedded in `src/NetworkManager.cpp` at line ~217:

```cpp
<style>/* CSS content goes here */</style>
```

The script automatically replaces everything between `<style>` and `</style>` tags.

## Development Workflow

1. Edit `scss/minimal.scss`
2. Run `npm run build` 
3. Compile the ESP32 project with PlatformIO
4. The updated CSS will be included in the captive portal

## File Sizes

- Original PicoCSS v2: ~71KB
- Custom build: ~6.3KB minified
- Perfect for ESP32 constraints