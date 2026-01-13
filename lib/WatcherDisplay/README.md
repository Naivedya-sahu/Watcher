# WatcherDisplay Library

High-level display wrapper for **Waveshare 4.2" V2 E-Paper** (400×300, UC8176 controller) that automatically fixes partial refresh ghosting and provides a simplified graphics API.

## Features

### Core Features

✅ **Automatic Ghosting Fix** - Hybrid refresh strategy (partial + periodic full refresh)
✅ **Simple API** - High-level methods for common graphics operations
✅ **Buffer Management** - Multi-buffer support for complex UIs
✅ **Smart Region Updates** - Automatic byte alignment and bounds checking
✅ **Performance Optimized** - Fast partial refresh (~300-400ms) with clean display

### Drawing & Graphics

✅ **Custom Fonts** - TTF/OTF font support via integrated FontHandler
✅ **Geometric Shapes** - Triangles, polygons, arcs, ellipses, stars, hexagons
✅ **Advanced Drawing** - Bezier curves, thick lines, rounded rectangles
✅ **7-Segment Digits** - Built-in support for large timer displays
✅ **Progress Bars** - Filled and segmented styles

### Developer Features

✅ **Font Generator Integration** - Convert any TTF/OTF to bitmap fonts
✅ **Extensible Architecture** - Easy to add custom UI elements
✅ **Comprehensive Examples** - Full code samples for all features

## The Problem This Solves

The Waveshare EPD library's `EPD_4IN2_V2_PartialDisplay()` function has a known issue: **repeated partial refreshes cause ghosting and corruption** because it only updates RAM 0x24 (new image), not RAM 0x26 (previous image for comparison).

**WatcherDisplay fixes this automatically** by tracking partial refresh count and forcing a full refresh every N updates (default: 5). This gives you:
- **Fast updates** with partial refresh (~300-400ms)
- **Clean display** maintained by periodic full refresh (~2-3s)
- **68% faster** than full-refresh-only approach

See [PARTIAL_REFRESH_FINDINGS.md](../../Documentation/PARTIAL_REFRESH_FINDINGS.md) for technical details.

## Quick Start

### 1. Include the Library

```cpp
#include <WatcherDisplay.h>

// Create display instance (full refresh every 5 partial updates)
WatcherDisplay display;
```

### 2. Initialize Hardware

```cpp
void setup() {
    Serial.begin(115200);

    // Initialize display
    if (!display.begin()) {
        Serial.println("Display init failed!");
        return;
    }

    // Clear screen
    display.clear();
}
```

### 3. Draw and Update

```cpp
void loop() {
    // Draw something
    display.clearRegion(10, 10, 200, 50);
    display.drawText(10, 10, "Hello World!", &Font24, true);

    // Update region (auto-hybrid refresh)
    display.updateRegion(10, 10, 200, 50);

    delay(1000);
}
```

That's it! The library handles all the complexity of hybrid refresh automatically.

## API Reference

### Initialization

#### `WatcherDisplay(uint8_t fullRefreshInterval = 5)`
Constructor. Set how many partial refreshes before forcing full refresh (default: 5).

```cpp
WatcherDisplay display;        // Default: full refresh every 5 partials
WatcherDisplay display(10);    // Custom: full refresh every 10 partials
```

#### `bool begin(bool fastInit = false)`
Initialize display hardware and allocate buffers.

```cpp
display.begin();           // Standard init (supports partial refresh)
display.begin(true);       // Fast init (~1s refresh, no partial support)
```

#### `void clear(uint16_t color = UNCOLORED)`
Clear entire display and reset buffers.

```cpp
display.clear();           // Clear to white
display.clear(COLORED);    // Clear to black
```

#### `void sleep()`
Put display into deep sleep mode to save power.

```cpp
display.sleep();
```

---

### Display Updates

#### `void updateRegion(uint16_t x, uint16_t y, uint16_t width, uint16_t height)`
**⭐ Main update method** - Use this for all updates. Performs partial refresh and automatically does full refresh when needed.

```cpp
// Draw timer digits
display.clearRegion(10, 10, 160, 140);
display.draw7SegmentDigit(10, 10, 5, 30, 5, COLORED);
display.updateRegion(10, 10, 160, 140);  // Auto-hybrid refresh ✅
```

#### `void fullRefresh()`
Force a full screen refresh (~2-3 seconds). Clears all ghosting.

```cpp
display.fullRefresh();  // Manual full refresh
```

#### `void partialRefresh(const UIRegion& region)`
Force a partial refresh (~300-400ms). **Warning**: May ghost after many updates. Prefer `updateRegion()`.

```cpp
UIRegion region(10, 10, 100, 50);
display.partialRefresh(region);  // Manual partial (not recommended)
```

#### `void maintainDisplay()`
Force full refresh if counter exceeded. Call periodically if managing updates manually.

```cpp
// In loop, if doing manual updates
display.maintainDisplay();
```

---

### Drawing Methods

#### Text & Numbers

```cpp
uint16_t drawText(uint16_t x, uint16_t y, const char* text, sFONT* font, bool colored = true)
uint16_t drawNumber(uint16_t x, uint16_t y, int number, sFONT* font, bool colored = true)
```

**Available fonts**: `Font8`, `Font12`, `Font16`, `Font20`, `Font24`

```cpp
// Black text on white background
display.drawText(10, 10, "Timer", &Font20, true);

// White text on black background
display.fillRegion(10, 50, 100, 30, COLORED);
display.drawText(20, 55, "Paused", &Font16, false);

// Draw countdown number
int remaining = 299;
display.drawNumber(100, 100, remaining, &Font24, true);
```

#### Basic Shapes

```cpp
void setPixel(uint16_t x, uint16_t y, uint16_t color)
void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
void drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color, bool filled = false)
void drawCircle(uint16_t x, uint16_t y, uint16_t radius, uint16_t color, bool filled = false)
```

```cpp
// Draw rectangle border
display.drawRect(10, 10, 100, 50, COLORED, false);

// Draw filled circle
display.drawCircle(200, 150, 30, COLORED, true);

// Draw line
display.drawLine(0, 50, 400, 50, COLORED);
```

#### Region Fill/Clear

```cpp
void clearRegion(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
void fillRegion(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
```

```cpp
// Clear area (fill with white)
display.clearRegion(50, 50, 200, 100);

// Fill area with black
display.fillRegion(50, 50, 200, 100, COLORED);
```

---

### Advanced Drawing

#### 7-Segment Digits

```cpp
void draw7SegmentDigit(uint16_t x, uint16_t y, uint8_t digit,
                       uint16_t segmentLength, uint16_t segmentThickness,
                       uint16_t color)
```

Perfect for large countdown timers!

```cpp
// Draw "25:00" timer (4 digits)
uint16_t segLen = 30;
uint16_t segThick = 5;
uint16_t digitWidth = segLen + 2 * segThick;
uint16_t digitSpacing = 10;
uint16_t colonSpacing = 15;

// Minutes: "2"
display.draw7SegmentDigit(10, 50, 2, segLen, segThick, COLORED);

// Minutes: "5"
display.draw7SegmentDigit(10 + digitWidth + digitSpacing, 50, 5, segLen, segThick, COLORED);

// Colon: ":" (draw two small circles)
uint16_t colonX = 10 + 2 * (digitWidth + digitSpacing) + colonSpacing;
display.drawCircle(colonX, 70, 3, COLORED, true);
display.drawCircle(colonX, 90, 3, COLORED, true);

// Seconds: "0"
display.draw7SegmentDigit(colonX + colonSpacing, 50, 0, segLen, segThick, COLORED);

// Seconds: "0"
display.draw7SegmentDigit(colonX + colonSpacing + digitWidth + digitSpacing, 50, 0, segLen, segThick, COLORED);
```

#### Progress Bars

```cpp
void drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                     uint8_t progress, bool filled = true)
```

```cpp
// Filled progress bar (0-100%)
display.drawProgressBar(50, 200, 300, 20, 75, true);

// Segmented progress bar (10 segments)
display.drawProgressBar(50, 230, 300, 20, 75, false);
```

#### Bitmaps

```cpp
void drawBitmap(uint16_t x, uint16_t y, const unsigned char* bitmap,
                uint16_t width, uint16_t height)
```

```cpp
// Custom 32x32 icon
const unsigned char icon[] PROGMEM = { /* bitmap data */ };
display.drawBitmap(100, 100, icon, 32, 32);
```

---

### Buffer Management

For complex UIs with multiple independent regions (digits, progress bars, buttons), use sub-buffers to avoid interference.

#### `UBYTE* createSubBuffer(uint16_t width, uint16_t height)`
Allocate isolated buffer for a UI element. **You must `free()` when done!**

```cpp
// Create 160x140 buffer for digit region
UBYTE* digitBuffer = display.createSubBuffer(160, 140);
if (!digitBuffer) {
    Serial.println("Buffer allocation failed!");
    return;
}

// Draw to sub-buffer using Paint library
Paint_SelectImage(digitBuffer);
Paint_Clear(UNCOLORED);
// ... draw digits ...

// Copy to main screen buffer
display.copySubBuffer(digitBuffer, 0, 0, 160, 140, 50, 50);

// Update region on screen
display.updateRegion(50, 50, 160, 140);

// Cleanup
free(digitBuffer);
```

#### `void copySubBuffer(...)`
Copy sub-buffer contents to main screen buffer.

```cpp
void copySubBuffer(const UBYTE* subBuffer,
                   uint16_t srcX, uint16_t srcY,      // Source position in sub-buffer
                   uint16_t width, uint16_t height,   // Region size
                   uint16_t destX, uint16_t destY)    // Destination on screen
```

#### `UBYTE* getBuffer()`
Get direct access to main 15KB screen buffer (advanced use).

```cpp
UBYTE* mainBuffer = display.getBuffer();
// Manipulate buffer directly with Paint_SelectImage()
```

---

### Configuration

#### `void setFullRefreshInterval(uint8_t interval)`
Change hybrid refresh interval.

```cpp
display.setFullRefreshInterval(10);  // Full refresh every 10 partials
```

#### `void setAutoFullRefresh(bool enabled)`
Enable/disable automatic full refresh.

```cpp
display.setAutoFullRefresh(false);  // Disable auto full refresh
// Now you must call maintainDisplay() or fullRefresh() manually
```

#### `uint8_t getPartialRefreshCount()`
Get number of partial refreshes since last full refresh.

```cpp
if (display.getPartialRefreshCount() >= 8) {
    display.fullRefresh();
}
```

#### `void resetRefreshCounter()`
Reset counter (call after manual full refresh).

```cpp
EPD_4IN2_V2_Display(customBuffer);  // Manual full refresh
display.resetRefreshCounter();       // Reset counter
```

---

## Complete Examples

### Example 1: Simple Countdown Timer

```cpp
#include <WatcherDisplay.h>

WatcherDisplay display;
int countdown = 60;

void setup() {
    Serial.begin(115200);
    display.begin();
    display.clear();
}

void loop() {
    // Clear timer area
    display.clearRegion(100, 100, 200, 50);

    // Draw countdown
    display.drawNumber(100, 100, countdown, &Font24, true);
    display.drawText(200, 100, "seconds", &Font16, true);

    // Update region (auto-hybrid refresh)
    display.updateRegion(100, 100, 200, 50);

    countdown--;
    if (countdown < 0) countdown = 60;

    delay(1000);
}
```

### Example 2: Pomodoro Timer with 7-Segment Display

```cpp
#include <WatcherDisplay.h>

WatcherDisplay display(5);  // Full refresh every 5 updates
int minutes = 25;
int seconds = 0;

void setup() {
    display.begin();
    display.clear();

    // Draw title
    display.drawText(120, 10, "POMODORO TIMER", &Font20, true);
    display.updateRegion(120, 10, 200, 30);
}

void loop() {
    // Clear digit area
    display.clearRegion(50, 60, 300, 80);

    // Draw MM:SS with 7-segment digits
    int min1 = minutes / 10;
    int min2 = minutes % 10;
    int sec1 = seconds / 10;
    int sec2 = seconds % 10;

    display.draw7SegmentDigit(50, 60, min1, 30, 5, COLORED);
    display.draw7SegmentDigit(110, 60, min2, 30, 5, COLORED);

    // Colon
    display.drawCircle(175, 80, 4, COLORED, true);
    display.drawCircle(175, 100, 4, COLORED, true);

    display.draw7SegmentDigit(190, 60, sec1, 30, 5, COLORED);
    display.draw7SegmentDigit(250, 60, sec2, 30, 5, COLORED);

    // Update digit region
    display.updateRegion(50, 60, 300, 80);

    // Draw progress bar
    int totalSeconds = 25 * 60;
    int elapsed = (25 - minutes) * 60 + (60 - seconds);
    int progress = (elapsed * 100) / totalSeconds;

    display.clearRegion(50, 180, 300, 30);
    display.drawProgressBar(50, 180, 300, 25, progress, false);
    display.updateRegion(50, 180, 300, 30);

    // Countdown logic
    delay(1000);
    if (seconds == 0) {
        if (minutes == 0) {
            minutes = 25;  // Reset
        } else {
            minutes--;
            seconds = 59;
        }
    } else {
        seconds--;
    }
}
```

### Example 3: Multi-Region UI with Sub-Buffers

```cpp
#include <WatcherDisplay.h>

WatcherDisplay display;

void setup() {
    display.begin();
    display.clear();

    // Create sub-buffer for header (isolated from other regions)
    UBYTE* headerBuffer = display.createSubBuffer(400, 40);
    Paint_SelectImage(headerBuffer);
    Paint_Clear(COLORED);  // Black background
    Paint_DrawString_EN(120, 10, "My Application", &Font20, COLORED, UNCOLORED);

    display.copySubBuffer(headerBuffer, 0, 0, 400, 40, 0, 0);
    display.updateRegion(0, 0, 400, 40);
    free(headerBuffer);

    // Draw status bar
    display.drawText(10, 260, "WiFi: Connected", &Font12, true);
    display.drawText(250, 260, "Battery: 85%", &Font12, true);
    display.updateRegion(0, 250, 400, 50);
}

void loop() {
    // Main content area updates without affecting header/footer
    static int counter = 0;

    display.clearRegion(50, 80, 300, 150);
    display.drawText(100, 120, "Counter:", &Font20, true);
    display.drawNumber(220, 120, counter++, &Font20, true);
    display.updateRegion(50, 80, 300, 150);

    delay(2000);
}
```

---

## Migration from Raw Waveshare Library

**Before (Raw Waveshare):**

```cpp
// Manual buffer management
UBYTE *BlackImage;
BlackImage = (UBYTE*)malloc(BUFFER_SIZE);
Paint_NewImage(BlackImage, 400, 300, 270, UNCOLORED);
Paint_SelectImage(BlackImage);

// Manual partial refresh with ghosting issue
Paint_DrawString_EN(10, 10, "Hello", &Font20, UNCOLORED, COLORED);
partialRefresh(10, 10, 100, 30);  // Ghosts after 5-10 updates ❌

// Manual full refresh cycle
if (++refreshCount >= 5) {
    EPD_4IN2_V2_Display(BlackImage);
    refreshCount = 0;
}
```

**After (WatcherDisplay):**

```cpp
// Automatic buffer management
WatcherDisplay display;
display.begin();

// Automatic hybrid refresh (no ghosting) ✅
display.drawText(10, 10, "Hello", &Font20, true);
display.updateRegion(10, 10, 100, 30);  // Auto-handles full refresh cycle
```

**Benefits:**
- ✅ No manual buffer allocation
- ✅ No manual refresh counter tracking
- ✅ No ghosting issues
- ✅ Simpler, cleaner code

---

## Performance

**5-minute Pomodoro Timer (300 seconds, 1 update/second):**

| Strategy | Partial Refreshes | Full Refreshes | Total Time | Notes |
|----------|------------------|----------------|------------|-------|
| Full-only | 0 | 300 | ~600s | Clean but slow ❌ |
| Partial-only | 300 | 0 | ~90s | Fast but ghosts ❌ |
| **Hybrid (interval=5)** | **240** | **60** | **~192s** | **Fast + clean ✅** |

**Hybrid is 68% faster** than full-only while maintaining display quality.

---

## Architecture

```
┌─────────────────────────────────────┐
│       Your Application              │
│  (Simple high-level API calls)      │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│      WatcherDisplay Library         │
│  • Automatic hybrid refresh         │
│  • Buffer management                │
│  • Region tracking                  │
│  • High-level graphics API          │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│    Waveshare EPD Library            │
│  • GUI_Paint (drawing primitives)   │
│  • EPD_4in2_V2 (UC8176 driver)      │
│  • DEV_Config (SPI/GPIO)            │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│   UC8176 E-Paper Controller         │
│   (400×300 Waveshare 4.2" V2)       │
└─────────────────────────────────────┘
```

---

## Technical Details

### Refresh Strategies

| Method | Speed | RAM Updates | Ghosting | Use Case |
|--------|-------|-------------|----------|----------|
| Full Refresh | ~2-3s | 0x24 + 0x26 | None ✅ | Initial display, periodic cleanup |
| Partial Refresh | ~300-400ms | 0x24 only | After 5-10 updates ⚠️ | Fast incremental updates |
| **Hybrid** | **Mixed** | **Auto-managed** | **None ✅** | **Production use (recommended)** |

### Byte Alignment

E-paper pixels are stored 8 per byte (1-bit per pixel). X coordinates must be byte-aligned (multiple of 8) for partial refresh.

**WatcherDisplay handles this automatically:**

```cpp
// You request: x=13, width=45
// Library uses: x=8, width=48 (byte-aligned)
display.updateRegion(13, 10, 45, 30);  // Auto-aligns to 8-pixel boundaries
```

### Memory Usage

- **Main screen buffer**: 15,000 bytes (50 bytes/row × 300 rows)
- **Temp region buffer**: Allocated during partial refresh, freed immediately
- **Sub-buffers**: User-allocated, user-managed

**Example memory footprint:**
```
Main buffer:     15,000 bytes
Digit region:     2,800 bytes (160×140, temporary)
Progress region:  7,488 bytes (288×208, temporary)
Total:           ~25 KB peak during updates
```

---

## Future Extensions

This library is designed to be easily extended for new UI features:

### Custom UI Widget Example

```cpp
class TimerWidget {
private:
    WatcherDisplay* display;
    uint16_t x, y;

public:
    TimerWidget(WatcherDisplay* disp, uint16_t _x, uint16_t _y)
        : display(disp), x(_x), y(_y) {}

    void draw(int minutes, int seconds) {
        // Clear area
        display->clearRegion(x, y, 200, 80);

        // Draw custom timer design
        int min1 = minutes / 10;
        int min2 = minutes % 10;
        int sec1 = seconds / 10;
        int sec2 = seconds % 10;

        display->draw7SegmentDigit(x, y, min1, 25, 4, COLORED);
        display->draw7SegmentDigit(x + 40, y, min2, 25, 4, COLORED);
        display->draw7SegmentDigit(x + 100, y, sec1, 25, 4, COLORED);
        display->draw7SegmentDigit(x + 140, y, sec2, 25, 4, COLORED);

        // Update region
        display->updateRegion(x, y, 200, 80);
    }
};

// Usage
WatcherDisplay display;
TimerWidget timer(&display, 50, 60);
timer.draw(25, 30);
```

### Custom Font System

**NEW: Integrated Font Generator** - Convert any TTF/OTF font to bitmap arrays for e-paper!

The library now includes a complete custom font system with Python-based font generator tools.

#### Quick Start

**1. Generate fonts from TTF/OTF files:**

```bash
cd lib/WatcherDisplay/tools
pip install -r requirements.txt
python generate_all.py
```

**2. Include and register fonts:**

```cpp
#include <WatcherDisplay.h>
#include "Monocraft16.h"
#include "Monocraft24.h"

void setup() {
    display.begin();

    // Register custom fonts
    FONT_REGISTER("Monocraft", 16, Monocraft16);
    FONT_REGISTER("Monocraft", 24, Monocraft24);
}
```

**3. Use custom fonts:**

```cpp
// Draw text with custom font
display.drawTextCustom(50, 20, "HELLO WORLD", "Monocraft", 24, true);

// Draw numbers with custom font
display.drawNumberCustom(50, 60, 42, "Monocraft", 16, true);
```

#### Font System Features

✅ **Automatic Font Generation** - Batch convert TTF/OTF to C++ bitmap arrays
✅ **Standard Sizes** - Auto-generates 12, 14, 16, 18, 20, 24, 28, 32px
✅ **Font Analysis Tool** - Preview font metrics before generating
✅ **Easy Integration** - Simple registration and usage API
✅ **Memory Efficient** - Only ~2-9 KB per font depending on size
✅ **Sample Fonts Included** - Monocraft, Minecraft, and more

**See [FONT_INTEGRATION_GUIDE.md](FONT_INTEGRATION_GUIDE.md) for complete documentation.**

### Geometric Shapes

Rich set of drawing primitives for advanced graphics:

```cpp
// Triangles, polygons, stars
display.drawTriangle(50, 20, 30, 60, 70, 60, COLORED, true);
display.drawStar(200, 150, 30, 12, 5, COLORED, true);

// Arcs, ellipses, rounded rectangles
display.drawArc(100, 100, 40, 0, 180, COLORED);
display.drawEllipse(200, 100, 50, 30, COLORED, true);
display.drawRoundRect(50, 200, 100, 40, 10, COLORED, false);

// Bezier curves, thick lines
display.drawBezier(50, 50, 100, 20, 150, 50, COLORED);
display.drawThickLine(50, 100, 150, 100, 5, COLORED);
```

**See [GEOMETRIC_SHAPES_GUIDE.md](GEOMETRIC_SHAPES_GUIDE.md) for all shape functions.**

---

## Complete Documentation

📖 **Comprehensive Guides:**

- **[FONT_INTEGRATION_GUIDE.md](FONT_INTEGRATION_GUIDE.md)** - Complete custom font system documentation
  - Font generation workflow
  - TTF/OTF conversion tools
  - Font registration and usage
  - Memory optimization
  - Troubleshooting

- **[GEOMETRIC_SHAPES_GUIDE.md](GEOMETRIC_SHAPES_GUIDE.md)** - Advanced shape drawing
  - All geometric primitives
  - Bezier curves and arcs
  - Complex polygons
  - Filled/outlined variants

- **[examples/custom_fonts_demo.cpp](examples/custom_fonts_demo.cpp)** - Complete working examples
  - Font registration
  - Text and number rendering
  - Multi-font displays
  - Timer and dashboard UIs

---

## Troubleshooting

### Display not initializing

```cpp
if (!display.begin()) {
    Serial.println("Check wiring:");
    Serial.println("  CS=10, DC=15, RST=16, BUSY=17");
    Serial.println("  MOSI=11, SCK=12");
}
```

### Ghosting still appearing

```cpp
// Reduce refresh interval
display.setFullRefreshInterval(3);  // Full refresh every 3 partials

// Or force full refresh
display.fullRefresh();
```

### Memory allocation failure

```cpp
UBYTE* buffer = display.createSubBuffer(400, 300);
if (!buffer) {
    Serial.println("Out of memory!");
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    // Use smaller buffers or reduce allocation
}
```

### Slow updates

```cpp
// Disable auto full refresh for fastest updates (will ghost!)
display.setAutoFullRefresh(false);

// Update many times with partial only
for (int i = 0; i < 20; i++) {
    // ... draw ...
    display.updateRegion(...);  // Fast partial only
}

// Manually clean up
display.fullRefresh();
```

---

## License

MIT License - See project root for details.

---

## See Also

- [EPD_RENDERING_EXPLAINED.md](../../Documentation/EPD_RENDERING_EXPLAINED.md) - Technical deep-dive
- [PARTIAL_REFRESH_FINDINGS.md](../../Documentation/PARTIAL_REFRESH_FINDINGS.md) - Root cause analysis
- [BUFFER_ARCHITECTURE.md](../../Documentation/BUFFER_ARCHITECTURE.md) - Buffer design patterns
- [pomodoro.cpp](../../src/pomodoro.cpp) - Reference implementation

---

**Built for The Watcher Project** 🕐
*ESP32-S3 E-Paper Timer*
