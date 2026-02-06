# Quick Start Guide: Adding Plugins to The Watcher

Get your first plugin running in 15 minutes!

## Choose Your Path

### Path A: Simple Local Plugin (No WiFi)
**Time**: 10 minutes
**Difficulty**: Easy ⭐
**Battery**: Excellent
**Example**: Add a clock display

### Path B: WiFi Plugin (Online Data)
**Time**: 20 minutes
**Difficulty**: Moderate ⭐⭐
**Battery**: Good (with deep sleep)
**Example**: Add weather display

### Path C: TRMNL Integration (60+ Plugins)
**Time**: 30 minutes
**Difficulty**: Moderate ⭐⭐
**Battery**: Good
**Example**: Full plugin ecosystem

## Path A: Add Local Clock Plugin

### Step 1: Create New PlatformIO Environment

Edit `platformio.ini`:

```ini
[env:clock]
build_src_filter = -<*> +<clock_demo.cpp>
```

### Step 2: Create Source File

Copy the clock plugin:
```bash
cp assets/plugin_examples/local_plugins/clock_plugin.cpp src/clock_demo.cpp
```

Or create `src/clock_demo.cpp`:

```cpp
#include <Arduino.h>
#include "EPD.h"
#include "GUI_Paint.h"

// Button pins from your existing setup
#define BTN_LEFT 35
#define BTN_MID 36
#define BTN_RIGHT 37

UBYTE *frameBuffer;
bool use24Hour = true;
int hour = 12, minute = 0;

void setup() {
    Serial.begin(115200);

    // Initialize buttons
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_MID, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);

    // Initialize display
    DEV_Module_Init();
    EPD_4IN2_V2_Init();

    // Allocate buffer
    frameBuffer = (UBYTE*)malloc(15000);
    Paint_NewImage(frameBuffer, 400, 300, 0, WHITE);

    Serial.println("Clock ready!");
}

void updateDisplay() {
    Paint_SelectImage(frameBuffer);
    Paint_Clear(WHITE);

    // Format time string
    char timeStr[20];
    if (use24Hour) {
        sprintf(timeStr, "%02d:%02d", hour, minute);
    } else {
        int h12 = hour % 12;
        if (h12 == 0) h12 = 12;
        sprintf(timeStr, "%2d:%02d %s", h12, minute,
                hour >= 12 ? "PM" : "AM");
    }

    // Center the time
    int textWidth = strlen(timeStr) * 24;
    int x = (400 - textWidth) / 2;
    int y = 150 - 12;

    Paint_DrawString_EN(x, y, timeStr, &Font24, WHITE, BLACK);

    // Show format at bottom
    Paint_DrawString_EN(10, 280,
                       use24Hour ? "24-hour" : "12-hour",
                       &Font12, WHITE, BLACK);

    // Display
    EPD_4IN2_V2_Display(frameBuffer);
}

void loop() {
    // Left button: Increment hour
    if (digitalRead(BTN_LEFT) == LOW) {
        hour = (hour + 1) % 24;
        updateDisplay();
        delay(200);
    }

    // Middle button: Toggle 12/24 hour format
    if (digitalRead(BTN_MID) == LOW) {
        use24Hour = !use24Hour;
        updateDisplay();
        delay(200);
    }

    // Right button: Increment minute
    if (digitalRead(BTN_RIGHT) == LOW) {
        minute = (minute + 1) % 60;
        updateDisplay();
        delay(200);
    }

    delay(10);
}
```

### Step 3: Build and Upload

```bash
pio run -e clock -t upload
```

**Done!** You now have a working clock display.

### Enhancement: Auto-increment Time

Add to loop():
```cpp
static unsigned long lastUpdate = 0;

if (millis() - lastUpdate > 60000) {  // Every minute
    minute++;
    if (minute >= 60) {
        minute = 0;
        hour = (hour + 1) % 24;
    }
    updateDisplay();
    lastUpdate = millis();
}
```

## Path B: Add WiFi Weather Plugin

### Step 1: Get API Key

1. Go to https://openweathermap.org/api
2. Sign up (free tier: 1000 calls/day)
3. Get API key

### Step 2: Create Environment

`platformio.ini`:
```ini
[env:weather]
build_src_filter = -<*> +<weather_demo.cpp>
lib_deps =
    bblanchon/ArduinoJson@^6.21.0
```

### Step 3: Create Source

`src/weather_demo.cpp`:
```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "EPD.h"
#include "GUI_Paint.h"

// Configuration
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";
const char* API_KEY = "YOUR_OPENWEATHER_API_KEY";
const char* CITY = "London";

UBYTE *frameBuffer;
float temperature = 0;
String description = "Loading...";

void connectWiFi() {
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected!");
    } else {
        Serial.println("\nFailed to connect");
    }
}

bool fetchWeather() {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=";
    url += CITY;
    url += "&appid=";
    url += API_KEY;
    url += "&units=metric";

    http.begin(url);
    int code = http.GET();

    if (code == 200) {
        String payload = http.getString();

        StaticJsonDocument<1024> doc;
        deserializeJson(doc, payload);

        temperature = doc["main"]["temp"];
        description = doc["weather"][0]["description"].as<String>();

        http.end();
        Serial.println("Weather updated!");
        return true;
    } else {
        Serial.printf("HTTP Error: %d\n", code);
        http.end();
        return false;
    }
}

void updateDisplay() {
    Paint_SelectImage(frameBuffer);
    Paint_Clear(WHITE);

    // City name
    Paint_DrawString_EN(10, 10, CITY, &Font20, WHITE, BLACK);

    // Temperature
    char tempStr[20];
    sprintf(tempStr, "%.1f C", temperature);
    Paint_DrawString_EN(10, 50, tempStr, &Font24, WHITE, BLACK);

    // Description
    Paint_DrawString_EN(10, 90, description.c_str(), &Font16, WHITE, BLACK);

    // WiFi status
    if (WiFi.status() == WL_CONNECTED) {
        Paint_DrawString_EN(10, 280, "WiFi: Connected",
                          &Font12, WHITE, BLACK);
    } else {
        Paint_DrawString_EN(10, 280, "WiFi: Disconnected",
                          &Font12, WHITE, BLACK);
    }

    EPD_4IN2_V2_Display(frameBuffer);
}

void setup() {
    Serial.begin(115200);

    // Initialize display
    DEV_Module_Init();
    EPD_4IN2_V2_Init();

    frameBuffer = (UBYTE*)malloc(15000);
    Paint_NewImage(frameBuffer, 400, 300, 0, WHITE);

    // Connect and fetch
    connectWiFi();
    fetchWeather();
    updateDisplay();
}

void loop() {
    // Update every 30 minutes
    delay(30 * 60 * 1000);

    connectWiFi();
    fetchWeather();
    updateDisplay();

    // Optional: Disconnect to save power
    WiFi.disconnect();
}
```

### Step 4: Configure and Upload

1. Edit `WIFI_SSID`, `WIFI_PASSWORD`, `API_KEY`, `CITY`
2. Build: `pio run -e weather -t upload`

**Done!** Weather updates every 30 minutes.

### Power Optimization

Replace loop() with deep sleep:
```cpp
void loop() {
    Serial.println("Going to sleep for 30 min...");

    WiFi.disconnect();
    esp_sleep_enable_timer_wakeup(30 * 60 * 1000000ULL);
    esp_deep_sleep_start();
}
```

## Path C: TRMNL Integration

### Step 1: Deploy TRMNL Server

**Option 1: Python (Easiest)**
```bash
# On your PC or Raspberry Pi
git clone https://github.com/usetrmnl/trmnl-fastapi
cd trmnl-fastapi
pip install -r requirements.txt
python main.py
```

Server runs at `http://localhost:8000`

### Step 2: Flash TRMNL Client

Copy the client code:
```bash
cp assets/trmnl_integration/trmnl_client.cpp src/trmnl_demo.cpp
```

Add to `platformio.ini`:
```ini
[env:trmnl]
build_src_filter = -<*> +<trmnl_demo.cpp>
lib_deps =
    bblanchon/ArduinoJson@^6.21.0
```

Edit `src/trmnl_demo.cpp`:
```cpp
const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASSWORD = "your_password";
const char* TRMNL_SERVER = "http://192.168.1.100:8000";
```

### Step 3: Upload and Run

```bash
pio run -e trmnl -t upload
```

Check serial monitor for device registration:
```
WiFi connected
TRMNL setup complete!
Friendly ID: watcher-abcd
Display updated!
Going to sleep...
```

### Step 4: Configure Plugins

1. Open browser: `http://localhost:8000`
2. Find your device (watcher-abcd)
3. Enable plugins (Weather, Clock, Bitcoin, etc.)
4. Set refresh interval (15 min recommended)

**Done!** You now have access to 60+ plugins!

## Combining with Existing Pomodoro

Add plugin mode to your Pomodoro timer:

```cpp
enum Mode { POMODORO, CLOCK, WEATHER };
Mode currentMode = POMODORO;

void loop() {
    // Middle button cycles modes
    if (digitalRead(BTN_MID) == LOW) {
        currentMode = (Mode)((currentMode + 1) % 3);
        delay(200);
    }

    switch(currentMode) {
        case POMODORO:
            runPomodoroLogic();
            break;
        case CLOCK:
            renderClock();
            break;
        case WEATHER:
            renderWeather();
            break;
    }
}
```

## Troubleshooting

### Display Shows Nothing

```cpp
// Check initialization
Serial.println("Display init...");
DEV_Module_Init();
EPD_4IN2_V2_Init();
Serial.println("Display ready");

// Verify buffer
if (frameBuffer == NULL) {
    Serial.println("Buffer allocation failed!");
}
```

### WiFi Won't Connect

```cpp
// Print diagnostics
Serial.println(WIFI_SSID);
Serial.print("MAC: ");
Serial.println(WiFi.macAddress());

// Try static IP if DHCP fails
WiFi.config(IPAddress(192,168,1,100),
           IPAddress(192,168,1,1),
           IPAddress(255,255,255,0));
```

### JSON Parsing Fails

```cpp
// Increase buffer size
StaticJsonDocument<2048> doc;  // Was 1024

// Check for errors
DeserializationError error = deserializeJson(doc, payload);
if (error) {
    Serial.print("JSON error: ");
    Serial.println(error.c_str());
}
```

## Next Steps

1. ✅ **Test a simple plugin** (clock or weather)
2. **Add plugin switching** to your Pomodoro timer
3. **Try TRMNL** for instant access to many plugins
4. **Create custom plugin** based on templates
5. **Optimize battery life** with deep sleep

## File Reference

```
assets/
├── plugin_examples/
│   ├── README.md                    ← Plugin catalog
│   ├── local_plugins/
│   │   ├── clock_plugin.cpp         ← Start here
│   │   └── habit_tracker_plugin.cpp
│   ├── wifi_plugins/
│   │   ├── weather_plugin.cpp       ← Or here
│   │   └── bitcoin_ticker_plugin.cpp
│   └── hybrid_plugins/
│       └── cached_weather_plugin.cpp
├── trmnl_integration/
│   ├── README.md                    ← TRMNL guide
│   └── trmnl_client.cpp             ← Ready to use
└── inkypi_bridge/
    └── README.md                    ← InkyPi porting guide
```

## Getting Help

- Main guide: `PLUGIN_INTEGRATION_GUIDE.md`
- TRMNL docs: https://docs.trmnl.com
- InkyPi wiki: https://github.com/fatihak/InkyPi/wiki
- Your hardware: `README.md`

---

**Pick a path and start coding!** 🚀
