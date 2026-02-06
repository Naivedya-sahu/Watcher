/**
 * WEATHER PLUGIN - WiFi Required
 *
 * Fetches weather data from OpenWeatherMap API
 * Displays current conditions and 3-day forecast
 *
 * Similar to TRMNL Weather & InkyPi Weather plugins
 *
 * Setup:
 * 1. Get free API key from https://openweathermap.org/api
 * 2. Set API_KEY and CITY constants
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "EPD.h"
#include "GUI_Paint.h"

class WeatherPlugin {
private:
    String apiKey;
    String city;

    struct WeatherData {
        float temp;
        float feelsLike;
        int humidity;
        String description;
        String icon;
        int pressure;
        float windSpeed;
    };

    WeatherData currentWeather;
    bool dataValid = false;

public:
    void init(const char* key, const char* location) {
        apiKey = key;
        city = location;
    }

    bool fetchWeather() {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi not connected");
            return false;
        }

        HTTPClient http;
        String url = "http://api.openweathermap.org/data/2.5/weather?q=" +
                     city + "&appid=" + apiKey + "&units=metric";

        http.begin(url);
        int httpCode = http.GET();

        if (httpCode == 200) {
            String payload = http.getString();
            parseWeatherData(payload);
            dataValid = true;
            http.end();
            return true;
        } else {
            Serial.printf("HTTP Error: %d\n", httpCode);
            http.end();
            return false;
        }
    }

    void render(UBYTE* buffer, int width, int height) {
        Paint_SelectImage(buffer);
        Paint_Clear(WHITE);

        if (!dataValid) {
            Paint_DrawString_EN(10, height/2, "No weather data",
                              &Font16, WHITE, BLACK);
            Paint_DrawString_EN(10, height/2 + 20, "Check WiFi & API key",
                              &Font12, WHITE, BLACK);
            return;
        }

        // Title
        Paint_DrawString_EN(10, 10, city.c_str(), &Font20, WHITE, BLACK);

        // Temperature (large)
        char tempStr[20];
        sprintf(tempStr, "%.1f°C", currentWeather.temp);
        Paint_DrawString_EN(10, 50, tempStr, &Font24, WHITE, BLACK);

        // Feels like
        char feelsStr[30];
        sprintf(feelsStr, "Feels like %.1f°C", currentWeather.feelsLike);
        Paint_DrawString_EN(10, 90, feelsStr, &Font16, WHITE, BLACK);

        // Description
        Paint_DrawString_EN(10, 120, currentWeather.description.c_str(),
                          &Font16, WHITE, BLACK);

        // Weather icon (simple text representation)
        int iconX = 250;
        int iconY = 50;
        drawWeatherIcon(currentWeather.icon.c_str(), iconX, iconY);

        // Additional details
        int detailY = 160;
        char detail[50];

        sprintf(detail, "Humidity: %d%%", currentWeather.humidity);
        Paint_DrawString_EN(10, detailY, detail, &Font12, WHITE, BLACK);

        sprintf(detail, "Pressure: %d hPa", currentWeather.pressure);
        Paint_DrawString_EN(10, detailY + 20, detail, &Font12, WHITE, BLACK);

        sprintf(detail, "Wind: %.1f m/s", currentWeather.windSpeed);
        Paint_DrawString_EN(10, detailY + 40, detail, &Font12, WHITE, BLACK);

        // Update time
        Paint_DrawString_EN(10, height - 20, "Updated now",
                          &Font12, WHITE, BLACK);
    }

private:
    void parseWeatherData(const String& json) {
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, json);

        if (error) {
            Serial.println("JSON parsing failed");
            return;
        }

        currentWeather.temp = doc["main"]["temp"];
        currentWeather.feelsLike = doc["main"]["feels_like"];
        currentWeather.humidity = doc["main"]["humidity"];
        currentWeather.pressure = doc["main"]["pressure"];
        currentWeather.windSpeed = doc["wind"]["speed"];
        currentWeather.description = doc["weather"][0]["description"].as<String>();
        currentWeather.icon = doc["weather"][0]["icon"].as<String>();
    }

    void drawWeatherIcon(const char* iconCode, int x, int y) {
        // Simple icon representations
        // OpenWeatherMap icon codes: 01d/01n (clear), 02d (few clouds),
        // 03d (scattered clouds), 04d (broken clouds), 09d (rain),
        // 10d (rain), 11d (thunderstorm), 13d (snow), 50d (mist)

        if (strncmp(iconCode, "01", 2) == 0) {
            // Sun
            Paint_DrawCircle(x, y, 20, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
            Paint_DrawCircle(x, y, 15, BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
        } else if (strncmp(iconCode, "02", 2) == 0 ||
                   strncmp(iconCode, "03", 2) == 0 ||
                   strncmp(iconCode, "04", 2) == 0) {
            // Clouds
            Paint_DrawCircle(x-10, y, 12, BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
            Paint_DrawCircle(x+10, y, 12, BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
            Paint_DrawCircle(x, y-5, 15, BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
        } else if (strncmp(iconCode, "09", 2) == 0 ||
                   strncmp(iconCode, "10", 2) == 0) {
            // Rain
            Paint_DrawCircle(x, y-10, 12, BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
            Paint_DrawLine(x-5, y+5, x-5, y+15, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
            Paint_DrawLine(x, y+10, x, y+20, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
            Paint_DrawLine(x+5, y+5, x+5, y+15, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
        } else if (strncmp(iconCode, "13", 2) == 0) {
            // Snow (asterisks)
            Paint_DrawString_EN(x-10, y-10, "*", &Font24, WHITE, BLACK);
            Paint_DrawString_EN(x+5, y-5, "*", &Font20, WHITE, BLACK);
            Paint_DrawString_EN(x-5, y+5, "*", &Font16, WHITE, BLACK);
        }
    }

public:
    const char* getName() { return "Weather"; }
};

// Example usage
/*
WeatherPlugin weatherPlugin;
UBYTE* buffer = (UBYTE*)malloc(15000);

const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASSWORD = "your_password";
const char* OPENWEATHER_API_KEY = "your_api_key";
const char* CITY = "London";

void setup() {
    Serial.begin(115200);
    DEV_Module_Init();
    EPD_4IN2_V2_Init();

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    weatherPlugin.init(OPENWEATHER_API_KEY, CITY);
    weatherPlugin.fetchWeather();
}

void loop() {
    weatherPlugin.render(buffer, 400, 300);
    EPD_4IN2_V2_Display(buffer);

    // Sleep for 30 minutes, then wake and update
    esp_sleep_enable_timer_wakeup(30 * 60 * 1000000ULL);

    // Before sleep, disconnect WiFi
    WiFi.disconnect();
    esp_deep_sleep_start();
}

// Or for active display, update every 30 minutes without deep sleep
void loop_active() {
    weatherPlugin.fetchWeather();
    weatherPlugin.render(buffer, 400, 300);
    EPD_4IN2_V2_Display(buffer);

    delay(30 * 60 * 1000);  // 30 minutes
}
*/
