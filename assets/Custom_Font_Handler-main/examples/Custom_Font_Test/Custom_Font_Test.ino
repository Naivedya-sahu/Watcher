/*
 * Font Test Sketch - Minecraft Fonts
 * Tests FontMinecraft16 and FontMinecraft24 on e-paper display
 */

// CRITICAL: Define fonts BEFORE including fonts.h
#define FONT_ENABLE_MINECRAFT_16
#define FONT_ENABLE_MINECRAFT_24

#include "EPD.h"
#include "GUI_Paint.h"
#include "DEV_Config.h"

UBYTE *ImageBuffer;

void setup() {
    Serial.begin(115200);
    Serial.println("Font Test - Minecraft Fonts");
    
    // Initialize hardware
    DEV_Module_Init();
    
    // Initialize display
    Serial.println("Initializing display...");
    EPD_4IN2_V2_Init();
    EPD_4IN2_V2_Clear();
    
    // Calculate buffer size
    UWORD imageSize = ((EPD_4IN2_V2_WIDTH % 8 == 0) ? 
                       (EPD_4IN2_V2_WIDTH / 8) : 
                       (EPD_4IN2_V2_WIDTH / 8 + 1)) * EPD_4IN2_V2_HEIGHT;
    
    Serial.print("Buffer size: ");
    Serial.print(imageSize);
    Serial.println(" bytes");
    
    // Allocate buffer
    ImageBuffer = (UBYTE *)malloc(imageSize);
    if (ImageBuffer == NULL) {
        Serial.println("ERROR: Failed to allocate buffer!");
        while(1);
    }
    
    // Initialize paint library
    Paint_NewImage(ImageBuffer, EPD_4IN2_V2_WIDTH, EPD_4IN2_V2_HEIGHT, 
                   ROTATE_0, WHITE);
    Paint_Clear(WHITE);
    
    Serial.println("Drawing text...");
    
    // Test FontMinecraft16
    Paint_DrawString_EN(10, 10, "Minecraft 16px Font", 
                        &FontMinecraft16, WHITE, BLACK);
    
    Paint_DrawString_EN(10, 40, "0123456789", 
                        &FontMinecraft16, WHITE, BLACK);
    
    Paint_DrawString_EN(10, 70, "ABCDEFGHIJKLMNOP", 
                        &FontMinecraft16, WHITE, BLACK);
    
    // Test FontMinecraft24
    Paint_DrawString_EN(10, 120, "Minecraft 24px", 
                        &FontMinecraft24, WHITE, BLACK);
    
    Paint_DrawString_EN(10, 160, "Bigger Text!", 
                        &FontMinecraft24, WHITE, BLACK);
    
    // Test numbers with FontMinecraft16
    Paint_DrawNum(10, 220, 42, &FontMinecraft16, WHITE, BLACK);
    Paint_DrawString_EN(60, 220, "is the answer", 
                        &FontMinecraft16, WHITE, BLACK);
    
    Serial.println("Displaying...");
    EPD_4IN2_V2_Display(ImageBuffer);
    
    Serial.println("Done! Display should show text now.");
    Serial.println("Putting display to sleep...");
    
    EPD_4IN2_V2_Sleep();
    
    Serial.println("Test complete.");
}

void loop() {
    // Nothing - this is a one-shot test
}
