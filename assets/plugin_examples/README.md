# Plugin Examples for The Watcher

Ready-to-use plugin implementations based on TRMNL and InkyPi architectures.

## Directory Structure

```
plugin_examples/
├── local_plugins/          # No WiFi required
│   ├── clock_plugin.cpp
│   └── habit_tracker_plugin.cpp
├── wifi_plugins/           # Requires WiFi
│   ├── weather_plugin.cpp
│   └── bitcoin_ticker_plugin.cpp
└── hybrid_plugins/         # Works offline + online
    └── cached_weather_plugin.cpp
```

## Plugin Categories

### 🔌 Local Plugins (Offline)

**Advantages:**
- No WiFi = Better battery life
- Always available
- Fast updates
- No API dependencies

**Examples:**
- **Clock** - RTC-based time display with NTP sync option
- **Habit Tracker** - Daily habit tracking with EEPROM persistence

### 📡 WiFi Plugins (Online)

**Advantages:**
- Access to real-time data
- Rich external content
- API integrations

**Requirements:**
- WiFi connection
- API keys (for some services)

**Examples:**
- **Weather** - OpenWeatherMap integration
- **Bitcoin Ticker** - CoinGecko crypto prices (no API key needed!)

### 🔄 Hybrid Plugins (Best of Both)

**Advantages:**
- Works offline with cached data
- Updates when WiFi available
- Power efficient
- Always displays something

**Examples:**
- **Cached Weather** - Updates hourly, works offline

## Quick Start

### 1. Local Plugin Example (Clock)

```cpp
#include "assets/plugin_examples/local_plugins/clock_plugin.cpp"

ClockPlugin clockPlugin;
UBYTE* buffer = (UBYTE*)malloc(15000);

void setup() {
    DEV_Module_Init();
    EPD_4IN2_V2_Init();
    clockPlugin.init();
}

void loop() {
    clockPlugin.render(buffer, 400, 300);
    EPD_4IN2_V2_Display_Partial(buffer);
    delay(60000);  // Update every minute
    clockPlugin.update();
}
```

### 2. WiFi Plugin Example (Weather)

```cpp
#include "assets/plugin_examples/wifi_plugins/weather_plugin.cpp"

WeatherPlugin weatherPlugin;

void setup() {
    WiFi.begin("ssid", "password");
    while (WiFi.status() != WL_CONNECTED) delay(500);

    weatherPlugin.init("your_api_key", "London");
    weatherPlugin.fetchWeather();
}
```

### 3. Hybrid Plugin Example (Cached Weather)

```cpp
#include "assets/plugin_examples/hybrid_plugins/cached_weather_plugin.cpp"

CachedWeatherPlugin weather;

void setup() {
    weather.init("api_key", "London");

    // Only connect WiFi if cache is stale
    if (weather.isCacheStale()) {
        WiFi.begin("ssid", "password");
        // ... wait for connection
        weather.update(true);
        WiFi.disconnect();
    }

    weather.render(buffer, 400, 300);
}
```

## Plugin Management System

### Option A: Simple Plugin Switcher

```cpp
enum PluginMode { CLOCK, WEATHER, BITCOIN, HABITS };
PluginMode currentMode = CLOCK;

void loop() {
    switch(currentMode) {
        case CLOCK:
            clockPlugin.render(buffer, 400, 300);
            break;
        case WEATHER:
            weatherPlugin.render(buffer, 400, 300);
            break;
        // ... more plugins
    }

    // Switch plugins with button
    if (buttonPressed(BUTTON_MODE)) {
        currentMode = (PluginMode)((currentMode + 1) % 4);
    }
}
```

### Option B: Scheduled Plugin Rotation

```cpp
struct PluginSchedule {
    Plugin* plugin;
    int displaySeconds;
};

PluginSchedule schedule[] = {
    {&clockPlugin, 60},      // Show clock for 60s
    {&weatherPlugin, 30},    // Then weather for 30s
    {&btcPlugin, 30},        // Then Bitcoin for 30s
};

int currentPlugin = 0;
unsigned long lastSwitch = 0;

void loop() {
    if (millis() - lastSwitch > schedule[currentPlugin].displaySeconds * 1000) {
        currentPlugin = (currentPlugin + 1) % 3;
        lastSwitch = millis();
    }

    schedule[currentPlugin].plugin->render(buffer, 400, 300);
    EPD_4IN2_V2_Display_Partial(buffer);
}
```

## Power Optimization Tips

### Deep Sleep Strategy

```cpp
// Good: Deep sleep between updates (120+ day battery)
void loop() {
    updatePlugin();
    EPD_4IN2_V2_Display(buffer);

    esp_sleep_enable_timer_wakeup(15 * 60 * 1000000ULL);  // 15 min
    WiFi.disconnect();
    esp_deep_sleep_start();
}
```

### Selective WiFi Strategy

```cpp
RTC_DATA_ATTR int bootCount = 0;

void setup() {
    bootCount++;

    // Only connect WiFi every 3rd boot (30 min intervals)
    if (bootCount % 3 == 0) {
        WiFi.begin(ssid, password);
        fetchOnlineData();
        WiFi.disconnect();
    }

    // Always display (uses cache if WiFi skipped)
    renderDisplay();

    esp_sleep_enable_timer_wakeup(10 * 60 * 1000000ULL);
    esp_deep_sleep_start();
}
```

## API Keys & Configuration

### OpenWeatherMap (Weather Plugin)
1. Sign up at https://openweathermap.org/api
2. Get free API key (1000 calls/day)
3. Set in code: `weatherPlugin.init("YOUR_API_KEY", "CityName");`

### CoinGecko (Bitcoin Plugin)
- **No API key needed!** Free tier works out of the box
- 50 calls/minute limit
- `btcPlugin.init("bitcoin", "usd");`

### TRMNL Server (For Full Plugin Ecosystem)
- Deploy server: See `../trmnl_integration/`
- Access 60+ plugins instantly
- Server generates BMP images, ESP32 just displays

## Extending Plugins

### Create Your Own Plugin

```cpp
class MyCustomPlugin {
private:
    // Your data members

public:
    void init() {
        // Initialize plugin
    }

    void render(UBYTE* buffer, int width, int height) {
        Paint_SelectImage(buffer);
        Paint_Clear(WHITE);

        // Draw your content
        Paint_DrawString_EN(10, 10, "My Plugin", &Font24, WHITE, BLACK);
    }

    void update() {
        // Fetch new data / update state
    }

    const char* getName() { return "My Plugin"; }
};
```

### Plugin Interface (Base Class)

```cpp
class Plugin {
public:
    virtual void init() = 0;
    virtual void render(UBYTE* buffer, int width, int height) = 0;
    virtual void update() = 0;
    virtual const char* getName() = 0;
};

// Now all plugins inherit from Plugin
class WeatherPlugin : public Plugin {
    // Implement interface methods
};
```

## More Plugin Ideas

### Easy to Implement
- [ ] Countdown timer to specific date
- [ ] Quote of the day (random from list)
- [ ] Simple calendar/agenda
- [ ] Step counter (if you add accelerometer)
- [ ] QR code generator (WiFi credentials, contact info)
- [ ] Binary clock
- [ ] Focus mode timer (like Pomodoro but customizable)

### Moderate Difficulty
- [ ] RSS feed reader
- [ ] GitHub contribution graph
- [ ] Spotify now playing
- [ ] Email unread count
- [ ] Google Calendar events
- [ ] Home Assistant integration
- [ ] Stock portfolio tracker

### Advanced
- [ ] Mini games (Snake, Pong on e-paper!)
- [ ] Photo frame (SD card or WiFi)
- [ ] Generative art
- [ ] Music visualizer
- [ ] Smart home dashboard

## Next Steps

1. **Test a simple plugin**: Start with clock_plugin.cpp
2. **Add WiFi plugin**: Try weather or Bitcoin ticker
3. **Build plugin manager**: Switch between plugins with buttons
4. **Optimize power**: Implement deep sleep
5. **Explore TRMNL**: Deploy server for access to 60+ plugins

## Resources

- Main guide: `../../PLUGIN_INTEGRATION_GUIDE.md`
- TRMNL integration: `../trmnl_integration/`
- InkyPi bridge: `../inkypi_bridge/`
- Your current Pomodoro: `../../src/pomodoro.cpp`

---

**Hardware**: ESP32-S3, Waveshare 4.2" (400×300)
**Inspiration**: TRMNL (usetrmnl) + InkyPi (fatihak)
