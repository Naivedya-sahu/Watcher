/**
 * CACHED WEATHER PLUGIN - Hybrid Offline/Online
 *
 * Works offline with cached data, updates when WiFi available
 * Best of both worlds: always displays something, fresh when possible
 *
 * Uses Preferences for persistent caching
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "EPD.h"
#include "GUI_Paint.h"

class CachedWeatherPlugin {
private:
    Preferences prefs;
    String apiKey;
    String city;

    struct WeatherCache {
        float temp;
        int humidity;
        char description[50];
        unsigned long lastUpdate;
        bool valid;
    };

    WeatherCache cache;
    unsigned long cacheExpiryMs = 30 * 60 * 1000;  // 30 minutes

public:
    void init(const char* key, const char* location) {
        apiKey = key;
        city = location;

        prefs.begin("weather", false);
        loadCache();
    }

    void update(bool forceOnline = false) {
        // Try online update if WiFi available or forced
        if ((WiFi.status() == WL_CONNECTED) || forceOnline) {
            if (fetchWeatherOnline()) {
                saveCache();
                return;
            }
        }

        // If online failed or WiFi unavailable, use cache
        Serial.println("Using cached weather data");
    }

    void render(UBYTE* buffer, int width, int height) {
        Paint_SelectImage(buffer);
        Paint_Clear(WHITE);

        if (!cache.valid) {
            Paint_DrawString_EN(10, height/2, "No weather data",
                              &Font16, WHITE, BLACK);
            Paint_DrawString_EN(10, height/2 + 20,
                              "Enable WiFi to fetch",
                              &Font12, WHITE, BLACK);
            return;
        }

        // Check if cache is stale
        unsigned long age = millis() - cache.lastUpdate;
        bool isStale = age > cacheExpiryMs;

        // City name
        Paint_DrawString_EN(10, 10, city.c_str(), &Font20, WHITE, BLACK);

        // Temperature
        char tempStr[20];
        sprintf(tempStr, "%.1f°C", cache.temp);
        Paint_DrawString_EN(10, 50, tempStr, &Font24, WHITE, BLACK);

        // Description
        Paint_DrawString_EN(10, 90, cache.description, &Font16, WHITE, BLACK);

        // Humidity
        char humStr[30];
        sprintf(humStr, "Humidity: %d%%", cache.humidity);
        Paint_DrawString_EN(10, 120, humStr, &Font12, WHITE, BLACK);

        // Cache status
        char statusStr[50];
        if (isStale) {
            int minutes = age / 60000;
            sprintf(statusStr, "Updated %d min ago (stale)", minutes);
            Paint_DrawString_EN(10, height - 40, statusStr,
                              &Font12, WHITE, BLACK);
            Paint_DrawString_EN(10, height - 20,
                              "Connect WiFi to refresh",
                              &Font12, WHITE, BLACK);
        } else {
            int minutes = age / 60000;
            sprintf(statusStr, "Updated %d min ago", minutes);
            Paint_DrawString_EN(10, height - 20, statusStr,
                              &Font12, WHITE, BLACK);
        }

        // WiFi status indicator
        if (WiFi.status() == WL_CONNECTED) {
            Paint_DrawString_EN(width - 100, 10, "WiFi: ON",
                              &Font12, WHITE, BLACK);
        } else {
            Paint_DrawString_EN(width - 100, 10, "WiFi: OFF",
                              &Font12, WHITE, BLACK);
        }
    }

private:
    bool fetchWeatherOnline() {
        HTTPClient http;
        String url = "http://api.openweathermap.org/data/2.5/weather?q=" +
                     city + "&appid=" + apiKey + "&units=metric";

        http.begin(url);
        int httpCode = http.GET();

        if (httpCode == 200) {
            String payload = http.getString();
            parseWeatherData(payload);
            cache.lastUpdate = millis();
            cache.valid = true;
            http.end();
            Serial.println("Weather updated from API");
            return true;
        } else {
            Serial.printf("API fetch failed: HTTP %d\n", httpCode);
            http.end();
            return false;
        }
    }

    void parseWeatherData(const String& json) {
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, json);

        if (!error) {
            cache.temp = doc["main"]["temp"];
            cache.humidity = doc["main"]["humidity"];
            String desc = doc["weather"][0]["description"].as<String>();
            strncpy(cache.description, desc.c_str(), 49);
            cache.description[49] = '\0';
        }
    }

    void saveCache() {
        prefs.putFloat("temp", cache.temp);
        prefs.putInt("humidity", cache.humidity);
        prefs.putString("desc", cache.description);
        prefs.putULong("updated", cache.lastUpdate);
        prefs.putBool("valid", cache.valid);
    }

    void loadCache() {
        cache.temp = prefs.getFloat("temp", 0);
        cache.humidity = prefs.getInt("humidity", 0);
        String desc = prefs.getString("desc", "");
        strncpy(cache.description, desc.c_str(), 49);
        cache.description[49] = '\0';
        cache.lastUpdate = prefs.getULong("updated", 0);
        cache.valid = prefs.getBool("valid", false);
    }

public:
    bool isCacheStale() {
        unsigned long age = millis() - cache.lastUpdate;
        return age > cacheExpiryMs;
    }

    void setCacheExpiry(unsigned long minutes) {
        cacheExpiryMs = minutes * 60 * 1000;
    }

    const char* getName() { return "Cached Weather"; }
};

// Example usage with power-efficient strategy
/*
CachedWeatherPlugin weather;
UBYTE* buffer = (UBYTE*)malloc(15000);

RTC_DATA_ATTR int bootCount = 0;

void setup() {
    Serial.begin(115200);
    bootCount++;

    DEV_Module_Init();
    EPD_4IN2_V2_Init();

    weather.init("your_api_key", "London");

    // Strategy: Only connect WiFi every 3rd boot to save power
    if (bootCount % 3 == 0 || weather.isCacheStale()) {
        Serial.println("Connecting to WiFi for update...");
        WiFi.begin("ssid", "password");

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            weather.update(true);
            WiFi.disconnect();
        } else {
            Serial.println("WiFi failed, using cache");
        }
    }

    // Always update display (uses cache if offline)
    weather.update(false);
    weather.render(buffer, 400, 300);
    EPD_4IN2_V2_Display(buffer);

    // Deep sleep for 10 minutes
    esp_sleep_enable_timer_wakeup(10 * 60 * 1000000ULL);
    esp_deep_sleep_start();
}

void loop() {
    // Never reached due to deep sleep
}

// Alternative: Manual WiFi control with button
void setupWithButton() {
    pinMode(36, INPUT_PULLUP);  // Middle button

    // Hold button to force WiFi update
    if (digitalRead(36) == LOW) {
        Serial.println("Button pressed - forcing WiFi update");
        WiFi.begin("ssid", "password");
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
        }
        weather.update(true);
        WiFi.disconnect();
    } else {
        // Normal operation - use cache
        weather.update(false);
    }

    weather.render(buffer, 400, 300);
    EPD_4IN2_V2_Display(buffer);
}
*/
