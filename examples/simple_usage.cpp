/**
 * Simple E-Paper Display Example
 * 
 * Demonstrates the simplified API (Adafruit GFX style)
 * - Auto buffer allocation
 * - Single begin() call
 * - Clean initialization
 */

#include <Arduino.h>
#include "EPD.h"

// Option 1: C++ Class Style (Recommended)
EPD_Display epd;

void setup() {
    Serial.begin(115200);
    Serial.println("Simple E-Paper Example");
    
    // Single call initialization!
    if(!epd.begin()) {
        Serial.println("EPD initialization failed!");
        while(1) delay(1000);
    }
    
    Serial.println("EPD ready!");
    
    // Clear to white
    epd.clear();
    
    // Draw something
    Paint_DrawString_EN(100, 120, "Hello World!", &Font24, WHITE, BLACK);
    Paint_DrawRectangle(50, 100, 350, 160, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    
    // Display it
    epd.display();
    
    Serial.println("Done!");
}

void loop() {
    delay(1000);
}
