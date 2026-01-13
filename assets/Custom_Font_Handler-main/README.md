# E-Paper Font Generator

> **Convert TrueType/OpenType fonts to C bitmap arrays for e-paper displays**

Automatic batch processing tool for generating Arduino/ESP32 compatible bitmap fonts from TTF/OTF files. Designed for Waveshare e-paper displays and GUI_Paint library.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## ✨ Features

- ✅ **Automatic batch processing** - Drop fonts in `input/`, run one command
- ✅ **Standard sizes** - Generates 12, 14, 16, 18, 20, 24, 28, 32px automatically
- ✅ **Organized output** - Each font in its own subdirectory
- ✅ **Descender support** - Properly handles lowercase g, j, p, q, y characters
- ✅ **Visual debugging** - Comments show character bitmaps in hex
- ✅ **Zero configuration** - Works out of the box
- ✅ **Cross-platform** - Windows, macOS, Linux

## 🚀 Quick Start

### 1. Install Dependencies

```bash
pip install -r requirements.txt
```

Or manually:
```bash
pip install Pillow
```

### 2. Add Your Fonts

Place `.ttf` or `.otf` files in the `input/` directory:

```
input/
├── Monocraft.ttf
├── IBMPlexMono-Regular.ttf
└── Roboto-Regular.ttf
```

### 3. Generate

```bash
python generate_all.py
```

### 4. Output

Find generated files in `output/` organized by font:

```
output/
├── Monocraft/
│   ├── Monocraft12.cpp
│   ├── Monocraft12.h
│   ├── Monocraft14.cpp
│   ├── Monocraft14.h
│   ├── ...
│   └── Monocraft32.h
├── IBMPlexMono/
│   ├── IBMPlexMono12.cpp
│   └── ...
└── Roboto/
    └── ...
```

## 📋 Integration with Arduino/ESP32

### Step 1: Copy Files

Copy desired `.cpp` and `.h` files to your Arduino project:

```bash
cp output/Monocraft/Monocraft16.* YourProject/
```

### Step 2: Update fonts.h

Add extern declarations to your `fonts.h`:

```c
// Custom fonts
#ifdef FONT_ENABLE_MONOCRAFT16
  extern sFONT Monocraft16;
#endif

#ifdef FONT_ENABLE_MONOCRAFT24
  extern sFONT Monocraft24;
#endif
```

### Step 3: Enable in Sketch

```cpp
// Define BEFORE #include "fonts.h"
#define FONT_ENABLE_MONOCRAFT16
#define FONT_ENABLE_MONOCRAFT24
#include "fonts.h"
```

### Step 4: Use Font

```cpp
Paint_DrawString_EN(x, y, "Hello World", &Monocraft16, BLACK, WHITE);
Paint_DrawString_EN(x, y, "Larger Text", &Monocraft24, BLACK, WHITE);
```

See [examples/arduino_example.ino](examples/arduino_example.ino) for complete code.

## 📐 Standard Sizes

| Size | Use Case | Typical Usage |
|------|----------|---------------|
| 12px | Tiny labels | Metadata, timestamps, fine print |
| 14px | Small text | Secondary information, captions |
| 16px | Body text | UI labels, descriptions, main text |
| 18px | Emphasized | Important labels, highlighted info |
| 20px | Subheaders | Section titles, categories |
| 24px | Headers | Screen titles, primary headings |
| 28px | Large headers | Main titles, emphasis |
| 32px | Display text | Time display, large numbers |

### Custom Sizes

Edit `STANDARD_SIZES` in `generate_all.py`:

```python
STANDARD_SIZES = [10, 15, 22, 30]  # Your custom sizes
```

## 🎨 Font Recommendations

### Monospace (UI, Data, Code)

Perfect for aligned layouts, tables, numeric displays:

- **Monocraft** - Pixel art aesthetic, gaming/retro projects
- **IBM Plex Mono** - Professional, clean, corporate
- **JetBrains Mono** - Developer-friendly, code displays
- **Roboto Mono** - Material Design, modern
- **Courier New** - Classic, universally available

### Sans-Serif (Body Text)

Better for readability in longer text:

- **Roboto** - Modern, geometric, Android standard
- **Open Sans** - Friendly, readable, web-optimized
- **Inter** - UI-optimized, excellent small sizes

### Display (Headers, Titles)

For visual impact:

- **Roboto Bold** - Strong emphasis, same family as Roboto
- **Bebas Neue** - Tall, condensed, impactful headers

### Where to Download

- [Google Fonts](https://fonts.google.com/) - Free, open-source
- [Font Squirrel](https://www.fontsquirrel.com/) - Commercial-use fonts
- [IBM Plex](https://www.ibm.com/plex/) - Professional fonts
- [JetBrains Mono](https://www.jetbrains.com/lp/mono/) - Code font

## 💾 Memory Usage

Typical memory per font (95 ASCII characters):

| Size | Memory | ESP32 % (8MB) |
|------|--------|---------------|
| 12px | ~1.5 KB | 0.02% |
| 16px | ~3 KB | 0.04% |
| 24px | ~7 KB | 0.09% |
| 32px | ~12 KB | 0.15% |

**Example:** 4 fonts × 3 sizes each = ~50 KB total (0.6% of ESP32 flash)

Memory is negligible for most projects. Generate all sizes you might need.

## 🛠️ Advanced Usage

### Analyze Font Before Generating

```bash
python .\tools\analyze_font.py '.\input\YourFont.ttf'
```

Shows natural height at each size to verify fit.

### Custom Character Range

Default: ASCII 0x20-0x7E (printable characters: space through tilde)

To modify, edit `generate_font_bitmap()` in `generate_all.py`:

```python
for ascii_code in range(0x20, 0xFF):  # Extended ASCII
for ascii_code in range(0x30, 0x3A):  # Only digits 0-9
```

### Generate Single Size

```bash
python font_generator.py input/Font.ttf 18 CustomName18
```

Creates `output/CustomName18.cpp` and `.h`

## 🐛 Troubleshooting

### "No font files found"

**Problem:** input/ directory is empty

**Solution:** Place `.ttf` or `.otf` files in `input/` directory

### Descenders clipping (g, j, p, q, y cut off)

**Problem:** Font natural height exceeds target size

**Solution:** 
1. Run `python analyze_font.py input/YourFont.ttf`
2. Check "natural_height" in output
3. Use larger size (e.g., 18px instead of 16px)

### Characters look compressed

**Problem:** Natural height > target cell height

**Solution:** Increase generation size by 2-4px

### Arduino compilation errors

**Problems:**
- "sFONT was not declared"
- "Undefined reference to FontName"

**Solutions:**
1. Verify `fonts.h` has `extern sFONT FontName;` declaration
2. Check `#define FONT_ENABLE_X` is BEFORE `#include "fonts.h"`
3. Ensure both `.cpp` and `.h` are in project directory
4. Verify files compiled (check Arduino IDE output)

### Font looks wrong on display

**Problem:** Alignment or spacing issues

**Solution:**
1. Check natural_height in generated `.cpp` header comments
2. Verify you're using correct font size
3. Test with different sizes to find optimal

## 📁 Project Structure

```
E-Paper-Font-Generator/
├── README.md                  ← This file
├── LICENSE                    ← MIT License
├── requirements.txt           ← Python dependencies
├── .gitignore                 ← Git ignore rules
│
├── generate_all.py            ← Main batch generator
├── font_generator.py          ← Single font generator
├── analyze_font.py            ← Font analysis tool
├── setup.py                   ← Initial setup helper
│
├── input/                     ← Place your TTF/OTF files here
│   └── (your fonts)
│
├── output/                    ← Generated files (auto-created)
│   └── FontName/
│       ├── FontName12.cpp
│       ├── FontName12.h
│       ├── FontName14.cpp
│       └── ...
│
├── examples/                  ← Example code
│   ├── arduino_example.ino    ← Complete Arduino example
│   └── (jupyter notebooks)
│
├── docs/                      ← Additional documentation
│   ├── FONT_LIBRARY_GUIDE.md  ← Font recommendations
│   └── (development notes)
│
└── tools/                     ← Development versions
    └── (generator iterations)
```

## ⚙️ How It Works

1. **Font Loading** - Uses PIL (Pillow) to load TTF/OTF files
2. **Rasterization** - Renders each character at target pixel size
3. **Measurement** - Calculates natural character bounds (handles descenders)
4. **Centering** - Positions characters vertically within target cell
5. **Bitmap Conversion** - Converts to 1-bit monochrome bitmap
6. **C Array Generation** - Creates byte arrays with visual comments
7. **sFONT Wrapping** - Packages in Waveshare-compatible structure

## 🔧 Compatibility

**Tested With:**
- Waveshare 4.2" e-paper display (400×300px)
- ESP32-S3 DevKit C-1
- Arduino IDE 2.x
- GUI_Paint library

**Should Work With:**
- All Waveshare e-paper displays (any size)
- Arduino Uno, Mega, Due
- ESP8266, ESP32 (all variants)
- STM32 (Blue Pill, Nucleo)
- Any microcontroller using GUI_Paint library

## 📜 License

MIT License - Free for personal and commercial use

See [LICENSE](LICENSE) for full text.

## 🤝 Contributing

Contributions welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 🙏 Credits

- **Font Rendering:** [Pillow (PIL Fork)](https://python-pillow.org/)
- **Compatible With:** Waveshare e-paper libraries
- **Inspired By:** Pixel font community and bitmap font tools
- **Created For:** [The Watcher Project](https://github.com/YourUsername/The-Watcher) - ESP32-S3 e-ink clock

## 📞 Support

- **Issues:** [GitHub Issues](https://github.com/YourUsername/E-Paper-Font-Generator/issues)
- **Discussions:** [GitHub Discussions](https://github.com/YourUsername/E-Paper-Font-Generator/discussions)
- **Documentation:** See `docs/` directory

## 🗺️ Roadmap

- [ ] GUI interface for font selection
- [ ] Preview mode before generating
- [ ] Support for non-Latin character sets
- [ ] Font subsetting (reduce memory)
- [ ] Anti-aliasing option for grayscale displays
- [ ] Direct integration with PlatformIO

## 📊 Changelog

See [CHANGELOG.md](CHANGELOG.md) for version history.

---

**Made with ❤️ for embedded developers**

*If this tool helped your project, please give it a ⭐ on GitHub!*
