# WatcherDisplay Font Generator Tools

Python-based tools for converting TrueType/OpenType fonts to C++ bitmap arrays for e-paper displays.

## Quick Start

### 1. Install Dependencies

```bash
pip install -r requirements.txt
```

Only requirement: `Pillow>=10.0.0`

### 2. Add Your Fonts

Place `.ttf` or `.otf` files in `../fonts/input/`:

```
fonts/input/
├── Monocraft.ttf       ✓ Included (example)
├── YourFont.ttf        ← Add here
└── AnotherFont.otf     ← Add here
```

### 3. Generate Fonts

```bash
python generate_all.py
```

**Output:** `../fonts/output/FontName/` with 8 sizes (12, 14, 16, 18, 20, 24, 28, 32px)

### 4. Use Generated Fonts

Copy `.cpp` and `.h` files to your project and use:

```cpp
#include "Monocraft16.h"

FONT_REGISTER("Monocraft", 16, Monocraft16);
display.drawTextCustom(50, 50, "Hello!", "Monocraft", 16, true);
```

---

## Available Tools

### generate_all.py

**Purpose:** Batch convert all fonts in `../fonts/input/` to bitmap arrays

**Usage:**
```bash
python generate_all.py
```

**Output:** Generates 8 standard sizes for each font
- 12px, 14px, 16px, 18px, 20px, 24px, 28px, 32px

**Customize sizes:** Edit `STANDARD_SIZES` in the script

---

### font_generator.py

**Purpose:** Generate a single font at a specific size

**Usage:**
```bash
python font_generator.py <font_path> <size> <output_name>
```

**Example:**
```bash
python font_generator.py ../fonts/input/Monocraft.ttf 20 Monocraft20
```

**Output:**
- `../fonts/output/Monocraft20.cpp`
- `../fonts/output/Monocraft20.h`

---

### analyze_font.py

**Purpose:** Preview font metrics before generating

**Usage:**
```bash
python analyze_font.py <font_path>
```

**Example:**
```bash
python analyze_font.py ../fonts/input/Monocraft.ttf
```

**Output:**
```
Font Analysis: Monocraft.ttf
Natural height at standard sizes:
  12px → 11px (fits ✓)
  16px → 15px (fits ✓)
  24px → 22px (fits ✓)
  32px → 29px (fits ✓)
```

**Why use this?**
- Check if characters with descenders (g, j, p, q, y) will clip
- Verify font will render properly at target sizes
- Choose appropriate sizes before generating

---

### test_system.py

**Purpose:** Test that the font generation system is working

**Usage:**
```bash
python test_system.py
```

**What it does:**
- Verifies Pillow is installed
- Tests font loading
- Tests bitmap generation
- Validates output format

---

### gen.bat (Windows only)

**Purpose:** Quick batch generation for Windows users

**Usage:**
```cmd
gen.bat
```

Equivalent to `python generate_all.py` but easier to double-click on Windows.

---

## Generated File Format

### Output Structure

```
fonts/output/
└── Monocraft/
    ├── Monocraft12.cpp     # 12px bitmap data + sFONT struct
    ├── Monocraft12.h       # Header with extern declaration
    ├── Monocraft16.cpp
    ├── Monocraft16.h
    └── ... (all 8 sizes)
```

### Example Generated Code

**Monocraft16.h:**
```cpp
#ifndef __MONOCRAFT16_H__
#define __MONOCRAFT16_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "fonts.h"

extern sFONT Monocraft16;

#ifdef __cplusplus
}
#endif

#endif
```

**Monocraft16.cpp:**
```cpp
#include "fonts.h"

const uint8_t Monocraft16_Table[] = {
    // Bitmap data for all 95 ASCII characters (0x20-0x7E)
    // Each character stored as rows of bytes
    // Visual comments show character appearance
};

sFONT Monocraft16 = {
  Monocraft16_Table,
  10,  /* Width */
  16,  /* Height */
};
```

---

## Configuration

### Changing Generated Sizes

Edit `generate_all.py`:

```python
# Line 17: Standard sizes to generate
STANDARD_SIZES = [12, 14, 16, 18, 20, 24, 28, 32]

# Change to your preferred sizes:
STANDARD_SIZES = [10, 15, 22, 30, 40]
```

### Changing Character Range

Edit `generate_all.py` line 85:

```python
# Default: ASCII printable (0x20-0x7E)
for ascii_code in range(0x20, 0x7F):
    # ...

# Extended ASCII (0x20-0xFF)
for ascii_code in range(0x20, 0x100):
    # ...

# Only digits (0-9)
for ascii_code in range(0x30, 0x3A):
    # ...
```

---

## Troubleshooting

### "No module named PIL"

**Solution:**
```bash
pip install Pillow
```

### "No font files found"

**Solution:**
- Check you're in `lib/WatcherDisplay/tools/` directory
- Verify `../fonts/input/` contains `.ttf` or `.otf` files
- Run `ls ../fonts/input/` to list files

### "Permission denied" on gen.bat

**Solution (Windows):**
- Right-click gen.bat → Properties → Unblock
- Or run: `python generate_all.py` directly

### Characters Look Clipped

**Solution:**
1. Run `python analyze_font.py ../fonts/input/YourFont.ttf`
2. Check "natural height" column
3. If natural height > target size, use a larger size
4. Regenerate with appropriate size

---

## Font Recommendations

### Best Fonts for E-Paper

**Monospace (Digital Displays):**
- Monocraft ✓ (included) - Pixel art style, very clear
- IBM Plex Mono - Professional, corporate
- JetBrains Mono - Developer-friendly
- Roboto Mono - Modern, Material Design

**Sans-Serif (Body Text):**
- Roboto - Clean, readable
- Inter - UI-optimized
- Open Sans - Friendly

**Where to Download:**
- [Google Fonts](https://fonts.google.com/) - Free, open source
- [Font Squirrel](https://www.fontsquirrel.com/) - Commercial-use fonts
- [IBM Plex](https://www.ibm.com/plex/) - Professional fonts

---

## Memory Usage

**Typical sizes per font (95 ASCII characters):**

| Size | Memory | ESP32 Flash % |
|------|--------|---------------|
| 12px | ~1.5 KB | 0.02% |
| 16px | ~3 KB | 0.04% |
| 24px | ~7 KB | 0.09% |
| 32px | ~12 KB | 0.15% |

**Example:** 4 fonts × 3 sizes each = ~50 KB total (0.6% of ESP32 flash)

Memory is negligible for ESP32. Generate all sizes you might need.

---

## License

These tools are MIT licensed - see `FONT_GENERATOR_LICENSE.txt`

**Note:** Font licenses are separate! Always check the license of fonts you use.

---

## Complete Documentation

📖 **See parent directory for full guides:**

- [../FONT_INTEGRATION_GUIDE.md](../FONT_INTEGRATION_GUIDE.md) - Complete custom font system documentation
- [../README.md](../README.md) - WatcherDisplay library documentation
- [../DEVELOPMENT_WORKFLOW.md](../DEVELOPMENT_WORKFLOW.md) - Development workflow guide

---

**Part of the WatcherDisplay Library**
*ESP32 E-Paper Display Library with Custom Font Support*
