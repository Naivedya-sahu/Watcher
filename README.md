# The Watcher - ESP32-S3 E-Paper Timer

Minimal Pomodoro timer with Waveshare 4.2" e-paper (400×300, UC8176).

## Hardware

- **MCU**: ESP32-S3-DevKitC-1 N8R8
- **Display**: Waveshare 4.2" V2 (400×300 B/W, UC8176)
- **Buttons**: GPIO 35, 36, 37
- **Vibrator**: GPIO 38

## Pin Map

```text
Display:  CS→10, DC→15, RST→16, BUSY→17, MOSI→11, SCK→12
Buttons:  Left→35, Mid→36, Right→37
Vibrator: PWM→38
```

## Quick Start

```bash
# Pomodoro V2 Timer (buffer-isolated, recommended)
pio run -e pomodoro-v2 -t upload

# Simple countdown timer
pio run -e simple -t upload

# Button input test
pio run -e buttons -t upload
```

## Libraries

### Waveshare EPD

**Location**: `lib/waveshare-epd/`

Full-featured e-paper display library with:

- **DEV_Config**: Hardware initialization (GPIO/SPI)
- **GUI_Paint**: Drawing API (fonts, shapes, text)
- **EPD_4IN2_V2**: UC8176 controller driver
- Zero external dependencies
- UC8176 commands optimized

**Example**:

```cpp
#include "EPD.h"
#include "GUI_Paint.h"

UBYTE *buffer = (UBYTE*)malloc(15000);
DEV_Module_Init();
EPD_4IN2_V2_Init();

Paint_NewImage(buffer, 400, 300, 0, WHITE);
Paint_SelectImage(buffer);
Paint_DrawString_EN(50, 50, "HELLO", &Font24, WHITE, BLACK);

EPD_4IN2_V2_Display(buffer);
```

## Builds

| Env | File | Lines | Description |
| --- | --- | --- | --- |
| `pomodoro-v2` | pomodoro_v2.cpp | 750 | **Buffer-isolated Pomodoro (recommended)** |
| `simple` | simple_timer_bitmap.cpp | 384 | Simple countdown timer |
| `buttons` | switch_observe.cpp | 92 | Button input test |

## UC8176 Controller

400×300 B/W e-paper driver:

- Full refresh: ~2s
- Partial refresh: ~400ms
- Anti-ghosting: Full refresh every 8 partials

## Pomodoro V2 Features

- **Buffer-isolated architecture**: Dedicated buffers for each UI region
- **Timer modes**: 5, 10, 15, 20, 25 minutes
- **7-segment display**: 70×130 pixel digits
- **Progress visualization**: 60 squares (1 per second)
- **Haptic feedback**: Vibrator alarm on completion
- **Button controls**:
  - Left (GPIO 35): START/PAUSE
  - Middle (GPIO 36): MODE select
  - Right (GPIO 37): RESET

See [BUFFER_ARCHITECTURE.md](BUFFER_ARCHITECTURE.md) for detailed implementation guide.

---

**Display**: Waveshare 4.2" V2 | **Controller**: UC8176 | **Resolution**: 400×300
