# InkyPi Bridge for ESP32-S3

Adapt InkyPi plugins for use with your ESP32-S3 display.

## What is InkyPi?

InkyPi is a Raspberry Pi-based e-ink display system with:
- **Python/Flask web interface**
- **Direct plugin execution** on Raspberry Pi
- **Plugin types**: Image, Clock, Weather, Calendar, AI, News
- **Scheduled playlists** for automated content rotation

## Why Bridge to ESP32?

InkyPi plugins run on Raspberry Pi (Python), but you can:
1. **Use InkyPi as a server** - Keep Pi running, ESP32 fetches images
2. **Port plugins to C++** - Rewrite logic for native ESP32
3. **Hybrid approach** - Simple plugins on ESP32, complex ones on Pi

## Architecture Options

### Option 1: InkyPi as Image Server

```
┌─────────────────┐         ┌──────────────────┐         ┌─────────────────┐
│  Raspberry Pi   │         │   ESP32-S3       │         │  E-Paper 4.2"   │
│  (InkyPi)       │ ────────▶│  (Fetches BMP)   │ ────────▶│  400×300        │
└─────────────────┘  WiFi   └──────────────────┘   SPI   └─────────────────┘
        │
        ├─ InkyPi plugins run here
        ├─ Generates images
        └─ Serves via HTTP
```

**Implementation:**
- Modify InkyPi to export images instead of direct display
- ESP32 fetches via HTTP
- Best for complex plugins (AI, web scraping)

### Option 2: Direct Plugin Porting

```
┌──────────────────┐         ┌─────────────────┐
│   ESP32-S3       │         │  E-Paper 4.2"   │
│  (Native Plugin) │ ────────▶│  400×300        │
└──────────────────┘   SPI   └─────────────────┘
        │
        └─ Ported InkyPi logic in C++
```

**Implementation:**
- Analyze InkyPi plugin Python code
- Rewrite in C++ for ESP32
- Best for simple plugins (clock, weather)

## InkyPi Plugin Analysis

### 1. Clock Plugin

**InkyPi (Python):**
```python
# plugins/clock.py
from PIL import Image, ImageDraw, ImageFont
import time

def generate(config):
    img = Image.new('RGB', (400, 300), 'white')
    draw = ImageDraw.Draw(img)

    current_time = time.strftime(config.get('format', '%H:%M'))
    font = ImageFont.truetype('font.ttf', 48)

    draw.text((100, 100), current_time, fill='black', font=font)
    return img
```

**ESP32 Port (C++):**
```cpp
// See: ../plugin_examples/local_plugins/clock_plugin.cpp
void ClockPlugin::render(UBYTE* buffer, int width, int height) {
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d", hour, minute);
    Paint_DrawString_EN(100, 100, timeStr, &Font24, WHITE, BLACK);
}
```

**Difficulty**: ⭐ Easy - Direct translation

### 2. Weather Plugin

**InkyPi (Python):**
```python
# plugins/weather.py
import requests

def generate(config):
    api_key = config['api_key']
    city = config['city']

    response = requests.get(f'http://api.openweathermap.org/...')
    data = response.json()

    # Draw weather info
    draw.text((10, 10), f"{data['temp']}°C")
    # ... more rendering
```

**ESP32 Port (C++):**
```cpp
// See: ../plugin_examples/wifi_plugins/weather_plugin.cpp
HTTPClient http;
http.begin(apiUrl);
String payload = http.getString();
// Parse JSON with ArduinoJson
// Render with Paint_DrawString_EN
```

**Difficulty**: ⭐⭐ Moderate - HTTP + JSON parsing

### 3. AI Generation Plugin

**InkyPi (Python):**
```python
# plugins/ai_generation.py
import openai

def generate(config):
    response = openai.Image.create(
        prompt=config['prompt'],
        size="512x512"
    )

    # Download image
    # Resize to 400×300
    # Convert to B/W
    return processed_image
```

**ESP32 Port:**
❌ **Not Recommended** - Too complex, memory intensive

**Alternative**: Use InkyPi as server, ESP32 fetches result

**Difficulty**: ⭐⭐⭐⭐⭐ Very Hard - Better on server

## Setting Up InkyPi as Image Server

### Step 1: Install InkyPi on Raspberry Pi

```bash
# On Raspberry Pi
git clone https://github.com/fatihak/InkyPi
cd InkyPi
sudo bash install/install.sh
```

### Step 2: Modify InkyPi to Export Images

Create a new endpoint to serve images:

```python
# In InkyPi Flask app
from flask import send_file
import io

@app.route('/api/get_image')
def get_image():
    # Get current active plugin
    plugin = get_active_plugin()

    # Generate image (InkyPi already does this)
    image = plugin.generate(config)

    # Convert to BMP format
    img_io = io.BytesIO()
    image.save(img_io, 'BMP')
    img_io.seek(0)

    return send_file(img_io, mimetype='image/bmp')
```

### Step 3: ESP32 Client

```cpp
#include <WiFi.h>
#include <HTTPClient.h>

void fetchInkyPiImage() {
    HTTPClient http;
    http.begin("http://raspberrypi.local:5000/api/get_image");

    int httpCode = http.GET();
    if (httpCode == 200) {
        // Download BMP
        WiFiClient* stream = http.getStreamPtr();
        // ... parse BMP and display
    }

    http.end();
}
```

## Plugin Porting Guide

### Checklist for Porting

- [ ] **Analyze Python code** - Understand what it does
- [ ] **Identify APIs** - What external services are called?
- [ ] **Check memory** - Will it fit in ESP32 RAM?
- [ ] **Find C++ equivalents** - HTTPClient, ArduinoJson, etc.
- [ ] **Test incrementally** - Get API working first, then rendering

### Python → C++ Library Mapping

| Python Library | C++ Equivalent | Notes |
|----------------|----------------|-------|
| `requests` | `HTTPClient.h` | HTTP requests |
| `json` | `ArduinoJson.h` | JSON parsing |
| `PIL` / `ImageDraw` | `GUI_Paint.h` | Drawing functions |
| `datetime` | `time.h` | Time handling |
| `os.path` | `FS.h` / `SPIFFS.h` | File system |
| `sqlite3` | `Preferences.h` | Persistent storage |

### Common Porting Patterns

**Pattern 1: HTTP API Call**

```python
# Python
response = requests.get(url, headers={'API-Key': key})
data = response.json()
```

```cpp
// C++
HTTPClient http;
http.begin(url);
http.addHeader("API-Key", key);
int code = http.GET();
String payload = http.getString();

StaticJsonDocument<1024> doc;
deserializeJson(doc, payload);
```

**Pattern 2: Drawing Text**

```python
# Python
draw.text((x, y), text, fill='black', font=font)
```

```cpp
// C++
Paint_DrawString_EN(x, y, text, &Font16, WHITE, BLACK);
```

**Pattern 3: Configuration Storage**

```python
# Python
config = json.load(open('config.json'))
```

```cpp
// C++
Preferences prefs;
prefs.begin("config", false);
String value = prefs.getString("key", "default");
```

## InkyPi Plugins Worth Porting

### ✅ Easy Ports (Do These First)

1. **Clock** - Just time display
2. **Simple Image** - Load from SPIFFS
3. **Quote Display** - Rotate through predefined quotes

### ⚠️ Moderate Ports (Require WiFi)

1. **Weather** - API call + JSON parsing
2. **Calendar** - OAuth is tricky but possible
3. **News Headlines** - RSS parsing

### ❌ Keep on Server (Too Complex)

1. **AI Generation** - OpenAI API + image processing
2. **Newspaper Comics** - Web scraping + OCR
3. **Complex Charts** - Matplotlib equivalent difficult

## Example: Porting InkyPi Weather

### Original InkyPi Code

```python
# InkyPi weather plugin (simplified)
import requests
from PIL import Image, ImageDraw, ImageFont

def generate_weather(config):
    # Fetch weather
    url = f"http://api.openweathermap.org/data/2.5/weather"
    params = {'q': config['city'], 'appid': config['api_key']}
    data = requests.get(url, params=params).json()

    # Create image
    img = Image.new('1', (400, 300), 255)  # 1-bit image
    draw = ImageDraw.Draw(img)

    # Draw temperature
    temp = data['main']['temp']
    draw.text((10, 50), f"{temp}°C", fill=0)

    return img
```

### Ported ESP32 Code

Already done! See:
```
../plugin_examples/wifi_plugins/weather_plugin.cpp
```

Key differences:
- `requests.get()` → `HTTPClient.GET()`
- `json()` → `ArduinoJson`
- `ImageDraw.text()` → `Paint_DrawString_EN()`

## Combining InkyPi + TRMNL Approaches

Best of both worlds:

```cpp
// Use TRMNL for rich plugin ecosystem
// Use InkyPi for custom Python plugins
// Use native ESP32 for offline plugins

enum PluginSource { LOCAL, TRMNL, INKYPI };

void updateDisplay(PluginSource source) {
    switch(source) {
        case LOCAL:
            clockPlugin.render(buffer, 400, 300);
            break;

        case TRMNL:
            trmnlClient.fetchAndDisplay(buffer);
            break;

        case INKYPI:
            fetchInkyPiImage(buffer);
            break;
    }

    EPD_4IN2_V2_Display(buffer);
}
```

## Resources

- InkyPi Repository: https://github.com/fatihak/InkyPi
- InkyPi Wiki: https://github.com/fatihak/InkyPi/wiki
- Plugin examples: `../plugin_examples/`
- Weather port: `../plugin_examples/wifi_plugins/weather_plugin.cpp`

## Next Steps

1. **Install InkyPi** on Raspberry Pi (optional)
2. **Try direct ports** - Start with clock plugin
3. **Use Pi as server** - For complex plugins
4. **Mix approaches** - Local + Remote plugins

---

**InkyPi**: Raspberry Pi + Python + Pimoroni Inky
**Your Setup**: ESP32-S3 + C++ + Waveshare
**Bridge Strategy**: Port simple plugins, proxy complex ones
