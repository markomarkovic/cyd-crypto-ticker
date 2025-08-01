# JetBrains Mono Fonts for LVGL

This directory contains LVGL-compatible font files generated from JetBrains Mono TTF files.

## Font Generation

These fonts are generated from the original JetBrains Mono TTF files using the LVGL font converter tool. The generated files include:

- `jetbrains_mono_12.c` - 12pt JetBrains Mono font
- `jetbrains_mono_14.c` - 14pt JetBrains Mono font
- `jetbrains_mono_16.c` - 16pt JetBrains Mono font
- `jetbrains_mono_22.c` - 22pt JetBrains Mono font
- `jetbrains_mono_fonts.h` - Header file with font declarations

## License and Attribution

JetBrains Mono is licensed under the SIL Open Font License 1.1.

For the original font files, license terms, and attribution information, please refer to:

- **Official Repository**: [JetBrains/JetBrainsMono](https://github.com/JetBrains/JetBrainsMono)
- **License**: SIL Open Font License 1.1
- **Authors**: JetBrains team

Please ensure compliance with the SIL OFL 1.1 license terms when using these fonts.

## How to Regenerate Fonts

To regenerate these fonts from original JetBrains Mono TTF files:

1. Download JetBrains Mono TTF files from the official repository
2. Use the LVGL online font converter: https://lvgl.io/tools/fontconverter
3. Configure settings:
   - Set name `jetbrains_mono_XX` where `XX` is the size
   - Set size (12, 14, 16, 22)
   - Set BPP to _4_ (for anti-aliasing)
   - Leave fallback _empty_
   - Output _C file_
   - Do NOT enable _font compression_ for better performance on ESP32
   - Do NOT enable _horizontal subpixel rendering_ it breaks the rendering
   - Do NOT enable _try to use glyph color info..._
   - Upload the JetBrains Mono TTF file
     - Sizes 12, 14, 16: `JetBrainsMonoNL-Regular.ttf`
     - Size 22: `JetBrainsMonoNL-Bold.ttf`
   - Include ASCII range `0x20-0x7F` at minimum
   - Leave _symbols_ empty
4. Generate and download the C file
5. Replace the corresponding file in this directory
6. Potentially update font declarations to use `const lv_font_t` (LVGL requirement)

## Usage

These fonts are designed for use with LVGL graphics library in embedded applications. Include the header file in your source code:

```c
#include "jetbrains_mono_fonts.h"
```

Then reference the fonts in your LVGL code:

- `&jetbrains_mono_12`
- `&jetbrains_mono_14`
- `&jetbrains_mono_16`
- `&jetbrains_mono_22`
