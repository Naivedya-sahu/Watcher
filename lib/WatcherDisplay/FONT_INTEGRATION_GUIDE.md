# Custom Font Integration Guide

Complete guide for using custom TTF/OTF fonts with the WatcherDisplay library for e-paper displays.

## Overview

The WatcherDisplay library includes a complete custom font system that allows you to use **any TrueType (.ttf) or OpenType (.otf) font** on your e-paper display by converting them to bitmap arrays.

### System Components

```
WatcherDisplay/
├── tools/                      # Font generation tools
│   ├── generate_all.py         # Batch font generator
│   ├── font_generator.py       # Single font generator
│   ├── analyze_font.py         # Font analysis tool
│   ├── test_system.py          # System tester
│   ├── requirements.txt        # Python dependencies
│   └── gen.bat                 # Windows batch helper
│
├── fonts/                      # Font assets
│   ├── input/                  # Your TTF/OTF source fonts
│   │   ├── Monocraft.ttf       # Example fonts
│   │   ├── Minecraft.otf
│   │   └── orange juice 2.0.ttf
│   │
│   └── output/                 # Generated C++ bitmap fonts
│       ├── Monocraft/
│       │   ├── Monocraft12.cpp
│       │   ├── Monocraft12.h
│       │   ├── Monocraft16.cpp
│       │   ├── Monocraft16.h
│       │   └── ... (all sizes)
│       └── ...
│
└── src/                        # Library source
    ├── FontHandler.h           # Font registry system
    ├── FontHandler.cpp
    ├── WatcherDisplay.h        # Main display library
    └── WatcherDisplay.cpp
```

---

## Quick Start (5 Minutes)

### Step 1: Install Python Dependencies

```bash
# Navigate to tools directory
cd lib/WatcherDisplay/tools

# Install Pillow (Python imaging library)
pip install -r requirements.txt
```

**Note:** Only requirement is `Pillow>=10.0.0`

### Step 2: Add Your Fonts

Place your `.ttf` or `.otf` files in `lib/WatcherDisplay/fonts/input/`:

```
fonts/input/
├── Monocraft.ttf       ✓ Already included (example)
├── YourFont.ttf        ← Add your font here
└── AnotherFont.otf     ← Or here
```

**Free font sources:**
- [Google Fonts](https://fonts.google.com/) - Huge selection
- [Font Squirrel](https://www.fontsquirrel.com/) - Commercial-use fonts
- [DaFont](https://www.dafont.com/) - Check licenses

### Step 3: Generate Bitmap Fonts

```bash
cd lib/WatcherDisplay/tools
python generate_all.py
```

**Output:** Generates 8 standard sizes (12, 14, 16, 18, 20, 24, 28, 32px) for each font

```
fonts/output/
└── Monocraft/
    ├── Monocraft12.cpp  ← 12px bitmap font
    ├── Monocraft12.h
    ├── Monocraft16.cpp  ← 16px bitmap font
    ├── Monocraft16.h
    └── ... (up to 32px)
```

### Step 4: Copy to Your Project

Copy the generated `.cpp` and `.h` files you want to use:

```bash
# Example: Copy Monocraft 16px and 24px
cp fonts/output/Monocraft/Monocraft16.* ../../src/
cp fonts/output/Monocraft/Monocraft24.* ../../src/
```

### Step 5: Register and Use Fonts

**In your sketch:**

```cpp
#include <WatcherDisplay.h>

// Include generated font headers
#include "Monocraft16.h"
#include "Monocraft24.h"

WatcherDisplay display;

void setup() {
    Serial.begin(115200);

    // Initialize display
    display.begin();
    display.clear();

    // Register custom fonts
    FONT_REGISTER("Monocraft", 16, Monocraft16);
    FONT_REGISTER("Monocraft", 24, Monocraft24);

    // Use custom font
    display.drawTextCustom(50, 50, "Hello World!", "Monocraft", 16, true);
    display.updateRegion(50, 50, 200, 30);
}

void loop() {
    // Your code
}
```

**Done!** Your custom font is now displaying on e-paper.

---

## Detailed Usage

### Font Generation Options

#### Standard Batch Generation

Generates all standard sizes (12, 14, 16, 18, 20, 24, 28, 32px):

```bash
cd lib/WatcherDisplay/tools
python generate_all.py
```

#### Custom Size Set

Edit `STANDARD_SIZES` in `generate_all.py`:

```python
# Edit this line to generate custom sizes
STANDARD_SIZES = [10, 15, 22, 30, 40, 50]
```

Then run:

```bash
python generate_all.py
```

#### Single Font, Single Size

Generate one specific size:

```bash
cd lib/WatcherDisplay/tools
python font_generator.py ../fonts/input/Monocraft.ttf 18 Monocraft18
```

**Arguments:**
1. Font file path
2. Size in pixels
3. Output name (without extension)

**Output:** Creates `output/Monocraft18.cpp` and `Monocraft18.h`

#### Analyze Font Before Generating

Check if a font will fit at target sizes:

```bash
python analyze_font.py ../fonts/input/Monocraft.ttf
```

**Output:**
```
Font Analysis: Monocraft.ttf
Natural height at different sizes:
  12px → 11px natural height ✓
  16px → 15px natural height ✓
  24px → 22px natural height ✓
  32px → 29px natural height ✓
```

**Why?** Some fonts have tall descenders (g, j, p, q, y). If natural height > target size, characters may clip. Use analysis to pick appropriate sizes.

---

### Font Registration API

#### Basic Registration

```cpp
#include "FontMonocraft16.h"

void setup() {
    FONT_REGISTER("Monocraft", 16, Monocraft16);
}
```

**Macro expands to:**
```cpp
FontHandler::getInstance().registerFont("Monocraft", 16, &Monocraft16);
```

#### Register Multiple Sizes

```cpp
#include "Monocraft12.h"
#include "Monocraft16.h"
#include "Monocraft24.h"
#include "Monocraft32.h"

void setup() {
    // Register all sizes of Monocraft
    FONT_REGISTER("Monocraft", 12, Monocraft12);
    FONT_REGISTER("Monocraft", 16, Monocraft16);
    FONT_REGISTER("Monocraft", 24, Monocraft24);
    FONT_REGISTER("Monocraft", 32, Monocraft32);
}
```

#### Register Multiple Fonts

```cpp
#include "Monocraft16.h"
#include "Minecraft16.h"
#include "Roboto20.h"

void setup() {
    FONT_REGISTER("Monocraft", 16, Monocraft16);
    FONT_REGISTER("Minecraft", 16, Minecraft16);
    FONT_REGISTER("Roboto", 20, Roboto20);
}
```

---

### Using Custom Fonts

#### Method 1: High-Level API (Recommended)

**Draw text with custom font:**

```cpp
display.drawTextCustom(x, y, text, fontName, fontSize, colored);
```

**Example:**

```cpp
// Black text "TIMER" in Monocraft 24px
display.drawTextCustom(100, 50, "TIMER", "Monocraft", 24, true);

// White text "PAUSE" in Minecraft 16px (on black background)
display.fillRegion(50, 100, 150, 30, COLORED);  // Black background
display.drawTextCustom(60, 105, "PAUSE", "Minecraft", 16, false);
```

**Draw number with custom font:**

```cpp
display.drawNumberCustom(x, y, number, fontName, fontSize, colored);
```

**Example:**

```cpp
int countdown = 299;  // 4:59

// Display countdown in Monocraft 32px
display.drawNumberCustom(150, 100, countdown, "Monocraft", 32, true);
```

#### Method 2: Direct Font Access

**Get font object and use with standard API:**

```cpp
sFONT* font = FONT_GET("Monocraft", 16);
if (font) {
    display.drawText(100, 50, "Hello", font, true);
}
```

#### Method 3: Low-Level Paint API

**Use with GUI_Paint library directly:**

```cpp
sFONT* font = FONT_GET("Monocraft", 24);
if (font) {
    Paint_SelectImage(display.getBuffer());
    Paint_DrawString_EN(100, 50, "Hello", font, WHITE, BLACK);
    display.updateRegion(100, 50, 200, 40);
}
```

---

### Font Management

#### Check if Font Exists

```cpp
bool hasFont = FontHandler::getInstance().hasFont("Monocraft", 16);
if (hasFont) {
    // Use font
} else {
    Serial.println("Font not registered!");
}
```

#### List All Registered Fonts

```cpp
void setup() {
    FONT_REGISTER("Monocraft", 16, Monocraft16);
    FONT_REGISTER("Monocraft", 24, Monocraft24);
    FONT_REGISTER("Minecraft", 20, Minecraft20);

    // Debug: List all fonts
    FontHandler::getInstance().listFonts();
}
```

**Serial output:**
```
Registered fonts (3):
  - Monocraft 16px
  - Monocraft 24px
  - Minecraft 20px
```

#### Unregister Font

```cpp
// Unregister specific size
FontHandler::getInstance().unregisterFont("Monocraft", 16);

// Unregister all sizes of a font
FontHandler::getInstance().unregisterFont("Monocraft", 0);

// Unregister ALL fonts
FontHandler::getInstance().clear();
```

---

## Real-World Examples

### Example 1: Pomodoro Timer with Custom Font

```cpp
#include <WatcherDisplay.h>
#include "Monocraft32.h"  // Large digits
#include "Monocraft16.h"  // Labels

WatcherDisplay display;
int minutes = 25;
int seconds = 0;

void setup() {
    display.begin();
    display.clear();

    // Register fonts
    FONT_REGISTER("Monocraft", 32, Monocraft32);
    FONT_REGISTER("Monocraft", 16, Monocraft16);

    // Draw title
    display.drawTextCustom(120, 10, "POMODORO", "Monocraft", 16, true);
    display.updateRegion(120, 10, 200, 30);
}

void loop() {
    // Clear timer area
    display.clearRegion(50, 60, 300, 50);

    // Format time as "MM:SS"
    char timeStr[6];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", minutes, seconds);

    // Draw timer in large font
    display.drawTextCustom(100, 70, timeStr, "Monocraft", 32, true);
    display.updateRegion(50, 60, 300, 50);

    // Countdown logic
    delay(1000);
    if (seconds == 0) {
        if (minutes > 0) {
            minutes--;
            seconds = 59;
        }
    } else {
        seconds--;
    }
}
```

### Example 2: Multi-Font Dashboard

```cpp
#include <WatcherDisplay.h>
#include "Monocraft24.h"   // Header
#include "Roboto16.h"      // Body text
#include "Roboto12.h"      // Small labels

WatcherDisplay display;

void setup() {
    display.begin();
    display.clear();

    FONT_REGISTER("Monocraft", 24, Monocraft24);
    FONT_REGISTER("Roboto", 16, Roboto16);
    FONT_REGISTER("Roboto", 12, Roboto12);

    drawDashboard();
}

void drawDashboard() {
    // Header in Monocraft
    display.drawTextCustom(100, 10, "SYSTEM STATUS", "Monocraft", 24, true);
    display.updateRegion(100, 10, 250, 35);

    // Stats in Roboto
    display.drawTextCustom(20, 60, "Temperature:", "Roboto", 16, true);
    display.drawNumberCustom(200, 60, 23, "Roboto", 16, true);
    display.drawTextCustom(240, 60, "C", "Roboto", 12, true);

    display.drawTextCustom(20, 90, "Humidity:", "Roboto", 16, true);
    display.drawNumberCustom(200, 90, 65, "Roboto", 16, true);
    display.drawTextCustom(240, 90, "%", "Roboto", 12, true);

    display.updateRegion(20, 60, 260, 50);

    // Footer in small font
    display.drawTextCustom(140, 270, "Updated: 12:45", "Roboto", 12, true);
    display.updateRegion(140, 270, 150, 20);
}

void loop() {
    // Update dashboard periodically
}
```

### Example 3: Font Size Comparison

```cpp
#include <WatcherDisplay.h>
#include "Monocraft12.h"
#include "Monocraft16.h"
#include "Monocraft20.h"
#include "Monocraft24.h"
#include "Monocraft32.h"

WatcherDisplay display;

void setup() {
    display.begin();
    display.clear();

    // Register all sizes
    FONT_REGISTER("Monocraft", 12, Monocraft12);
    FONT_REGISTER("Monocraft", 16, Monocraft16);
    FONT_REGISTER("Monocraft", 20, Monocraft20);
    FONT_REGISTER("Monocraft", 24, Monocraft24);
    FONT_REGISTER("Monocraft", 32, Monocraft32);

    // Display font samples
    int y = 10;
    display.drawTextCustom(10, y, "12px: Hello", "Monocraft", 12, true);
    y += 20;

    display.drawTextCustom(10, y, "16px: Hello", "Monocraft", 16, true);
    y += 25;

    display.drawTextCustom(10, y, "20px: Hello", "Monocraft", 20, true);
    y += 30;

    display.drawTextCustom(10, y, "24px: Hello", "Monocraft", 24, true);
    y += 35;

    display.drawTextCustom(10, y, "32px: Hello", "Monocraft", 32, true);

    display.fullRefresh();
}

void loop() {
    // Static display
}
```

---

## Memory Considerations

### Font Memory Usage

**Formula:** Memory per font ≈ `95 chars × (width × height / 8) bytes`

**Typical sizes:**

| Font Size | Width | Height | Memory | ESP32 Flash % |
|-----------|-------|--------|--------|---------------|
| 12px      | ~7px  | 12px   | ~1.2 KB | 0.015% |
| 16px      | ~10px | 16px   | ~2.4 KB | 0.03% |
| 20px      | ~12px | 20px   | ~3.6 KB | 0.045% |
| 24px      | ~14px | 24px   | ~5 KB | 0.06% |
| 32px      | ~18px | 32px   | ~9 KB | 0.11% |

**Example project:**
- 2 fonts × 3 sizes each = 6 total fonts
- Total: ~30 KB (0.375% of ESP32 flash)

**Recommendation:** Memory is negligible for ESP32. Generate all sizes you might need.

### Font Registry Limits

**Default:** `MAX_CUSTOM_FONTS = 20` fonts can be registered simultaneously

**To increase:**

Edit `FontHandler.h`:

```cpp
#ifndef MAX_CUSTOM_FONTS
#define MAX_CUSTOM_FONTS 50  // Increase to 50
#endif
```

**Memory impact:** ~20 bytes per slot (400 bytes total for 20 slots)

---

## Advanced Topics

### Character Range

**Default:** ASCII printable characters (0x20–0x7E)
- Includes: `A-Z a-z 0-9 ! " # $ % & ' ( ) * + , - . / : ; < = > ? @ [ \ ] ^ _ \` { | } ~`
- Total: 95 characters

**To generate extended ASCII or custom ranges:**

Edit `generate_all.py` line 85:

```python
# Default: ASCII 0x20-0x7E
for ascii_code in range(0x20, 0x7F):
    # ...

# Extended ASCII (0x20-0xFF)
for ascii_code in range(0x20, 0x100):
    # ...

# Only digits (0-9)
for ascii_code in range(0x30, 0x3A):
    # ...

# Custom set
for char in "0123456789:- TIMER":
    ascii_code = ord(char)
    # ...
```

**Benefits of reducing character set:**
- Saves memory (~30 bytes per excluded character)
- Useful if you only need digits/symbols for a clock display

### Font Subsetting (Manual)

If you only need specific characters (e.g., digits for a clock), manually edit generated `.cpp` file:

**Before (95 characters, ~5 KB):**
```cpp
const uint8_t Monocraft24_Table[] = {
    // All 95 ASCII characters
};
```

**After (13 characters "0123456789:-", ~700 bytes):**
```cpp
const uint8_t Monocraft24_Table[] = {
    // Only characters you need
};
```

**Trade-off:** Manual editing required after regeneration.

### Fallback to Built-in Fonts

If custom font not found, fall back to built-in:

```cpp
sFONT* getFont(const char* name, uint8_t size) {
    sFONT* font = FONT_GET(name, size);

    if (!font) {
        Serial.printf("Custom font '%s' %dpx not found, using Font16\n", name, size);
        return &Font16;  // Fallback to built-in
    }

    return font;
}

void drawSafeText(const char* text, const char* fontName, uint8_t size) {
    sFONT* font = getFont(fontName, size);
    display.drawText(100, 50, text, font, true);
}
```

---

## Troubleshooting

### Issue: "No font files found"

**Cause:** `fonts/input/` is empty

**Solution:** Place `.ttf` or `.otf` files in `lib/WatcherDisplay/fonts/input/`

---

### Issue: "Font not registered" at runtime

**Cause:** Forgot to call `FONT_REGISTER()` or misspelled font name

**Solution:**

```cpp
// 1. Include header
#include "Monocraft16.h"

// 2. Register in setup()
FONT_REGISTER("Monocraft", 16, Monocraft16);

// 3. Use exact name
display.drawTextCustom(x, y, "Text", "Monocraft", 16, true);  // ✓
display.drawTextCustom(x, y, "Text", "monocraft", 16, true);  // ✗ Case sensitive!
```

---

### Issue: Characters clipped (g, j, p, q, y cut off)

**Cause:** Font's natural height exceeds target size

**Solution:**

1. Run font analysis:

```bash
python analyze_font.py ../fonts/input/YourFont.ttf
```

2. Use a larger size if natural height > target:

```
20px font → 22px natural height ⚠️
Solution: Use 24px instead
```

---

### Issue: Compilation error "undefined reference to FontName"

**Cause:** Missing `.cpp` file in project

**Solution:** Ensure both `.cpp` and `.h` files are in your project's `src/` directory:

```
src/
├── main.cpp
├── Monocraft16.cpp  ✓ Required
└── Monocraft16.h    ✓ Required
```

---

### Issue: Font looks wrong/blurry

**Cause:** Pixel fonts work best at their intended size

**Solution:**
- Use pixel fonts (Monocraft, Minecraft) at multiples of their base size
- For smooth fonts (Roboto, Inter), any size works
- Test multiple sizes to find optimal rendering

---

## Font Recommendations

### Monospace Fonts (Great for E-Paper)

**Perfect for:**
- Digital clocks
- Timer displays
- Data tables
- Aligned layouts

**Recommended fonts:**

| Font | Style | Best For |
|------|-------|----------|
| **Monocraft** | Pixel art | Gaming/retro projects, very clear on e-paper |
| **IBM Plex Mono** | Professional | Corporate dashboards, clean data display |
| **JetBrains Mono** | Developer-friendly | Code snippets, technical displays |
| **Roboto Mono** | Modern | Material Design projects, Android-style UIs |
| **Courier New** | Classic | Traditional look, universally readable |

**Download:**
- Monocraft: [GitHub](https://github.com/IdreesInc/Monocraft)
- IBM Plex: [IBM Design](https://www.ibm.com/plex/)
- JetBrains Mono: [JetBrains](https://www.jetbrains.com/lp/mono/)
- Roboto: [Google Fonts](https://fonts.google.com/specimen/Roboto+Mono)

### Sans-Serif Fonts (Good Readability)

**Perfect for:**
- Body text
- Labels
- Instructions

**Recommended:**
- **Roboto** (Android standard, modern)
- **Inter** (UI-optimized, excellent small sizes)
- **Open Sans** (Friendly, readable)

### Display Fonts (Headers/Titles)

**Perfect for:**
- Large headings
- Visual impact
- Branding

**Recommended:**
- **Roboto Bold** (strong, same family as Roboto)
- **Bebas Neue** (tall, condensed, impactful)

---

## Integration with Existing Code

### Migration from Built-in Fonts

**Before:**
```cpp
display.drawText(100, 50, "TIMER", &Font24, true);
```

**After (custom font):**
```cpp
// Register once in setup()
FONT_REGISTER("Monocraft", 24, Monocraft24);

// Use anywhere
display.drawTextCustom(100, 50, "TIMER", "Monocraft", 24, true);
```

**Or use direct access:**
```cpp
sFONT* customFont = FONT_GET("Monocraft", 24);
display.drawText(100, 50, "TIMER", customFont, true);
```

---

## Development Workflow

### Recommended Workflow

1. **Design:** Prototype your UI, decide which fonts/sizes you need

2. **Source:** Download fonts from Google Fonts, Font Squirrel, etc.

3. **Generate:**
   ```bash
   cd lib/WatcherDisplay/tools
   python generate_all.py
   ```

4. **Test:** Copy one size to your project, test on hardware

5. **Iterate:** Adjust size if needed, regenerate

6. **Optimize:** Only include fonts/sizes you actually use

7. **Document:** Note which fonts you're using in your project README

---

## License

The font generator tools are MIT licensed (see `tools/FONT_GENERATOR_LICENSE.txt`).

**Note:** Font licenses are separate! Check the license of each font you use. Most free fonts allow embedding in projects, but verify before commercial use.

---

## See Also

- [WatcherDisplay README](README.md) - Main library documentation
- [GEOMETRIC_SHAPES_GUIDE.md](GEOMETRIC_SHAPES_GUIDE.md) - Drawing shapes
- [examples/custom_fonts_demo.cpp](examples/custom_fonts_demo.cpp) - Complete working example

---

**Built for The Watcher Project** 🕐
*ESP32-S3 E-Paper Display Library*
