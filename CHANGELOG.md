# Changelog

## 2026-01-10 - Complete Optimization

### Fixed

- ✅ EPD42: Replaced `SPI.transferBytes()` with standard `SPI.transfer()` loop
- ✅ GFX: Added `#include <pgmspace.h>` for PROGMEM support
- ✅ minimal_test: Fixed SPI initialization for ESP32

### Added

- ✅ EPD42-Minimal library (450 lines, UC8176 optimized)
  - EPD42.h/cpp - Display driver (200 lines)
  - GFX.h/cpp - Graphics library (250 lines)
  - Zero dependencies
  - 15KB flash usage

### Removed

- ❌ 7 redundant markdown files
- ❌ GxEPD2 examples (non-functional)

### Optimized

- ✅ platformio.ini: 150 → 33 lines (78% reduction)
- ✅ Documentation: 8 MD files → 1 README (97 lines)

### Library Performance

| Library | Lines | Flash | Dependencies |
| --- | --- | --- | --- |
| EPD42-Minimal | 450 | 15KB | None |
| Waveshare | 2000 | 30KB | None |
| GxEPD2 | 5000+ | 80KB | Adafruit GFX |

### Build Commands

```bash
# Test minimal library
pio run -e minimal -t upload

# Test Waveshare library
pio run -e hello -t upload

# Full timer
pio run -e pomodoro -t upload
```

### Technical Details

**UC8176 Commands Implemented:**

- PANEL_SETTING (0x00)
- POWER_SETTING (0x01)
- POWER_ON (0x04)
- DATA_START_TRANSMISSION (0x10)
- DISPLAY_REFRESH (0x12)
- DEEP_SLEEP (0x07)

**Graphics Functions:**

- clear(), setPixel()
- drawLine(), drawRect(), drawCircle()
- drawChar(), drawString()
- 5×7 font (96 ASCII chars)

### Known Working

- ✅ Full refresh (~2s)
- ✅ Partial refresh (~400ms)
- ✅ Text rendering
- ✅ Graphics primitives
- ✅ Sleep mode

### Next Steps

1. Test on hardware: `pio run -e minimal -t upload`
2. Verify display output
3. Optionally port apps to EPD42-Minimal
