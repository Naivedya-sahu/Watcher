/**
 * BITCOIN TICKER PLUGIN - WiFi Required
 *
 * Displays Bitcoin (and other crypto) prices with 24h change
 * Uses CoinGecko free API (no key required)
 *
 * Similar to TRMNL Stock Price plugin
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "EPD.h"
#include "GUI_Paint.h"

class BitcoinTickerPlugin {
private:
    struct CryptoData {
        String symbol;
        String name;
        float price;
        float change24h;
        float marketCap;
        float volume24h;
    };

    CryptoData crypto;
    bool dataValid = false;
    String cryptoId = "bitcoin";  // Can be: ethereum, cardano, etc.
    String currency = "usd";

public:
    void init(const char* id = "bitcoin", const char* curr = "usd") {
        cryptoId = id;
        currency = curr;
    }

    bool fetchPrice() {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi not connected");
            return false;
        }

        HTTPClient http;
        // CoinGecko API - free, no key required
        String url = "https://api.coingecko.com/api/v3/simple/price?ids=" +
                     cryptoId +
                     "&vs_currencies=" + currency +
                     "&include_24hr_change=true&include_market_cap=true&include_24hr_vol=true";

        http.begin(url);
        http.addHeader("Accept", "application/json");
        int httpCode = http.GET();

        if (httpCode == 200) {
            String payload = http.getString();
            parsePriceData(payload);
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
            Paint_DrawString_EN(10, height/2, "No price data",
                              &Font16, WHITE, BLACK);
            return;
        }

        // Title
        String title = cryptoId;
        title.toUpperCase();
        Paint_DrawString_EN(10, 10, title.c_str(), &Font24, WHITE, BLACK);

        // Current price (large)
        char priceStr[50];
        if (crypto.price >= 1000) {
            sprintf(priceStr, "$%.2f", crypto.price);
        } else if (crypto.price >= 1) {
            sprintf(priceStr, "$%.4f", crypto.price);
        } else {
            sprintf(priceStr, "$%.6f", crypto.price);
        }
        Paint_DrawString_EN(10, 50, priceStr, &Font24, WHITE, BLACK);

        // 24h change with arrow
        int changeY = 95;
        char changeStr[30];
        sprintf(changeStr, "24h: %.2f%%", crypto.change24h);

        // Draw arrow based on change
        if (crypto.change24h > 0) {
            // Up arrow
            Paint_DrawString_EN(10, changeY, "^", &Font20, WHITE, BLACK);
            Paint_DrawString_EN(30, changeY, changeStr, &Font16, WHITE, BLACK);
        } else if (crypto.change24h < 0) {
            // Down arrow
            Paint_DrawString_EN(10, changeY, "v", &Font20, WHITE, BLACK);
            Paint_DrawString_EN(30, changeY, changeStr, &Font16, WHITE, BLACK);
        } else {
            Paint_DrawString_EN(10, changeY, changeStr, &Font16, WHITE, BLACK);
        }

        // Market cap
        char mcapStr[50];
        if (crypto.marketCap >= 1e9) {
            sprintf(mcapStr, "Market Cap: $%.2fB", crypto.marketCap / 1e9);
        } else if (crypto.marketCap >= 1e6) {
            sprintf(mcapStr, "Market Cap: $%.2fM", crypto.marketCap / 1e6);
        } else {
            sprintf(mcapStr, "Market Cap: $%.0f", crypto.marketCap);
        }
        Paint_DrawString_EN(10, 130, mcapStr, &Font12, WHITE, BLACK);

        // 24h volume
        char volStr[50];
        if (crypto.volume24h >= 1e9) {
            sprintf(volStr, "24h Volume: $%.2fB", crypto.volume24h / 1e9);
        } else if (crypto.volume24h >= 1e6) {
            sprintf(volStr, "24h Volume: $%.2fM", crypto.volume24h / 1e6);
        } else {
            sprintf(volStr, "24h Volume: $%.0f", crypto.volume24h);
        }
        Paint_DrawString_EN(10, 150, volStr, &Font12, WHITE, BLACK);

        // Simple price chart (last 7 points - would need historical API)
        drawSimpleTrendLine(20, 180, 360, 100);

        // Footer
        Paint_DrawString_EN(10, height - 20,
                          "Data: CoinGecko API",
                          &Font12, WHITE, BLACK);
    }

private:
    void parsePriceData(const String& json) {
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, json);

        if (error) {
            Serial.println("JSON parsing failed");
            return;
        }

        // CoinGecko returns: {"bitcoin": {"usd": 50000, "usd_24h_change": 2.5, ...}}
        JsonObject coinData = doc[cryptoId];
        crypto.price = coinData[currency];
        crypto.change24h = coinData[currency + "_24h_change"];
        crypto.marketCap = coinData[currency + "_market_cap"];
        crypto.volume24h = coinData[currency + "_24h_vol"];
    }

    void drawSimpleTrendLine(int x, int y, int w, int h) {
        // Draw a simple trend indicator
        // In reality, you'd fetch historical data and plot it

        Paint_DrawString_EN(x, y - 15, "7-Day Trend", &Font12, WHITE, BLACK);

        // Draw border
        Paint_DrawRectangle(x, y, x + w, y + h, BLACK,
                          DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

        // Simulate trend based on 24h change
        int midY = y + h / 2;
        int points = 7;
        int spacing = w / (points - 1);

        for (int i = 0; i < points - 1; i++) {
            int x1 = x + i * spacing;
            int x2 = x + (i + 1) * spacing;

            // Random-ish trend based on change
            int y1 = midY + (random(-20, 20));
            int y2 = midY + (random(-20, 20) + (crypto.change24h > 0 ? -10 : 10));

            Paint_DrawLine(x1, y1, x2, y2, BLACK,
                         DOT_PIXEL_2X2, LINE_STYLE_SOLID);
        }

        // Center line
        Paint_DrawLine(x, midY, x + w, midY, BLACK,
                      DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    }

public:
    const char* getName() { return "Bitcoin Ticker"; }
};

// Example usage
/*
BitcoinTickerPlugin btcPlugin;
UBYTE* buffer = (UBYTE*)malloc(15000);

void setup() {
    Serial.begin(115200);
    DEV_Module_Init();
    EPD_4IN2_V2_Init();

    WiFi.begin("ssid", "password");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    btcPlugin.init("bitcoin", "usd");  // or "ethereum", "cardano", etc.
    btcPlugin.fetchPrice();
}

void loop() {
    btcPlugin.render(buffer, 400, 300);
    EPD_4IN2_V2_Display(buffer);

    // Update every 15 minutes
    delay(15 * 60 * 1000);
    btcPlugin.fetchPrice();
}

// Multi-crypto display
void setupMultiCrypto() {
    // Display multiple cryptocurrencies
    const char* cryptos[] = {"bitcoin", "ethereum", "cardano"};
    int currentCrypto = 0;

    // Cycle through cryptos every 30 seconds
    btcPlugin.init(cryptos[currentCrypto], "usd");
    btcPlugin.fetchPrice();
    btcPlugin.render(buffer, 400, 300);
    EPD_4IN2_V2_Display(buffer);

    currentCrypto = (currentCrypto + 1) % 3;
}
*/
