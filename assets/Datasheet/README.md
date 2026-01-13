# ESP32-S3 Habit Tracker

WiFi-connected habit tracker with e-ink display, rotary encoder, and cloud sync.

## Features

🎛️ **Rotary encoder** navigation with noise filtering
📶 **WiFi + NTP** time sync
🌐 **Web config** at `habittracker.local`
📝 **Obsidian sync** for tasks and calendar
🎨 **Custom fonts** via TTF/OTF upload
⏱️ **Pomodoro timer** with session tracking
✅ **Task list** with EEPROM persistence

## Hardware

- ESP32-S3-N8R8
- Waveshare 4.2" e-ink (400x300)
- DS3231 RTC
- Rotary encoder + 2 switches

## Quick Start

```bash
1. Upload firmware (Arduino IDE)
2. Connect to "HabitTracker-Setup" WiFi
3. Configure at 192.168.4.1
4. Access device at habittracker.local
```

**Full setup:** See `firmware/HabitTracker/SETUP.md`

## Configuration

All settings via web UI (no code changes):
- WiFi credentials
- Obsidian vault URL
- Pomodoro timers
- Custom fonts
- Timezone

## Obsidian Integration

Install "Local REST API" plugin, then:

```markdown
# tasks.md
- [ ] Your task here
- [x] Completed task
- [ ] Due date [[2024-12-01]]
```

Device syncs automatically every 15 min.

## Architecture

```
network/          WiFi, NTP, Obsidian sync, web server
ui/               Grid system, display, fonts
screens/          Clock, tasks, pomodoro
state/            Navigation state machine
hardware/         Encoder, RTC, e-ink init
```

## Development

**Add a new screen:**
1. Copy `screens/ScreenTemplate.h`
2. Implement `draw()` and `handleInput()`
3. Register in `HabitTracker.ino`

**Customize fonts:**
Upload TTF/OTF at `habittracker.local/fonts`

## Pin Map

```
E-ink:   BUSY→4, RST→16, DC→17, CS→5, CLK→18, DIN→23
RTC:     SDA→21, SCL→22
Encoder: CLK→32, DT→33, SW→25
Switches: BACK→26, MENU→27
```

## API

```bash
GET  /api              # Device status
POST /save             # Update settings
POST /sync             # Trigger Obsidian sync
GET  /fonts            # List fonts
POST /fonts/upload     # Upload font file
```

---

**Documentation:** `firmware/HabitTracker/SETUP.md`
**Encoder tuning:** `firmware/HabitTracker/ROTARY_ENCODER_GUIDE.md`
**Examples:** `firmware/HabitTracker/screens/`
