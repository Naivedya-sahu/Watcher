# TRMNL Integration for ESP32-S3

Turn your ESP32-S3 e-paper display into a TRMNL-compatible device and access 60+ plugins instantly!

## What is TRMNL?

TRMNL is an e-ink display platform with:
- **ESP32-based firmware** (similar to your hardware!)
- **Server-side plugin system** (20+ native, 40+ community plugins)
- **Multiple backend implementations** (Python, Ruby, Node.js, PHP, etc.)
- **BMP image rendering** (server generates, device displays)

## Architecture

```
┌─────────────────┐         ┌──────────────────┐         ┌─────────────────┐
│  TRMNL Server   │         │   ESP32-S3       │         │  E-Paper 4.2"   │
│  (Your PC/Pi)   │ ────────▶│  (The Watcher)   │ ────────▶│  400×300 B/W    │
└─────────────────┘  WiFi   └──────────────────┘   SPI   └─────────────────┘
        │
        ├─ Weather Plugin
        ├─ Calendar Plugin
        ├─ GitHub Plugin
        ├─ Bitcoin Plugin
        └─ 50+ more...
```

## Quick Start

### Step 1: Deploy TRMNL Server

Choose a backend implementation:

#### Option A: Python FastAPI (Recommended for Beginners)

```bash
# Clone the FastAPI implementation
git clone https://github.com/usetrmnl/trmnl-fastapi
cd trmnl-fastapi

# Install dependencies
pip install -r requirements.txt

# Run server
python main.py
```

Server runs at `http://localhost:8000`

#### Option B: Node.js Lite

```bash
git clone https://github.com/usetrmnl/trmnl-node-lite
cd trmnl-node-lite
npm install
npm start
```

#### Option C: Ruby Terminus (Full-Featured)

```bash
git clone https://github.com/usetrmnl/terminus
cd terminus
bundle install
rails db:migrate
rails server
```

### Step 2: Flash ESP32 Client

1. Copy `trmnl_client.cpp` to your project
2. Update configuration:

```cpp
const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASSWORD = "your_password";
const char* TRMNL_SERVER = "http://192.168.1.100:8000";  // Your server IP
```

3. Build and upload:

```bash
pio run -e trmnl -t upload
```

### Step 3: Configure Plugins

1. Access TRMNL dashboard: `http://localhost:8000`
2. Add your ESP32 device (auto-registered on first boot)
3. Enable plugins (Weather, Calendar, etc.)
4. Set refresh intervals
5. Watch your display update!

## Available Plugins

### Native Plugins (20+)

| Plugin | Description | API Required |
|--------|-------------|--------------|
| **Weather** | Current + forecast | OpenWeatherMap |
| **Calendar** | Google/Outlook events | OAuth |
| **GitHub** | Commit graph, stars | GitHub token |
| **ChatGPT** | AI-generated content | OpenAI |
| **Hacker News** | Top stories | None |
| **Stock Price** | Real-time quotes | Alpha Vantage |
| **Bitcoin** | Crypto prices | None (CoinGecko) |
| **Todoist** | Task list | Todoist |
| **YouTube** | Channel analytics | YouTube API |
| **Email Meter** | Inbox stats | Gmail API |
| **Shopify** | Store metrics | Shopify |
| **Lunar Calendar** | Moon phases | None |
| **Days Until** | Countdown timer | None |
| **Mondrian** | Generative art | None |
| **Notion** | Database views | Notion API |

### Community Plugins (40+)

- Strava fitness tracking
- Home Assistant integration
- Canvas LMS assignments
- Bluesky social feed
- Anki flashcards
- Spotify now playing
- Steam game stats
- ... and many more!

Full list: https://github.com/usetrmnl/plugins

## How It Works

### 1. Device Registration (First Boot)

```cpp
// ESP32 sends MAC address
GET /api/setup
Header: 'ID' => 'AA:BB:CC:DD:EE:FF'

// Server responds
{
  "api_key": "unique_device_key",
  "friendly_id": "watcher-1234",
  "image_url": "http://server/images/default.bmp"
}
```

### 2. Regular Updates

```cpp
// ESP32 requests new image
GET /api/display
Headers:
  'Access-Token': 'your_api_key'
  'Battery-Voltage': '3700'
  'FW-Version': '1.0.0'
  'RSSI': '-65'
  'Refresh-Rate': '15'

// Server responds with current plugin screen
{
  "image_url": "http://server/screens/weather_2024.bmp",
  "filename": "weather_2024.bmp",
  "firmware_url": null  // OTA update if available
}
```

### 3. Image Display

```cpp
// ESP32 downloads BMP image
GET /screens/weather_2024.bmp

// Parse BMP format
// Display on e-paper
// Go to deep sleep
```

## Power Management

TRMNL approach optimized for battery life:

```cpp
void loop() {
    // Wake up
    WiFi.begin(ssid, password);

    // Fetch new image (~10s awake)
    trmnl.updateRSSI();
    trmnl.fetchAndDisplay(buffer);
    EPD_4IN2_V2_Display(buffer);

    // Disconnect and sleep (15 min)
    WiFi.disconnect();
    esp_sleep_enable_timer_wakeup(15 * 60 * 1000000ULL);
    esp_deep_sleep_start();
}

// Result: ~100µA average, 120+ day battery life
```

## Configuration Options

### Refresh Rates

Common refresh intervals:
- **1 minute**: Clock, timer
- **5 minutes**: Stock prices, crypto
- **15 minutes**: Weather, calendar (default)
- **30 minutes**: News, social media
- **1 hour**: GitHub stats, analytics
- **4 hours**: Quotes, artwork

### Sleep Windows

Configure display to sleep during specific hours to save battery:

```cpp
// Example: Sleep from 11 PM to 7 AM
// 8-hour sleep window extends battery life from 140 to 210 days
```

## Creating TRMNL Plugins

If you want to create custom plugins for the server:

### Plugin Structure (Ruby Example)

```ruby
# plugins/custom_weather.rb
module Plugins
  class CustomWeather
    def initialize(plugin_setting)
      @api_key = plugin_setting.credentials['api_key']
      @city = plugin_setting.settings['city']
    end

    def locals
      weather_data = fetch_weather(@city, @api_key)

      {
        temperature: weather_data['temp'],
        condition: weather_data['description'],
        icon: weather_data['icon']
      }
    end

    private

    def fetch_weather(city, key)
      # Fetch from API
      # Return parsed data
    end
  end
end
```

### Template (ERB)

```erb
<!-- views/custom_weather.erb -->
<div class="weather-display">
  <h1><%= city %></h1>
  <div class="temp"><%= temperature %>°C</div>
  <div class="condition"><%= condition %></div>
</div>

<style>
  /* CSS for rendering to image */
</style>
```

Server converts this to 400×300 BMP image.

## Troubleshooting

### Device Not Registering

```cpp
// Check server URL
Serial.println(serverURL);

// Verify MAC address format
Serial.println(macAddress);  // Should be XX:XX:XX:XX:XX:XX

// Check HTTP response
Serial.println(httpCode);  // Should be 200
```

### Image Download Fails

```cpp
// Verify image URL
Serial.println(imageURL);

// Check available memory
Serial.printf("Free heap: %d\n", ESP.getFreeHeap());

// Ensure buffer size correct (15000 bytes for 400×300)
```

### Display Shows Garbage

- BMP format must be 1-bit (monochrome)
- Dimensions must match (400×300)
- Server should generate compatible format

### High Power Consumption

```cpp
// Make sure WiFi disconnects
WiFi.disconnect(true);
WiFi.mode(WIFI_OFF);

// Verify deep sleep is working
esp_sleep_enable_timer_wakeup(15 * 60 * 1000000ULL);
esp_deep_sleep_start();  // Should never return

// Check for stuck loops
```

## Comparison: TRMNL vs Direct Plugins

### TRMNL Approach ✅

**Pros:**
- Access to 60+ plugins instantly
- No firmware updates for new plugins
- Complex rendering on server (charts, images)
- Easy to create plugins (Ruby/Python/JS)
- Community plugins available

**Cons:**
- Requires server infrastructure
- WiFi always needed
- Slightly more power usage
- Depends on server uptime

### Direct ESP32 Plugins ✅

**Pros:**
- No server needed
- Can work offline
- Lower power consumption possible
- Full control over behavior

**Cons:**
- Limited by ESP32 memory/CPU
- Firmware updates for new plugins
- More complex plugin development
- No community plugin ecosystem

## Recommended Hybrid Approach

Use both!

```cpp
// Offline mode: Local plugins
if (WiFi.status() != WL_CONNECTED) {
    clockPlugin.render(buffer, 400, 300);
}

// Online mode: TRMNL plugins
else {
    trmnl.fetchAndDisplay(buffer);
}
```

## Next Steps

1. **Deploy a TRMNL server** (Python FastAPI recommended)
2. **Flash the TRMNL client** to your ESP32-S3
3. **Configure plugins** via dashboard
4. **Customize** appearance and refresh rates
5. **Create custom plugins** for your needs

## Resources

- TRMNL Documentation: https://docs.trmnl.com
- TRMNL Firmware: https://github.com/usetrmnl/firmware
- TRMNL Plugins: https://github.com/usetrmnl/plugins
- Community Discord: (link in TRMNL dashboard)
- Client code: `trmnl_client.cpp`

---

**Your Hardware**: ESP32-S3-DevKitC-1, Waveshare 4.2" V2
**Compatibility**: 100% - Same architecture as official TRMNL devices!
