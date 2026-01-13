# The Watcher - Project Overview

**Updated:** 2026-01-06
**Platform:** PlatformIO + Arduino Framework
**Board:** ESP32-S3-DevKitC-1 N8R8 (8MB Flash, 8MB PSRAM)

---

## Active Project: `The Watcher/`

✅ **Unified PlatformIO project** - Ready for development and upload

### Project Structure

```text
The Watcher/
├── platformio.ini          # PlatformIO configuration
├── src/
│   └── main.cpp           # Clock implementation (7-segment + RTC)
├── lib/
│   └── waveshare-epd/     # E-Paper display drivers
├── assets/
│   ├── digits/            # Digit bitmaps (0-9.png/svg)
│   ├── screens/           # Screen designs (SVG)
│   └── Datasheet/         # Hardware docs
└── .pio/                  # Build artifacts (auto-generated)
```

### Reference Projects (Archive)

- `d/Watcher/` - Original working implementations
- `d/Watcher-claude/` - Bitmap rendering tests
- `Watcher-alpha/` - ESP-IDF version

---

## Hardware Configuration

### Components

- **ESP32-S3-DevKitC-1 N8R8** (8MB Flash, 8MB PSRAM)
- **Waveshare 4.2" E-Paper V2** (400×300, 4-bit grayscale)
- **DS3231 RTC Module** (I2C, battery-backed)
- **3× Mechanical Push Buttons** (future: Menu, Back, Action)

### Pin Mapping

| Component | Signal | GPIO | Notes |
|-----------|--------|------|-------|
| E-Paper | DIN (MOSI) | 11 | SPI |
| E-Paper | CLK (SCK) | 12 | SPI |
| E-Paper | CS | 10 | Chip Select |
| E-Paper | DC | 15 | Data/Command |
| E-Paper | RST | 16 | Reset |
| E-Paper | BUSY | 17 | Status |
| RTC DS3231 | SDA | 8 | I2C |
| RTC DS3231 | SCL | 9 | I2C |
| Button | MENU | 35 | Future |
| Button | BACK | 36 | Future |
| Button | ACTION | 37 | Future |

---

## Dependencies

### PlatformIO Libraries

```ini
lib_deps =
    adafruit/RTClib@^2.1.4
    adafruit/Adafruit BusIO@^1.16.2
```

### Custom Library

- **waveshare-epd** (in `lib/` folder) - E-Paper display drivers with GUI_Paint

---

## PlatformIO Upload

### Quick Upload Commands

```bash
# Navigate to project
cd "d:/Software/The Watcher"

# Build and upload
pio run -t upload

# Upload and monitor
pio run -t upload -t monitor
```

### Configuration

- Upload speed: `921600` baud
- Monitor speed: `115200` baud
- Auto-detects COM port

### Troubleshooting

**Port not found:**

- Install CH340/CP210x USB drivers
- Manually specify: `--upload-port COM3`

**Upload fails:**

- Hold BOOT button during upload
- Try slower speed: `upload_speed = 115200` in platformio.ini

---

## Current Status

### Working Features

- ✅ RTC (DS3231) integration
- ✅ E-Paper display (7-segment rendering)
- ✅ Partial refresh for digits
- ✅ Full refresh cycle (every 20 updates)
- ✅ Date display with day of week

### Assets Ready

- Digit bitmaps: `assets/digits/` (0-9.png/svg)
- Screen designs: `assets/screens/` (Clock, Pomodoro, Timer, Calendar)
- Hardware docs: `assets/Datasheet/`

### Next Steps

1. Test current firmware on hardware
2. Integrate bitmap rendering
3. Add button input system
4. Implement screen navigation

---

**Status:** ✅ Ready for Upload
**Command:** `cd "The Watcher" && pio run -t upload`

