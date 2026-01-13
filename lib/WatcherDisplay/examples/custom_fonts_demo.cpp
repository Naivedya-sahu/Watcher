/**
 * Custom Fonts Demo - WatcherDisplay Library
 *
 * Demonstrates how to use custom TTF/OTF fonts with the WatcherDisplay library
 *
 * Hardware:
 *   - ESP32-S3 (or any ESP32)
 *   - Waveshare 4.2" V2 E-Paper Display (400×300)
 *
 * Setup:
 *   1. Generate fonts: cd lib/WatcherDisplay/tools && python generate_all.py
 *   2. Copy generated .cpp/.h files to your src/ directory
 *   3. Include font headers below
 *   4. Upload and run
 *
 * Features demonstrated:
 *   - Font registration
 *   - Drawing text with custom fonts
 *   - Drawing numbers with custom fonts
 *   - Multiple font sizes
 *   - Font fallback handling
 */

#include <Arduino.h>
#include <WatcherDisplay.h>

// Include your generated custom fonts
// (After running generate_all.py and copying to src/)
#include "Monocraft16.h"
#include "Monocraft24.h"
#include "Monocraft32.h"

// Create display instance
WatcherDisplay display;

void setup() {
    Serial.begin(115200);
    Serial.println("\n=================================");
    Serial.println("Custom Fonts Demo - WatcherDisplay");
    Serial.println("=================================\n");

    // Initialize display
    Serial.println("Initializing display...");
    if (!display.begin()) {
        Serial.println("ERROR: Display initialization failed!");
        Serial.println("Check wiring:");
        Serial.println("  CS=10, DC=15, RST=16, BUSY=17");
        Serial.println("  MOSI=11, SCK=12");
        while (1) delay(100);
    }
    Serial.println("Display initialized ✓");

    // Clear display
    Serial.println("Clearing display...");
    display.clear();
    Serial.println("Display cleared ✓\n");

    // Register custom fonts
    Serial.println("Registering custom fonts...");
    FONT_REGISTER("Monocraft", 16, Monocraft16);
    FONT_REGISTER("Monocraft", 24, Monocraft24);
    FONT_REGISTER("Monocraft", 32, Monocraft32);
    Serial.println("Fonts registered ✓\n");

    // List registered fonts
    Serial.println("Registered fonts:");
    FontHandler::getInstance().listFonts();
    Serial.println();

    // Draw demo content
    drawDemo();

    Serial.println("\nDemo complete!");
    Serial.println("Display is now showing custom fonts");
}

void loop() {
    // Static display - nothing to do in loop
    delay(1000);
}

/**
 * Draw demonstration content with custom fonts
 */
void drawDemo() {
    Serial.println("Drawing demo content...\n");

    // ===== Demo 1: Title with Large Font =====
    Serial.println("1. Drawing title (Monocraft 24px)...");
    display.drawTextCustom(80, 10, "CUSTOM FONTS", "Monocraft", 24, true);
    display.updateRegion(80, 10, 260, 35);
    delay(500);

    // ===== Demo 2: Subtitle with Medium Font =====
    Serial.println("2. Drawing subtitle (Monocraft 16px)...");
    display.drawTextCustom(100, 50, "WatcherDisplay Demo", "Monocraft", 16, true);
    display.updateRegion(100, 50, 220, 25);
    delay(500);

    // ===== Demo 3: Font Size Comparison =====
    Serial.println("3. Drawing size comparison...");
    int y = 90;

    display.drawTextCustom(20, y, "16px: The quick brown fox", "Monocraft", 16, true);
    y += 25;

    display.drawTextCustom(20, y, "24px: Quick fox", "Monocraft", 24, true);
    y += 35;

    display.drawTextCustom(20, y, "32px: Fox", "Monocraft", 32, true);

    display.updateRegion(20, 90, 380, 100);
    delay(500);

    // ===== Demo 4: Numbers with Custom Font =====
    Serial.println("4. Drawing numbers (countdown style)...");
    display.clearRegion(50, 200, 300, 50);

    // Draw countdown timer-style display
    char timeStr[10];
    int minutes = 25;
    int seconds = 0;
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", minutes, seconds);

    display.drawTextCustom(120, 205, timeStr, "Monocraft", 32, true);
    display.updateRegion(50, 200, 300, 50);
    delay(500);

    // ===== Demo 5: Footer with Small Font =====
    Serial.println("5. Drawing footer...");
    display.drawTextCustom(90, 270, "Font: Monocraft.ttf", "Monocraft", 16, true);
    display.updateRegion(90, 270, 240, 25);

    Serial.println("\nAll demo elements drawn successfully ✓");
}

/**
 * Example: Dynamic counter with custom font
 * (Uncomment and move to loop() for animated demo)
 */
/*
void updateCounter() {
    static int counter = 0;

    // Clear counter area
    display.clearRegion(150, 200, 100, 40);

    // Draw counter with large custom font
    display.drawNumberCustom(150, 205, counter, "Monocraft", 32, true);

    // Update display
    display.updateRegion(150, 200, 100, 40);

    counter++;
    if (counter > 99) counter = 0;
}
*/

/**
 * Example: Check if font exists before using
 */
void safeFontUsage() {
    const char* fontName = "Monocraft";
    uint8_t fontSize = 24;

    if (FontHandler::getInstance().hasFont(fontName, fontSize)) {
        Serial.printf("Font '%s' %dpx is available ✓\n", fontName, fontSize);
        display.drawTextCustom(100, 100, "Font OK", fontName, fontSize, true);
    } else {
        Serial.printf("Font '%s' %dpx not found, using fallback\n", fontName, fontSize);
        display.drawText(100, 100, "Font OK", &Font24, true);  // Fallback
    }

    display.updateRegion(100, 100, 150, 35);
}

/**
 * Example: Direct font access (alternative to drawTextCustom)
 */
void directFontAccess() {
    // Get font object
    sFONT* myFont = FONT_GET("Monocraft", 24);

    if (myFont) {
        // Use with standard drawText()
        display.drawText(100, 50, "Direct access", myFont, true);
        display.updateRegion(100, 50, 200, 35);
    } else {
        Serial.println("Font not found!");
    }
}

/**
 * Example: Multi-line text with custom font
 */
void multiLineDemo() {
    const char* fontName = "Monocraft";
    uint8_t fontSize = 16;
    uint16_t lineHeight = 20;
    uint16_t x = 50;
    uint16_t y = 80;

    const char* lines[] = {
        "Line 1: Custom fonts",
        "Line 2: On e-paper",
        "Line 3: Easy to use",
        "Line 4: Great results"
    };

    for (int i = 0; i < 4; i++) {
        display.drawTextCustom(x, y + (i * lineHeight), lines[i], fontName, fontSize, true);
    }

    display.updateRegion(x, y, 300, 80);
}

/**
 * Example: Centered text with custom font
 */
void centeredText(const char* text, uint16_t y, const char* fontName, uint8_t fontSize) {
    // Get font to calculate width (approximate)
    sFONT* font = FONT_GET(fontName, fontSize);
    if (!font) return;

    // Approximate text width (font->Width is per character)
    uint16_t textWidth = strlen(text) * font->Width;

    // Center on 400px wide display
    uint16_t x = (400 - textWidth) / 2;

    display.drawTextCustom(x, y, text, fontName, fontSize, true);
    display.updateRegion(x, y, textWidth + 20, font->Height + 5);
}

/**
 * Example: Timer display with custom font
 */
void timerDisplay(uint8_t minutes, uint8_t seconds) {
    // Format time
    char timeStr[6];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", minutes, seconds);

    // Large centered timer
    centeredText(timeStr, 120, "Monocraft", 32);

    // Label below
    centeredText("TIMER", 170, "Monocraft", 16);
}

/**
 * Example: Status display with mixed fonts
 */
void statusDisplay() {
    // Title
    display.drawTextCustom(120, 10, "SYSTEM STATUS", "Monocraft", 24, true);
    display.updateRegion(120, 10, 220, 35);

    // Labels with medium font
    uint16_t labelX = 30;
    uint16_t valueX = 200;
    uint16_t y = 60;
    uint16_t spacing = 30;

    display.drawTextCustom(labelX, y, "Temp:", "Monocraft", 16, true);
    display.drawNumberCustom(valueX, y, 23, "Monocraft", 16, true);
    y += spacing;

    display.drawTextCustom(labelX, y, "Humidity:", "Monocraft", 16, true);
    display.drawNumberCustom(valueX, y, 65, "Monocraft", 16, true);
    y += spacing;

    display.drawTextCustom(labelX, y, "Battery:", "Monocraft", 16, true);
    display.drawNumberCustom(valueX, y, 85, "Monocraft", 16, true);

    display.updateRegion(30, 60, 250, 100);
}
