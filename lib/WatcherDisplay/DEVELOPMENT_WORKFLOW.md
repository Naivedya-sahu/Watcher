# WatcherDisplay Development Workflow

Complete guide for developers working with the WatcherDisplay library, including custom font integration, testing, and deployment.

---

## Directory Structure

```
WatcherDisplay/
├── src/                           # Library source code
│   ├── WatcherDisplay.cpp         # Main display class
│   ├── WatcherDisplay.h
│   ├── FontHandler.cpp            # Custom font registry
│   └── FontHandler.h
│
├── examples/                      # Example sketches
│   ├── custom_fonts_demo.cpp      # Font system demo
│   └── ... (other examples)
│
├── tools/                         # Font generation tools (Python)
│   ├── generate_all.py            # Batch font generator
│   ├── font_generator.py          # Single font generator
│   ├── analyze_font.py            # Font analyzer
│   ├── test_system.py             # System tester
│   ├── gen.bat                    # Windows batch helper
│   ├── requirements.txt           # Python dependencies
│   └── FONT_GENERATOR_LICENSE.txt # MIT license for tools
│
├── fonts/                         # Font assets
│   ├── input/                     # Source TTF/OTF fonts
│   │   ├── Monocraft.ttf          # Example: pixel font
│   │   ├── Minecraft.otf          # Example: pixel font
│   │   └── ... (your fonts)
│   │
│   └── output/                    # Generated bitmap fonts
│       ├── Monocraft/             # Font family directory
│       │   ├── Monocraft12.cpp    # 12px size
│       │   ├── Monocraft12.h
│       │   ├── Monocraft16.cpp    # 16px size
│       │   ├── Monocraft16.h
│       │   └── ... (all 8 sizes)
│       └── ... (other fonts)
│
├── README.md                      # Main documentation
├── FONT_INTEGRATION_GUIDE.md      # Font system guide
├── GEOMETRIC_SHAPES_GUIDE.md      # Shape drawing guide
├── CUSTOM_FONTS_GUIDE.md          # Custom fonts (legacy reference)
├── DEVELOPMENT_WORKFLOW.md        # This file
└── library.json                   # PlatformIO library manifest
```

---

## Workflow 1: Adding Custom Fonts to a Project

### Step 1: Choose Your Fonts

**Option A: Use Included Example Fonts**

The library ships with sample fonts in `fonts/input/`:
- Monocraft.ttf (pixel art style)
- Minecraft.otf (blocky style)
- orange juice 2.0.ttf (casual style)

**Option B: Add Your Own Fonts**

Download fonts from:
- [Google Fonts](https://fonts.google.com/) - Huge selection, all free
- [Font Squirrel](https://www.fontsquirrel.com/) - Commercial-use fonts
- [DaFont](https://www.dafont.com/) - Check individual licenses

Copy `.ttf` or `.otf` files to `lib/WatcherDisplay/fonts/input/`

### Step 2: Set Up Python Environment

```bash
# Navigate to tools directory
cd lib/WatcherDisplay/tools

# Install Python dependencies (only Pillow needed)
pip install -r requirements.txt

# Or install manually
pip install Pillow>=10.0.0
```

**Verify installation:**
```bash
python -c "from PIL import Image; print('Pillow installed ✓')"
```

### Step 3: Analyze Font (Optional but Recommended)

Preview how a font will render at different sizes:

```bash
python analyze_font.py ../fonts/input/Monocraft.ttf
```

**Output example:**
```
Analyzing: Monocraft.ttf
Natural height at standard sizes:
  12px → 11px (fits ✓)
  16px → 15px (fits ✓)
  20px → 19px (fits ✓)
  24px → 22px (fits ✓)
  32px → 29px (fits ✓)
```

**Why analyze?**
- Fonts with tall descenders (g, j, p, q, y) may clip if natural height > target size
- Analysis helps you choose appropriate sizes

### Step 4: Generate Bitmap Fonts

**Batch generation (all fonts, all standard sizes):**

```bash
python generate_all.py
```

This generates 8 sizes (12, 14, 16, 18, 20, 24, 28, 32px) for each font in `fonts/input/`

**Output location:** `fonts/output/FontName/`

**Single font, single size:**

```bash
python font_generator.py ../fonts/input/Monocraft.ttf 20 Monocraft20
```

Creates `output/Monocraft20.cpp` and `Monocraft20.h`

**Custom size set:**

Edit `STANDARD_SIZES` in `generate_all.py`:

```python
STANDARD_SIZES = [10, 15, 22, 30, 40]  # Your custom sizes
```

### Step 5: Copy Generated Fonts to Project

Copy the `.cpp` and `.h` files you need to your project's `src/` directory:

```bash
# Example: Copy Monocraft 16px and 24px to main project
cp fonts/output/Monocraft/Monocraft16.* ../../src/
cp fonts/output/Monocraft/Monocraft24.* ../../src/

# Or copy all sizes
cp fonts/output/Monocraft/*.* ../../src/
```

### Step 6: Integrate in Code

**In your main sketch:**

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

    // Register fonts
    FONT_REGISTER("Monocraft", 16, Monocraft16);
    FONT_REGISTER("Monocraft", 24, Monocraft24);

    // Use fonts
    display.drawTextCustom(50, 50, "Hello World!", "Monocraft", 24, true);
    display.updateRegion(50, 50, 250, 35);
}
```

### Step 7: Build and Test

```bash
# Build your project
pio run -e your-environment

# Upload to device
pio run -e your-environment -t upload

# Monitor output
pio device monitor
```

---

## Workflow 2: Library Development

### Setting Up Development Environment

**Required:**
- PlatformIO Core or VSCode with PlatformIO extension
- Python 3.8+ with Pillow library
- ESP32-S3 or compatible board
- Waveshare 4.2" V2 e-paper display

**Clone and setup:**

```bash
git clone <your-repo-url>
cd Watcher
code .  # Open in VSCode
```

### Making Changes to Library Code

**1. Edit source files:**

```
lib/WatcherDisplay/src/
├── WatcherDisplay.cpp   ← Display wrapper implementation
├── WatcherDisplay.h     ← Public API
├── FontHandler.cpp      ← Font registry implementation
└── FontHandler.h        ← Font management API
```

**2. Test changes:**

Create a test sketch in `src/`:

```cpp
#include <WatcherDisplay.h>

WatcherDisplay display;

void setup() {
    display.begin();
    // Test your changes here
}

void loop() {
    // Test loop
}
```

**3. Build and verify:**

```bash
pio run -e your-test-env
pio run -e your-test-env -t upload
```

### Adding New Features

**Example: Adding a new drawing method**

**1. Add method declaration to `WatcherDisplay.h`:**

```cpp
class WatcherDisplay {
public:
    // ... existing methods ...

    /**
     * Draw a hexagon
     * @param x Center x coordinate
     * @param y Center y coordinate
     * @param radius Hexagon radius
     * @param color Color to draw
     * @param filled Fill hexagon or outline only
     */
    void drawHexagon(uint16_t x, uint16_t y, uint16_t radius,
                     uint16_t color, bool filled = false);
};
```

**2. Implement in `WatcherDisplay.cpp`:**

```cpp
void WatcherDisplay::drawHexagon(uint16_t x, uint16_t y, uint16_t radius,
                                  uint16_t color, bool filled) {
    // Implementation here
    // Use Paint_SelectImage() to work with buffer
    // Use existing Paint library functions
}
```

**3. Document in README.md**

**4. Add example in `examples/shapes_demo.cpp`**

**5. Test thoroughly**

### Testing Font Generator Changes

**Test workflow:**

```bash
cd lib/WatcherDisplay/tools

# Test single font generation
python font_generator.py ../fonts/input/Monocraft.ttf 16 TestFont16

# Verify output
ls ../fonts/output/TestFont16.*

# Test batch generation
python generate_all.py

# Test analyzer
python analyze_font.py ../fonts/input/Monocraft.ttf

# Run system tests
python test_system.py
```

---

## Workflow 3: Creating Examples

### Example Sketch Template

```cpp
/**
 * Example: Brief Description
 *
 * Demonstrates: Feature being shown
 *
 * Hardware:
 *   - ESP32-S3
 *   - Waveshare 4.2" V2 E-Paper
 */

#include <Arduino.h>
#include <WatcherDisplay.h>

WatcherDisplay display;

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Example Name ===\n");

    // Initialize
    if (!display.begin()) {
        Serial.println("Display init failed!");
        while(1);
    }

    display.clear();

    // Your demo code
    runDemo();
}

void loop() {
    // Optional: animated demo
}

void runDemo() {
    // Demo implementation
}
```

### Adding Examples to Library

**1. Create example file:**

```bash
# Create in examples directory
touch lib/WatcherDisplay/examples/my_feature_demo.cpp
```

**2. Write self-contained example**

**3. Document at top of file**

**4. Test on hardware**

**5. Reference in README.md**

---

## Workflow 4: Version Release

### Pre-Release Checklist

- [ ] All features tested on hardware
- [ ] Documentation updated
- [ ] Examples tested and working
- [ ] CHANGELOG.md updated
- [ ] Version bumped in `library.json`
- [ ] No compiler warnings
- [ ] Memory usage documented

### Version Bump

**Edit `library.json`:**

```json
{
  "version": "1.2.0",  // Update this
  // ...
}
```

**Semantic versioning:**
- Major (X.0.0): Breaking API changes
- Minor (1.X.0): New features, backward compatible
- Patch (1.1.X): Bug fixes only

### Creating Release

```bash
# Tag release
git tag -a v1.2.0 -m "Release v1.2.0: Custom font system"

# Push with tags
git push origin main --tags
```

---

## Workflow 5: Troubleshooting Common Issues

### Issue: Font Generation Fails

**Symptoms:**
```
ERROR: No font files found in fonts/input/
```

**Solution:**
1. Verify you're in `lib/WatcherDisplay/tools/` directory
2. Check `../fonts/input/` contains `.ttf` or `.otf` files
3. Run `ls ../fonts/input/` to list files

---

### Issue: Compilation Errors with Custom Fonts

**Symptoms:**
```
undefined reference to `Monocraft16`
```

**Solution:**
1. Ensure both `.cpp` and `.h` files are in `src/`
2. Check `#include "Monocraft16.h"` is present
3. Verify `FONT_REGISTER()` is called in `setup()`
4. Clean and rebuild: `pio run -t clean && pio run`

---

### Issue: Font Looks Wrong on Display

**Symptoms:**
- Characters clipped
- Spacing issues
- Garbled text

**Solutions:**

**1. Check font was registered:**
```cpp
if (FontHandler::getInstance().hasFont("Monocraft", 16)) {
    Serial.println("Font OK");
} else {
    Serial.println("Font not registered!");
}
```

**2. List all registered fonts:**
```cpp
FontHandler::getInstance().listFonts();
```

**3. Verify natural height:**
- Run `python analyze_font.py` on source TTF
- If natural height > target size, use larger size

---

### Issue: Display Not Updating

**Symptoms:**
- Text drawn but not visible
- Partial update not working

**Solutions:**

**1. Always call updateRegion():**
```cpp
display.drawTextCustom(50, 50, "Text", "Font", 16, true);
display.updateRegion(50, 50, 150, 25);  // ← Don't forget this!
```

**2. Check region coordinates:**
- Region must encompass drawn content
- X coordinates are auto-aligned to 8-pixel boundaries

**3. Force full refresh:**
```cpp
display.fullRefresh();  // Clears any issues
```

---

## Best Practices

### Code Organization

✅ **DO:**
- Keep font files in `src/` directory with your sketch
- Register all fonts in `setup()` once
- Use meaningful font names: `"Monocraft"`, `"Roboto"`
- Document which fonts your project uses

❌ **DON'T:**
- Register fonts in `loop()` repeatedly
- Mix font files between projects without copying
- Use hard-coded paths to font files

### Memory Management

✅ **DO:**
- Generate only the font sizes you actually use
- Free sub-buffers after use: `free(buffer)`
- Use `ESP.getFreeHeap()` to monitor memory

❌ **DON'T:**
- Generate all 8 sizes if you only need 2-3
- Allocate sub-buffers without freeing them
- Ignore memory allocation failures

### Font Selection

✅ **DO:**
- Use monospace fonts for timers and numeric displays
- Test fonts on actual hardware before finalizing
- Choose pixel fonts (Monocraft) for retro/gaming projects
- Use sans-serif fonts (Roboto) for modern UIs

❌ **DON'T:**
- Use complex decorative fonts (poor e-paper rendering)
- Mix too many font families (looks inconsistent)
- Use fonts smaller than 12px (hard to read on e-paper)

### Performance

✅ **DO:**
- Batch drawing operations before calling `updateRegion()`
- Use partial refresh for frequently updated regions
- Rely on auto-hybrid refresh for clean display

❌ **DON'T:**
- Call `fullRefresh()` every update (slow!)
- Update entire screen when only small region changed
- Disable auto-hybrid refresh unless you have specific needs

---

## Git Workflow

### Branch Strategy

```
main              ← Stable releases only
  └─ develop      ← Active development
      ├─ feature/custom-fonts   ← Feature branches
      ├─ feature/shapes         ← Feature branches
      └─ bugfix/ghosting-fix    ← Bug fixes
```

### Commit Messages

**Format:**
```
<type>: <description>

<optional body>
```

**Types:**
- `feat:` New feature
- `fix:` Bug fix
- `docs:` Documentation only
- `refactor:` Code restructuring
- `test:` Adding tests
- `chore:` Maintenance

**Examples:**
```
feat: Add custom TTF/OTF font support with generator tools

Integrates Python-based font generator that converts TTF/OTF files
to C++ bitmap arrays. Includes FontHandler class for runtime font
registration and management.

- Added tools/generate_all.py for batch font generation
- Added FontHandler singleton for font registry
- Added drawTextCustom() and drawNumberCustom() API methods
- Included sample fonts: Monocraft, Minecraft
- Documentation in FONT_INTEGRATION_GUIDE.md
```

---

## Testing Checklist

### Before Each Commit

- [ ] Code compiles without warnings
- [ ] Changes tested on actual hardware
- [ ] No memory leaks (check with `ESP.getFreeHeap()`)
- [ ] Examples still work
- [ ] Documentation updated

### Before Each Release

- [ ] All examples tested and working
- [ ] Font generator tested on Windows/Mac/Linux
- [ ] Memory usage profiled and documented
- [ ] README.md reflects all features
- [ ] Version number bumped
- [ ] CHANGELOG.md updated
- [ ] Git tag created

---

## Resources

### Documentation
- [FONT_INTEGRATION_GUIDE.md](FONT_INTEGRATION_GUIDE.md) - Font system complete guide
- [GEOMETRIC_SHAPES_GUIDE.md](GEOMETRIC_SHAPES_GUIDE.md) - Shape drawing reference
- [README.md](README.md) - Main library documentation

### Example Code
- [examples/custom_fonts_demo.cpp](examples/custom_fonts_demo.cpp) - Font usage examples
- See main project `src/` for real-world usage

### External Resources
- [Waveshare 4.2" V2 Wiki](https://www.waveshare.com/wiki/4.2inch_e-Paper_Module)
- [UC8176 Datasheet](https://www.waveshare.com/w/upload/3/3b/UC8176.pdf)
- [Pillow Documentation](https://pillow.readthedocs.io/)
- [PlatformIO Docs](https://docs.platformio.org/)

---

## Contributing

### How to Contribute

1. Fork the repository
2. Create feature branch: `git checkout -b feature/amazing-feature`
3. Make changes and test thoroughly
4. Commit: `git commit -m 'feat: add amazing feature'`
5. Push: `git push origin feature/amazing-feature`
6. Open Pull Request

### Pull Request Guidelines

**Include:**
- Clear description of changes
- Why the change is needed
- How to test the changes
- Screenshots/photos of hardware tests (if applicable)

**Checklist:**
- [ ] Code follows existing style
- [ ] Tested on real hardware
- [ ] Documentation updated
- [ ] Examples added (if new feature)
- [ ] No breaking changes (or clearly documented)

---

**Built for The Watcher Project** 🕐
*ESP32-S3 E-Paper Display Library*
