# Screenshot Capture Guide

This guide explains how to capture screenshots from the CYD Crypto Ticker device.

## Prerequisites

- Device flashed with the [`screenshots`](../../tree/screenshots) branch
- Serial monitor connection
- Python with Pillow library

## Capture Process

### 1. Flash the Screenshots Branch

```bash
git checkout screenshots
make upload
```

The `screenshots` branch includes special firmware that enables screenshot capture via the BOOT button.

### 2. Connect Serial Monitor and Capture Output

```bash
make monitor > screenshot.log
# or
uv tool run platformio run -t monitor > screenshot.log
```

This redirects all serial output to `screenshot.log`.

### 3. Trigger Screenshot Capture

**Quick press** the BOOT button (< 1 second) after the device has fully booted.

The device will:

- Pause WebSocket connections to free memory
- Capture the current screen framebuffer
- Output hex data to serial (RGB565 format)
- Automatically reboot

### 4. Stop Monitoring

Press `Ctrl+C` to stop the serial monitor. The screenshot data is now saved in `screenshot.log`.

### 5. Extract PNG Image

```bash
make extract-screenshot
```

This will:

- Parse the hex data from `screenshot.log`
- Convert RGB565 format to PNG
- Generate a timestamped file: `screenshot_YYYYMMDD_HHMMSS.png`
- Clean up temporary raw files

## Button Behavior in Screenshots Branch

- **Quick Press (< 1 second)**: Capture screenshot (works in both normal and config modes)
- **Short Press (1-5 seconds)**: Cancel configuration mode
- **Medium Press (5-10 seconds)**: Enter configuration mode
- **Long Press (10+ seconds)**: Factory reset

## Technical Details

- **Format**: RGB565 (16-bit color, 2 bytes per pixel)
- **Resolution**: 240×320 pixels
- **Output**: Timestamped PNG files
- **Memory**: Requires ~150KB RAM for framebuffer capture
- **Reboot**: Device automatically reboots after capture to restore normal operation

## Troubleshooting

**"Insufficient memory" error:**

- The screenshots branch automatically pauses WebSocket to free memory

**No hex data in serial output:**

- Ensure you're using the `screenshots` branch firmware
- Check that serial monitor captured the full output
- Look for "SCREENSHOT START" and "SCREENSHOT END" markers

**Extraction fails:**

- Install Pillow: `pip install pillow` or use `uv run --with pillow`
- Verify `screenshot.log` contains hex data between markers

## Tips

- Capture different screens: main list, chart view, configuration portal
- Use various chart intervals (1m, 1h, 1d) for variety
- Capture during both bull and bear market conditions
- Take photos of the physical device for documentation
