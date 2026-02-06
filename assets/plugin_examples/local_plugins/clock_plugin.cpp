/**
 * CLOCK PLUGIN - Local (No WiFi Required)
 *
 * Simple clock display using ESP32 internal RTC
 * Update frequency: 1 minute (partial refresh recommended)
 *
 * Inspired by InkyPi Clock Plugin
 */

#include <Arduino.h>
#include <time.h>
#include "EPD.h"
#include "GUI_Paint.h"

class ClockPlugin {
private:
    struct tm timeinfo;
    bool use24Hour = true;

public:
    void init() {
        // Initialize RTC (you can sync via NTP when WiFi available)
        // For now, set a default time
        timeinfo.tm_year = 2024 - 1900;
        timeinfo.tm_mon = 0;
        timeinfo.tm_mday = 30;
        timeinfo.tm_hour = 12;
        timeinfo.tm_min = 0;
        timeinfo.tm_sec = 0;
    }

    void render(UBYTE* buffer, int width, int height) {
        Paint_SelectImage(buffer);
        Paint_Clear(WHITE);

        // Get current time
        char timeStr[20];
        if (use24Hour) {
            sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        } else {
            int hour12 = timeinfo.tm_hour % 12;
            if (hour12 == 0) hour12 = 12;
            sprintf(timeStr, "%2d:%02d %s",
                   hour12, timeinfo.tm_min,
                   timeinfo.tm_hour >= 12 ? "PM" : "AM");
        }

        // Center the time (large font)
        int timeX = (width - strlen(timeStr) * 24) / 2;
        int timeY = height / 2 - 20;
        Paint_DrawString_EN(timeX, timeY, timeStr, &Font24, WHITE, BLACK);

        // Draw date below
        char dateStr[30];
        const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        sprintf(dateStr, "%s %d, %d",
                months[timeinfo.tm_mon],
                timeinfo.tm_mday,
                timeinfo.tm_year + 1900);

        int dateX = (width - strlen(dateStr) * 8) / 2;
        int dateY = timeY + 40;
        Paint_DrawString_EN(dateX, dateY, dateStr, &Font16, WHITE, BLACK);

        // Draw decorative elements
        Paint_DrawLine(50, height - 50, width - 50, height - 50, BLACK,
                      DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    }

    void update() {
        // Increment time (called every second)
        timeinfo.tm_sec++;
        mktime(&timeinfo);  // Normalize time structure
    }

    void syncWithNTP(const char* ntpServer = "pool.ntp.org") {
        // Optional: Sync with NTP when WiFi available
        configTime(0, 0, ntpServer);
        if (getLocalTime(&timeinfo)) {
            Serial.println("Time synced with NTP");
        }
    }

    void set24HourMode(bool enable) {
        use24Hour = enable;
    }

    const char* getName() { return "Clock"; }
};

// Example usage in main code
/*
ClockPlugin clockPlugin;
UBYTE* buffer = (UBYTE*)malloc(15000);

void setup() {
    DEV_Module_Init();
    EPD_4IN2_V2_Init();
    clockPlugin.init();

    // Optional: Sync with NTP if WiFi available
    // WiFi.begin("ssid", "password");
    // clockPlugin.syncWithNTP();
}

void loop() {
    clockPlugin.render(buffer, 400, 300);
    EPD_4IN2_V2_Display_Partial(buffer);

    delay(60000);  // Update every minute
    clockPlugin.update();
}
*/
