/**
 * Simple E-Paper Display Example - C Style
 * 
 * Demonstrates the simplified C API
 * - Auto buffer allocation
 * - No manual malloc needed
 */

#include <Arduino.h>
#include "EPD.h"

void setup() {
    Serial.begin(115200);
    Serial.println("Simple E-Paper Example (C Style)");
    
    // Initialize hardware
    if(DEV_Module_Init() != 0) {
        Serial.println("Hardware init failed!");
        while(1) delay(1000);
    }
    
    EPD_4IN2_V2_Init();
    
    // Auto-allocate buffer and initialize Paint
    if(!Paint_Begin(EPD_4IN2_V2_WIDTH, EPD_4IN2_V2_HEIGHT)) {
        Serial.println("Paint_Begin failed!");
        while(1) delay(1000);
    }
    
    Serial.println("Ready!");
    
    // Clear display
    EPD_4IN2_V2_Clear();
    Paint_Clear(WHITE);
    
    // Draw something
    Paint_DrawString_EN(100, 120, "Hello World!", &Font24, WHITE, BLACK);
    Paint_DrawCircle(200, 150, 50, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    
    // Display it
    EPD_4IN2_V2_Display(Paint_GetBuffer());
    
    // Put to sleep
    EPD_4IN2_V2_Sleep();
    
    Serial.println("Done!");
}

void loop() {
    delay(1000);
}

// Don't forget to free buffer when done
void cleanup() {
    Paint_End();
}
