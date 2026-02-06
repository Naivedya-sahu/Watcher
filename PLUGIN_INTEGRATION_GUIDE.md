# Plugin Integration Guide for The Watcher

## Overview

This guide explains how to integrate plugin-based functionality from InkyPi and TRMNL into your ESP32-S3 e-paper project.

## Repository Analysis

### 1. TRMNL (ESP32-Based) - Most Relevant
**Repository**: https://github.com/usetrmnl

**Architecture**:
- **Firmware**: ESP32-C3/ESP32 (Arduino framework, PlatformIO)
- **Backend**: Multiple implementations (Ruby, Python, Node.js, PHP, etc.)
- **Display**: BMP image rendering on e-paper
- **Power**: Deep sleep optimized (~100µA average, 120+ day battery life)

**How It Works**:
1. ESP32 wakes up on timer/button press
2. Fetches BMP image from server API (`/api/display`)
3. Renders image to e-paper display
4. Goes back to deep sleep

**Plugin System**:
- Plugins run on the **server-side** (not on ESP32)
- Server generates BMP images from plugin data
- ESP32 just displays pre-rendered images
- 20+ native plugins + 40+ community plugins

### 2. InkyPi (Raspberry Pi-Based)
**Repository**: https://github.com/fatihak/InkyPi

**Architecture**:
- **Platform**: Raspberry Pi (Python/Flask)
- **Interface**: Web UI for configuration
- **Display**: Pimoroni Inky or Waveshare displays
- **Plugins**: Run directly on the Pi

**Plugin Types**:
- Image Upload
- Clock
- Weather
- Calendar (Google, Outlook, Apple)
- AI Generation (OpenAI)
- Newspaper/Comics

**How It Works**:
1. Web UI manages plugin configuration
2. Plugins run as Python modules on the Pi
3. Direct rendering to e-paper display
4. Scheduled playlists for automated content rotation

## Integration Strategies for Your ESP32-S3 Project

### Strategy 1: TRMNL-Style Server-Client (Recommended for Complex Plugins)

**Architecture**:
```
[Server Backend] → [API] → [ESP32-S3] → [E-Paper Display]
     ↓
  [Plugins]
```

**Advantages**:
- Offloads processing from ESP32
- Better power efficiency
- Complex plugins possible (web APIs, databases, AI)
- Easy to add new plugins without firmware updates

**Implementation**:
1. Set up a backend server (Python/Node.js/Ruby)
2. Create plugin system that generates 400×300 BMP images
3. ESP32 fetches images via WiFi
4. Display on your Waveshare 4.2" display

**Example Flow**:
```cpp
// Pseudo-code for ESP32
WiFi.begin(ssid, password);
HTTPClient http;
http.begin("http://yourserver.com/api/display");
http.addHeader("Access-Token", apiKey);
int code = http.GET();
if (code == 200) {
    downloadBMPToBuffer(http.getStream());
    EPD_4IN2_V2_Display(imageBuffer);
}
WiFi.disconnect();
esp_deep_sleep_start();
```

### Strategy 2: Embedded Plugins (Direct on ESP32)

**Architecture**:
```
[ESP32-S3] → [Built-in Plugins] → [E-Paper Display]
```

**Advantages**:
- No server required
- Works offline
- Lower latency
- Simpler deployment

**Limitations**:
- Limited by ESP32 memory/processing
- No access to external APIs (unless WiFi enabled)
- Plugins must be compiled into firmware

**Implementation**:
Create plugin modules in C++ that generate display buffers:

```cpp
class Plugin {
public:
    virtual void render(UBYTE* buffer) = 0;
    virtual const char* getName() = 0;
};

class ClockPlugin : public Plugin {
public:
    void render(UBYTE* buffer) override {
        // Draw clock face
        Paint_SelectImage(buffer);
        Paint_DrawTime(getRTCTime(), &Font24);
    }
    const char* getName() override { return "Clock"; }
};
```

### Strategy 3: Hybrid Approach

**Architecture**:
```
[ESP32-S3] ←→ [Optional Server]
     ↓
[Local Plugins] + [Remote Plugins]
```

**Use Cases**:
- Local plugins: Clock, timer, simple displays (no WiFi needed)
- Remote plugins: Weather, calendar, news (fetch when WiFi available)

**Implementation**:
```cpp
enum PluginType { LOCAL, REMOTE };

struct PluginInfo {
    const char* name;
    PluginType type;
    void (*localRender)(UBYTE*);
    const char* remoteURL;
};

PluginInfo plugins[] = {
    {"Clock", LOCAL, renderClock, nullptr},
    {"Weather", REMOTE, nullptr, "http://server/weather.bmp"},
    {"Calendar", REMOTE, nullptr, "http://server/calendar.bmp"}
};
```

## Plugin Categories & ESP32 Compatibility

### ✅ Easily Implemented on ESP32

| Plugin | Implementation | Libraries Needed |
|--------|----------------|------------------|
| **Clock** | RTC module or NTP | `time.h`, `WiFi.h` |
| **Timer/Stopwatch** | Built-in millis() | None |
| **Counter/Tracker** | EEPROM persistence | `Preferences.h` |
| **Simple Graphics** | Draw shapes/patterns | GUI_Paint (already have) |
| **QR Codes** | Generate QR codes | `qrcode.h` |
| **Battery Status** | ADC reading | `esp_adc_cal.h` |

### ⚠️ Moderate Complexity (WiFi Required)

| Plugin | Implementation | Notes |
|--------|----------------|-------|
| **Weather** | OpenWeatherMap API | Parse JSON, render icons |
| **Bitcoin/Stocks** | API fetch | Render price charts |
| **RSS Feed** | Fetch & parse XML | Show headlines |
| **Google Calendar** | OAuth + API | Complex authentication |
| **GitHub Stats** | REST API | Fetch commit graphs |

### ❌ Challenging on ESP32 (Better Server-Side)

| Plugin | Why Server-Side is Better |
|--------|--------------------------|
| **AI Generation** | Requires OpenAI API, complex image processing |
| **Newspaper/Comics** | Web scraping, image manipulation |
| **Email Analytics** | OAuth flow, email parsing |
| **Complex Charts** | Heavy rendering libraries |
| **Photo Processing** | Memory intensive |

## Recommended Plugin Examples

See `assets/plugin_examples/` for ready-to-use implementations:

1. **Local Plugins** (no WiFi):
   - Multi-timer manager
   - Daily habit tracker
   - Focus mode scheduler
   - Battery monitor

2. **WiFi Plugins**:
   - Weather display
   - Bitcoin ticker
   - Calendar sync
   - RSS headlines

3. **Hybrid Plugins**:
   - Cached weather (updates hourly)
   - Offline-first todo list
   - Smart alarm (NTP sync)

## TRMNL Plugin Adaptation Guide

TRMNL plugins are Ruby/Python/JS code that generate screens. To adapt for ESP32:

### Option A: Use TRMNL Backend
1. Deploy TRMNL server (Python FastAPI or Node.js)
2. Use existing TRMNL plugins
3. ESP32 fetches generated BMP images
4. See `assets/trmnl_integration/` for ESP32 client code

### Option B: Port Plugin Logic
1. Study TRMNL plugin source (e.g., weather plugin)
2. Rewrite API calls in C++ (ESP32)
3. Render locally using GUI_Paint
4. See `assets/ported_plugins/` for examples

## InkyPi Plugin Adaptation Guide

InkyPi plugins are Python modules. To adapt:

### Option A: Keep Python Backend
1. Run InkyPi server on Raspberry Pi/PC
2. Modify to output BMP instead of direct display
3. ESP32 fetches from InkyPi API
4. See `assets/inkypi_bridge/` for implementation

### Option B: Port to C++
1. Analyze Python plugin logic
2. Rewrite in C++ for ESP32
3. Use equivalent libraries (ArduinoJson, HTTPClient)
4. See `assets/ported_plugins/` for examples

## Power Management Considerations

When adding plugins, consider TRMNL's power-efficient approach:

```cpp
// Good: Deep sleep between updates
void loop() {
    updateDisplay();  // 10s awake
    esp_deep_sleep(15 * 60 * 1e6);  // 15min sleep
    // Average: ~100µA, 120+ day battery
}

// Bad: Always-on loop
void loop() {
    updateDisplay();
    delay(60000);  // 60s delay
    // Average: ~80mA, <2 day battery
}
```

**Plugin Update Frequencies**:
- Clock: 1 minute partial refresh
- Weather: 30 minutes full refresh
- Calendar: 1 hour
- Stock prices: 15 minutes (market hours only)
- News: 4 hours

## Next Steps

1. **Choose your architecture** (Server-Client, Embedded, or Hybrid)
2. **Start with simple local plugins** (clock, timer)
3. **Add WiFi plugins gradually** (weather, stocks)
4. **Optimize power consumption** (deep sleep, partial refresh)
5. **Explore TRMNL/InkyPi codebases** for inspiration

## Additional Resources

- TRMNL Documentation: https://docs.trmnl.com
- TRMNL Plugins: https://github.com/usetrmnl/plugins
- InkyPi Wiki: https://github.com/fatihak/InkyPi/wiki
- Example implementations: `assets/` directory

---

**Your Current Setup**:
- ESP32-S3-DevKitC-1 N8R8
- Waveshare 4.2" V2 (400×300)
- 3 buttons + vibrator
- Optimized for Pomodoro timer

**Recommended First Plugin**: Multi-timer with mode selection (5/10/15/20/25 min) - you already have this! Consider adding WiFi sync for time accuracy.
