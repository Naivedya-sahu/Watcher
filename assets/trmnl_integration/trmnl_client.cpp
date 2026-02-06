/**
 * TRMNL CLIENT FOR ESP32-S3
 *
 * Connects to TRMNL backend server and fetches pre-rendered BMP images
 * Compatible with official TRMNL firmware protocol
 *
 * This allows you to use ANY TRMNL plugin by running a TRMNL server
 * and having your ESP32 act as a TRMNL display device
 *
 * Server options:
 * - TRMNL Terminus (Ruby): https://github.com/usetrmnl/terminus
 * - TRMNL FastAPI (Python): https://github.com/usetrmnl/trmnl-fastapi
 * - TRMNL Node.js: https://github.com/usetrmnl/trmnl-node-lite
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "EPD.h"
#include "GUI_Paint.h"

class TRMNLClient {
private:
    Preferences prefs;
    String serverURL;
    String apiKey;
    String friendlyID;
    int refreshRate = 15;  // minutes
    bool isSetup = false;

    // Device info
    String macAddress;
    String firmwareVersion = "1.0.0-ESP32S3";
    int batteryVoltage = 3700;  // mV (if using battery)
    int rssi = 0;

public:
    void init(const char* server) {
        serverURL = server;

        // Get MAC address
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        char macStr[18];
        sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        macAddress = macStr;

        // Load saved credentials
        prefs.begin("trmnl", false);
        apiKey = prefs.getString("api_key", "");
        friendlyID = prefs.getString("friendly_id", "");
        refreshRate = prefs.getInt("refresh_rate", 15);

        isSetup = (apiKey.length() > 0);
    }

    bool setup() {
        // First-time setup - register with server
        HTTPClient http;
        String url = serverURL + "/api/setup";

        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("ID", macAddress);

        int httpCode = http.GET();

        if (httpCode == 200) {
            String response = http.getString();
            StaticJsonDocument<512> doc;
            deserializeJson(doc, response);

            apiKey = doc["api_key"].as<String>();
            friendlyID = doc["friendly_id"].as<String>();

            // Save credentials
            prefs.putString("api_key", apiKey);
            prefs.putString("friendly_id", friendlyID);

            isSetup = true;
            http.end();

            Serial.println("TRMNL setup complete!");
            Serial.println("Friendly ID: " + friendlyID);
            return true;
        } else {
            Serial.printf("Setup failed: HTTP %d\n", httpCode);
            http.end();
            return false;
        }
    }

    bool fetchAndDisplay(UBYTE* buffer) {
        if (!isSetup) {
            Serial.println("Not setup - call setup() first");
            return false;
        }

        HTTPClient http;
        String url = serverURL + "/api/display";

        http.begin(url);
        http.addHeader("Access-Token", apiKey);
        http.addHeader("Battery-Voltage", String(batteryVoltage));
        http.addHeader("FW-Version", firmwareVersion);
        http.addHeader("RSSI", String(rssi));
        http.addHeader("Refresh-Rate", String(refreshRate));

        int httpCode = http.GET();

        if (httpCode == 200) {
            String response = http.getString();
            StaticJsonDocument<1024> doc;
            deserializeJson(doc, response);

            // Check for firmware update
            if (doc.containsKey("firmware_url")) {
                Serial.println("Firmware update available!");
                // Handle OTA update here if desired
            }

            // Get image URL
            String imageURL = doc["image_url"].as<String>();

            http.end();

            // Fetch the BMP image
            return downloadBMPImage(imageURL, buffer);

        } else {
            Serial.printf("Fetch failed: HTTP %d\n", httpCode);
            http.end();
            return false;
        }
    }

private:
    bool downloadBMPImage(const String& url, UBYTE* buffer) {
        HTTPClient http;
        http.begin(url);

        int httpCode = http.GET();

        if (httpCode == 200) {
            int len = http.getSize();
            WiFiClient* stream = http.getStreamPtr();

            // BMP header parsing (simplified - assumes correct format)
            uint8_t header[54];
            stream->readBytes(header, 54);

            // Read pixel data
            // BMP is stored bottom-to-top, need to flip
            int width = 400;
            int height = 300;
            int rowSize = ((width + 31) / 32) * 4;  // BMP row alignment

            for (int y = height - 1; y >= 0; y--) {
                for (int x = 0; x < width; x++) {
                    uint8_t pixel = stream->read();

                    // Convert to 1-bit (0=black, 1=white)
                    int bufferIndex = (y * width + x) / 8;
                    int bitIndex = 7 - (x % 8);

                    if (pixel > 127) {  // White
                        buffer[bufferIndex] |= (1 << bitIndex);
                    } else {  // Black
                        buffer[bufferIndex] &= ~(1 << bitIndex);
                    }
                }
            }

            http.end();
            return true;
        } else {
            Serial.printf("Image download failed: HTTP %d\n", httpCode);
            http.end();
            return false;
        }
    }

public:
    void updateBatteryVoltage(int mV) {
        batteryVoltage = mV;
    }

    void updateRSSI() {
        rssi = WiFi.RSSI();
    }

    const char* getFriendlyID() {
        return friendlyID.c_str();
    }

    const char* getName() { return "TRMNL Client"; }
};

// Example usage
/*
TRMNLClient trmnl;
UBYTE* imageBuffer = (UBYTE*)malloc(15000);  // 400*300/8 = 15000 bytes

const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASSWORD = "your_password";
const char* TRMNL_SERVER = "http://your-trmnl-server.com";

void setup() {
    Serial.begin(115200);

    // Initialize display
    DEV_Module_Init();
    EPD_4IN2_V2_Init();

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    // Initialize TRMNL client
    trmnl.init(TRMNL_SERVER);

    // First-time setup (only needed once)
    if (!trmnl.setup()) {
        Serial.println("Setup failed!");
        return;
    }

    // Fetch and display
    trmnl.updateRSSI();
    if (trmnl.fetchAndDisplay(imageBuffer)) {
        EPD_4IN2_V2_Display(imageBuffer);
        Serial.println("Display updated!");
    }
}

void loop() {
    // Deep sleep for 15 minutes
    Serial.println("Going to sleep...");
    esp_sleep_enable_timer_wakeup(15 * 60 * 1000000ULL);

    WiFi.disconnect();
    esp_deep_sleep_start();
}

// Alternative: Active update loop
void loop_active() {
    trmnl.updateRSSI();
    if (trmnl.fetchAndDisplay(imageBuffer)) {
        EPD_4IN2_V2_Display(imageBuffer);
    }

    delay(15 * 60 * 1000);  // 15 minutes
}
*/

/**
 * SETUP INSTRUCTIONS:
 *
 * 1. Deploy a TRMNL server:
 *    - Python FastAPI: easiest for beginners
 *      git clone https://github.com/usetrmnl/trmnl-fastapi
 *      pip install -r requirements.txt
 *      python main.py
 *
 *    - Ruby Terminus: full-featured
 *      git clone https://github.com/usetrmnl/terminus
 *      bundle install
 *      rails server
 *
 * 2. Configure plugins on server dashboard
 *
 * 3. Flash this code to ESP32-S3 with server URL
 *
 * 4. ESP32 will register and start fetching plugin screens
 *
 * Now you have access to ALL TRMNL plugins:
 * - Weather, Calendar, GitHub, YouTube Analytics
 * - ChatGPT, Hacker News, Stock Prices
 * - And 40+ community plugins!
 */
