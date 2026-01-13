# Custom Fonts Guide for WatcherDisplay

Complete guide to using custom TTF/OTF fonts with the WatcherDisplay library.

## Overview

The WatcherDisplay library integrates with the **E-Paper Font Generator** to support custom fonts from any TrueType (.ttf) or OpenType (.otf) file. This allows you to use beautiful custom typography on your e-paper display.

## Workflow

```
┌──────────────┐
│  TTF/OTF     │  Download font files
│  Font Files  │  (Google Fonts, etc.)
└──────┬───────┘
       │
       ▼
┌──────────────────┐
│  Font Generator  │  Convert to C bitmap arrays
│  (Python tool)   │  assets/Custom_Font_Handler-main/
└──────┬───────────┘
       │
       ▼
┌──────────────────┐
│  .cpp/.h Files   │  Generated font files
│  (Bitmap data)   │  FontName12.cpp, FontName16.cpp, etc.
└──────┬───────────┘
       │
       ▼
┌──────────────────┐
│  Copy to         │  Add to your project
│  Project         │  lib/WatcherDisplay/fonts/
└──────┬───────────┘
       │
       ▼
┌──────────────────┐
│  Register        │  Use FONT_REGISTER()
│  in Code         │  in setup()
└──────┬───────────┘
       │
       ▼
┌──────────────────┐
│  Draw Text       │  display.drawTextCustom()
│  on Display      │  with custom fonts
└──────────────────┘
```

---

## Step 1: Get Font Files

### Free Font Sources

**Google Fonts** (best for e-paper):
- [Roboto](https://fonts.google.com/specimen/Roboto) - Modern, clean
- [IBM Plex Mono](https://fonts.google.com/specimen/IBM+Plex+Mono) - Professional monospace
- [Inter](https://fonts.google.com/specimen/Inter) - UI-optimized
- [Space Mono](https://fonts.google.com/specimen/Space+Mono) - Retro monospace

**Specialized Fonts**:
- [Monocraft](https://github.com/IdreesInc/Monocraft) - Minecraft-style (already included)
- [JetBrains Mono](https://www.jetbrains.com/lp/mono/) - Code font
- [Fira Code](https://github.com/tonsky/FiraCode) - Developer font

### Font Recommendations for E-Paper

✅ **Best choices:**
- **Monospace fonts** - Consistent spacing, easy alignment
- **Sans-serif fonts** - Cleaner on low-resolution displays
- **Medium weights** - Regular or Medium (not Thin, not Extra Bold)
- **High x-height** - Better small-size readability

❌ **Avoid:**
- Very thin or very bold weights
- Highly decorative fonts
- Fonts with fine serifs (may disappear at small sizes)

---

## Step 2: Generate Bitmap Fonts

### Setup Font Generator (One-time)

```bash
cd assets/Custom_Font_Handler-main/

# Install dependencies
pip install Pillow

# Verify installation
python font_generator.py --help
```

### Generate Fonts

#### Method A: Batch Generate (Multiple Sizes)

Place your font files in `input/` directory:

```
assets/Custom_Font_Handler-main/
└── input/
    ├── Monocraft.ttf
    ├── IBMPlexMono-Regular.ttf
    └── Roboto-Regular.ttf
```

Run batch generator:

```bash
python generate_all.py
```

This generates standard sizes: **12, 14, 16, 18, 20, 24, 28, 32px**

Output:

```
output/
├── Monocraft/
│   ├── Monocraft12.cpp
│   ├── Monocraft12.h
│   ├── Monocraft16.cpp
│   ├── Monocraft16.h
│   ├── Monocraft24.cpp
│   └── Monocraft24.h
├── IBMPlexMono/
│   └── ... (similar)
└── Roboto/
    └── ... (similar)
```

#### Method B: Generate Single Font

For specific size or custom settings:

```bash
python font_generator.py input/Monocraft.ttf 20 FontMonocraft20 --output-dir output/Monocraft
```

Parameters:
- `input/Monocraft.ttf` - Input font file
- `20` - Size in pixels
- `FontMonocraft20` - Output name (C variable name)
- `--output-dir` - Output directory (optional)

#### Method C: Custom Sizes

Edit `generate_all.py` to customize sizes:

```python
# Line 16-17
STANDARD_SIZES = [12, 16, 20, 24, 32]  # Your custom sizes
```

Then run:

```bash
python generate_all.py
```

---

## Step 3: Add Fonts to Project

### Create Fonts Directory

```bash
# In your WatcherDisplay library
mkdir -p lib/WatcherDisplay/fonts
```

### Copy Generated Files

```bash
# Copy desired fonts to project
cp assets/Custom_Font_Handler-main/output/Monocraft/*.cpp lib/WatcherDisplay/fonts/
cp assets/Custom_Font_Handler-main/output/Monocraft/*.h lib/WatcherDisplay/fonts/
```

Your project structure:

```
lib/WatcherDisplay/
├── src/
│   ├── WatcherDisplay.h
│   ├── WatcherDisplay.cpp
│   ├── FontHandler.h
│   └── FontHandler.cpp
├── fonts/                    ← Custom fonts
│   ├── FontMonocraft12.cpp
│   ├── FontMonocraft12.h
│   ├── FontMonocraft16.cpp
│   ├── FontMonocraft16.h
│   ├── FontMonocraft24.cpp
│   └── FontMonocraft24.h
└── README.md
```

---

## Step 4: Use Custom Fonts in Code

### Full Example

```cpp
#include <WatcherDisplay.h>

// Include your custom fonts
#include "fonts/FontMonocraft16.h"
#include "fonts/FontMonocraft24.h"
#include "fonts/FontIBMPlex20.h"

WatcherDisplay display;

void setup() {
    Serial.begin(115200);

    // Initialize display
    if (!display.begin()) {
        Serial.println("Display init failed!");
        return;
    }

    display.clear();

    // ========== REGISTER FONTS ==========

    // Register all your custom fonts
    FONT_REGISTER("Monocraft", 16, FontMonocraft16);
    FONT_REGISTER("Monocraft", 24, FontMonocraft24);
    FONT_REGISTER("IBMPlex", 20, FontIBMPlex20);

    // List registered fonts (debug)
    display.getFontHandler().listFonts();

    // ========== USE FONTS ==========

    // Draw title with large Monocraft
    display.drawTextCustom(50, 20, "POMODORO", "Monocraft", 24, true);
    display.updateRegion(50, 20, 250, 30);

    // Draw body with medium IBMPlex
    display.drawTextCustom(50, 60, "Time Remaining:", "IBMPlex", 20, true);
    display.updateRegion(50, 60, 200, 25);

    // Draw numbers with Monocraft
    display.drawNumberCustom(50, 90, 25, "Monocraft", 16, true);
    display.drawTextCustom(80, 90, "minutes", "Monocraft", 16, true);
    display.updateRegion(50, 90, 150, 20);

    // Mix custom and standard fonts
    display.drawTextCustom(50, 120, "Status:", "Monocraft", 16, true);
    display.drawText(120, 120, "Running", &Font16, true);  // Standard font
    display.updateRegion(50, 120, 200, 20);
}

void loop() {
    // Update with custom font
    static int counter = 0;

    display.clearRegion(50, 150, 150, 25);
    display.drawTextCustom(50, 150, "Count:", "Monocraft", 16, true);
    display.drawNumberCustom(120, 150, counter++, "Monocraft", 16, true);
    display.updateRegion(50, 150, 150, 25);

    delay(1000);
}
```

### API Reference

#### Register Font

```cpp
// Method 1: Using macro (recommended)
FONT_REGISTER("FontName", sizeInPixels, fontVariable);

// Method 2: Using FontHandler directly
display.getFontHandler().registerFont("FontName", sizeInPixels, &fontVariable);
```

#### Draw Text with Custom Font

```cpp
uint16_t drawTextCustom(uint16_t x, uint16_t y, const char* text,
                        const char* fontName, uint8_t fontSize, bool colored = true);

// Example:
display.drawTextCustom(10, 10, "Hello", "Monocraft", 16, true);
```

#### Draw Number with Custom Font

```cpp
uint16_t drawNumberCustom(uint16_t x, uint16_t y, int number,
                          const char* fontName, uint8_t fontSize, bool colored = true);

// Example:
display.drawNumberCustom(100, 50, 42, "IBMPlex", 20, true);
```

#### Check if Font Exists

```cpp
if (display.getFontHandler().hasFont("Monocraft", 16)) {
    // Font is registered
}
```

#### List All Fonts (Debug)

```cpp
display.getFontHandler().listFonts();
// Prints to Serial:
// ========== Registered Fonts ==========
//   Monocraft       16px  (12x16)  2280 bytes
//   Monocraft       24px  (18x24)  5130 bytes
//   IBMPlex         20px  (15x20)  3562 bytes
// Total: 3/20 fonts
// ======================================
```

---

## Memory Usage

### Typical Font Sizes (95 ASCII characters)

| Size | Monospace | Proportional | ESP32 % |
|------|-----------|--------------|---------|
| 12px | ~1.5 KB   | ~1.2 KB      | 0.02%   |
| 16px | ~3 KB     | ~2.5 KB      | 0.04%   |
| 20px | ~4.5 KB   | ~3.8 KB      | 0.06%   |
| 24px | ~7 KB     | ~5.5 KB      | 0.09%   |
| 32px | ~12 KB    | ~9 KB        | 0.15%   |

### Example Project Memory

**Typical Pomodoro Timer:**
```
Fonts:
  Monocraft16    3 KB
  Monocraft24    7 KB
  IBMPlex20      4.5 KB
  Roboto12       1.5 KB
                 ────────
  Total         16 KB  (0.2% of ESP32 8MB flash)
```

Memory is negligible - feel free to include multiple fonts and sizes!

---

## Advanced Usage

### Dynamic Font Selection

```cpp
const char* getFontForSize(int value) {
    if (value > 100) return "Monocraft";
    else return "IBMPlex";
}

void displayValue(int val) {
    const char* fontName = getFontForSize(val);
    display.drawNumberCustom(50, 50, val, fontName, 20, true);
}
```

### Font Fallback

```cpp
void drawWithFallback(const char* text, const char* fontName, uint8_t size) {
    if (display.getFontHandler().hasFont(fontName, size)) {
        display.drawTextCustom(10, 10, text, fontName, size, true);
    } else {
        // Fall back to standard font
        display.drawText(10, 10, text, &Font16, true);
        Serial.printf("Font '%s' %dpx not found, using Font16\n", fontName, size);
    }
}
```

### Multi-Language Support

Generate fonts with extended character sets:

```python
# In font_generator.py, modify character range:
def generate_font_bitmap(..., start_char=0x20, end_char=0xFF):  # Extended ASCII
    ...
```

For UTF-8, you'll need to modify the generator to support Unicode ranges.

---

## Troubleshooting

### Problem: Font looks compressed/clipped

**Cause:** Natural font height exceeds target size

**Solution:** Use larger size

```bash
# Analyze font first
python tools/analyze_font.py input/YourFont.ttf

# Output shows natural height at each size:
# Size 16: natural_height=18px (may clip)
# Size 18: natural_height=20px (may clip)
# Size 20: natural_height=22px (OK)

# Use size 20 instead of 16
python font_generator.py input/YourFont.ttf 20 FontYourFont20
```

### Problem: ERROR: Custom font 'Name' 16px not found

**Cause:** Font not registered

**Solution:**

```cpp
// 1. Check font file is included
#include "fonts/FontName16.h"

// 2. Register in setup()
FONT_REGISTER("Name", 16, FontName16);

// 3. Verify name and size match exactly
display.drawTextCustom(x, y, "Text", "Name", 16, true);
//                                    ^^^^^^  ^^
//                                    Must match registration
```

### Problem: Compilation error: undefined reference to FontName16

**Cause:** Font .cpp file not compiled

**Solution:**

1. Verify both `.h` and `.cpp` files are in project
2. Check PlatformIO compiles the fonts directory:

```ini
# platformio.ini
[env:your_env]
build_flags = -Ilib/WatcherDisplay/fonts
build_src_filter = +<*> +<../lib/WatcherDisplay/fonts/>
```

### Problem: Characters appear as boxes

**Cause:** Font doesn't include that character

**Solution:** Regenerate with extended range or use different font

---

## Font Generation Best Practices

### 1. Choose Appropriate Sizes

**For 400×300 e-paper display:**
- **Headers**: 24-32px
- **Body text**: 16-20px
- **Labels/metadata**: 12-14px
- **Large numbers**: 32-48px

### 2. Test Readability

Generate a test size first:

```bash
python font_generator.py input/Font.ttf 16 FontTest16
```

Load on device, test readability, adjust size if needed.

### 3. Keep Multiple Sizes

Generate 2-3 sizes per font for hierarchy:

```bash
python font_generator.py input/Font.ttf 16 Font16
python font_generator.py input/Font.ttf 20 Font20
python font_generator.py input/Font.ttf 24 Font24
```

### 4. Monospace for Numbers

Use monospace fonts for numbers that change frequently (timers, counters) - prevents layout shifts:

```cpp
// Monospace - digits don't jump around
display.drawNumberCustom(100, 50, seconds, "Monocraft", 20, true);

// Proportional - "1" narrower than "0", layout shifts
display.drawNumberCustom(100, 50, seconds, "Roboto", 20, true);
```

---

## Example Projects

### Minimalist Clock

```cpp
FONT_REGISTER("IBMPlex", 32, FontIBMPlex32);
FONT_REGISTER("IBMPlex", 16, FontIBMPlex16);

void displayTime(int hours, int minutes) {
    char timeStr[6];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", hours, minutes);

    display.clearRegion(100, 100, 200, 40);
    display.drawTextCustom(100, 100, timeStr, "IBMPlex", 32, true);
    display.updateRegion(100, 100, 200, 40);

    display.drawTextCustom(150, 145, "Monday", "IBMPlex", 16, true);
    display.updateRegion(150, 145, 100, 20);
}
```

### Retro Game UI

```cpp
FONT_REGISTER("Monocraft", 16, FontMonocraft16);
FONT_REGISTER("Monocraft", 24, FontMonocraft24);

void displayGameUI(int score, int lives) {
    // Title
    display.drawTextCustom(120, 10, "SNAKE GAME", "Monocraft", 24, true);

    // Score
    display.drawTextCustom(50, 50, "Score:", "Monocraft", 16, true);
    display.drawNumberCustom(120, 50, score, "Monocraft", 16, true);

    // Lives (draw hearts)
    for (int i = 0; i < lives; i++) {
        display.drawTextCustom(50 + i*20, 75, "@", "Monocraft", 16, true);
    }

    display.updateRegion(40, 40, 200, 50);
}
```

---

## See Also

- [WatcherDisplay README](README.md) - Main library documentation
- [E-Paper Font Generator README](../../assets/Custom_Font_Handler-main/README.md) - Font generator docs
- [Google Fonts](https://fonts.google.com/) - Free font library
- [Font Squirrel](https://www.fontsquirrel.com/) - Commercial-use fonts

---

**Happy font designing!** 🎨
