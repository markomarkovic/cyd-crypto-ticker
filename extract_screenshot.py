#!/usr/bin/env python3
"""
Extract screenshot from serial monitor output and convert to PNG.

Usage:
    python extract_screenshot.py screenshot.log

The script will:
1. Extract hex data between SCREENSHOT START/END markers
2. Convert to raw RGB565 binary file
3. Use ImageMagick to convert to PNG
"""

import sys
import re
import subprocess
from pathlib import Path

def extract_screenshot(log_file):
    """Extract screenshot hex data from log file."""
    print(f"Reading log file: {log_file}")

    with open(log_file, 'r') as f:
        content = f.read()

    # Find screenshot data between markers
    start_marker = "========== SCREENSHOT START =========="
    end_marker = "========== SCREENSHOT END =========="

    start_idx = content.find(start_marker)
    end_idx = content.find(end_marker)

    if start_idx == -1 or end_idx == -1:
        print("Error: Screenshot markers not found in log file")
        print("Make sure to capture serial output from device after short button press")
        return None

    screenshot_section = content[start_idx:end_idx]

    # Extract metadata
    width = 240
    height = 320
    format_match = re.search(r'FORMAT:(\w+)', screenshot_section)
    width_match = re.search(r'WIDTH:(\d+)', screenshot_section)
    height_match = re.search(r'HEIGHT:(\d+)', screenshot_section)

    if format_match:
        img_format = format_match.group(1)
        print(f"Format: {img_format}")
    if width_match:
        width = int(width_match.group(1))
        print(f"Width: {width}")
    if height_match:
        height = int(height_match.group(1))
        print(f"Height: {height}")

    # Extract hex lines (lines containing only hex characters)
    hex_pattern = re.compile(r'^[0-9a-fA-F]+$', re.MULTILINE)
    hex_lines = hex_pattern.findall(screenshot_section)

    if not hex_lines:
        print("Error: No hex data found in screenshot section")
        return None

    print(f"Found {len(hex_lines)} hex data lines")

    # Combine all hex data
    hex_data = ''.join(hex_lines)
    print(f"Total hex characters: {len(hex_data)}")
    print(f"Expected bytes: {width * height * 2}")

    # Convert hex to bytes
    try:
        raw_data = bytes.fromhex(hex_data)
    except ValueError as e:
        print(f"Error converting hex data: {e}")
        return None

    print(f"Converted to {len(raw_data)} bytes")

    # Save raw RGB565 file with timestamp
    from datetime import datetime
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    raw_file = f"screenshot_{timestamp}.raw"
    with open(raw_file, 'wb') as f:
        f.write(raw_data)
    print(f"Saved raw file: {raw_file}")

    return raw_file, width, height, timestamp

def convert_to_png(raw_file, width, height, timestamp):
    """Convert RGB565 raw file to PNG using Python PIL."""
    png_file = f"screenshot_{timestamp}.png"

    print(f"\nConverting to PNG: {png_file}")

    try:
        import struct
        from PIL import Image

        # Read raw RGB565 data
        with open(raw_file, 'rb') as f:
            data = f.read()

        # Convert RGB565 (big-endian) to RGB888
        pixels = []
        for i in range(0, len(data), 2):
            # Read 16-bit value as big-endian (ESP32 ILI9341 uses big-endian RGB565)
            val = struct.unpack('>H', data[i:i+2])[0]

            # Extract RGB565 components
            r5 = (val >> 11) & 0x1F
            g6 = (val >> 5) & 0x3F
            b5 = val & 0x1F

            # Expand to 8-bit with MSB replication for better color accuracy
            r8 = (r5 << 3) | (r5 >> 2)
            g8 = (g6 << 2) | (g6 >> 4)
            b8 = (b5 << 3) | (b5 >> 2)

            pixels.append((r8, g8, b8))

        # Create and save image
        img = Image.new('RGB', (width, height))
        img.putdata(pixels)
        img.save(png_file)

        print(f"Successfully created: {png_file}")
        return png_file
    except ImportError:
        print("Error: PIL/Pillow not found")
        print("Install with: pip install pillow")
        print("Or use ImageMagick manually:")
        print(f"  convert -depth 5 -size {width}x{height} RGB565:{raw_file} {png_file}")
        return None
    except Exception as e:
        print(f"Error converting to PNG: {e}")
        return None

def main():
    if len(sys.argv) != 2:
        print("Usage: python extract_screenshot.py <screenshot.log>")
        print("\nTo capture screenshot:")
        print("1. Connect to device with serial monitor")
        print("2. Press BOOT button briefly (< 1 second)")
        print("3. Save serial output to a log file")
        print("4. Run this script on the log file")
        sys.exit(1)

    log_file = sys.argv[1]

    if not Path(log_file).exists():
        print(f"Error: Log file not found: {log_file}")
        sys.exit(1)

    result = extract_screenshot(log_file)
    if not result:
        sys.exit(1)

    raw_file, width, height, timestamp = result

    png_file = convert_to_png(raw_file, width, height, timestamp)
    if png_file:
        # Clean up raw file after successful conversion
        try:
            Path(raw_file).unlink()
            print(f"Cleaned up raw file: {raw_file}")
        except Exception as e:
            print(f"Warning: Could not delete raw file: {e}")
        print(f"\n✓ Screenshot saved as: {png_file}")
    else:
        print(f"\n✓ Raw data saved as: {raw_file}")
        print(f"  You can manually convert with:")
        print(f"  convert -depth 16 -size {width}x{height} -endian LSB rgb:{raw_file} screenshot_{timestamp}.png")

if __name__ == '__main__':
    main()
