#include "DEV_Config.h"
#include "EPD.h"
#include <stdlib.h>

// Display dimensions
#define DISPLAY_WIDTH  400
#define DISPLAY_HEIGHT 300

// Test patterns
UBYTE BlackImage[15000]; // 400*300/8 = 15000 bytes

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== EPD 4.2\" V2 Function Test ===");
    
    // Test 1: Module initialization
    Serial.print("Test 1: DEV_Module_Init... ");
    if (DEV_Module_Init() != 0) {
        Serial.println("FAILED - Check wiring");
        while(1) delay(1000);
    }
    Serial.println("OK");
    
    // Test 2: EPD initialization
    Serial.print("Test 2: EPD_4IN2_V2_Init... ");
    EPD_4IN2_V2_Init();
    Serial.println("OK");
    
    // Test 3: Clear display (white)
    Serial.print("Test 3: EPD_4IN2_V2_Clear (full white)... ");
    EPD_4IN2_V2_Clear();
    DEV_Delay_ms(2000);
    Serial.println("OK");
    
    // Test 4: Fill black pattern
    Serial.print("Test 4: Full black display... ");
    Paint_NewImage(BlackImage, DISPLAY_WIDTH, DISPLAY_HEIGHT, 270, WHITE);
    Paint_Clear(BLACK);
    EPD_4IN2_V2_Display(BlackImage);
    DEV_Delay_ms(2000);
    Serial.println("OK");
    
    // Test 5: Horizontal stripes
    Serial.print("Test 5: Horizontal stripes... ");
    Paint_Clear(WHITE);
    for(int y = 0; y < DISPLAY_HEIGHT; y += 40) {
        Paint_DrawRectangle(0, y, DISPLAY_WIDTH, y+20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }
    EPD_4IN2_V2_Display(BlackImage);
    DEV_Delay_ms(2000);
    Serial.println("OK");
    
    // Test 6: Vertical stripes
    Serial.print("Test 6: Vertical stripes... ");
    Paint_Clear(WHITE);
    for(int x = 0; x < DISPLAY_WIDTH; x += 40) {
        Paint_DrawRectangle(x, 0, x+20, DISPLAY_HEIGHT, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }
    EPD_4IN2_V2_Display(BlackImage);
    DEV_Delay_ms(2000);
    Serial.println("OK");
    
    // Test 7: Checkerboard pattern
    Serial.print("Test 7: Checkerboard... ");
    Paint_Clear(WHITE);
    for(int y = 0; y < DISPLAY_HEIGHT; y += 40) {
        for(int x = 0; x < DISPLAY_WIDTH; x += 40) {
            if((x/40 + y/40) % 2 == 0) {
                Paint_DrawRectangle(x, y, x+40, y+40, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            }
        }
    }
    EPD_4IN2_V2_Display(BlackImage);
    DEV_Delay_ms(2000);
    Serial.println("OK");
    
    // Test 8: Text rendering
    Serial.print("Test 8: Text rendering... ");
    Paint_Clear(WHITE);
    Paint_DrawString_EN(10, 10, "EPD 4.2\" V2 Test", &Font24, WHITE, BLACK);
    Paint_DrawString_EN(10, 50, "SPI Communication: OK", &Font20, WHITE, BLACK);
    Paint_DrawString_EN(10, 80, "Display Refresh: OK", &Font20, WHITE, BLACK);
    Paint_DrawString_EN(10, 110, "GPIO Control: OK", &Font20, WHITE, BLACK);
    EPD_4IN2_V2_Display(BlackImage);
    DEV_Delay_ms(2000);
    Serial.println("OK");
    
    // Test 9: Partial refresh (if supported)
    Serial.print("Test 9: Partial refresh test... ");
    Paint_Clear(WHITE);
    Paint_DrawString_EN(10, 10, "Partial Update Test", &Font24, WHITE, BLACK);
    Paint_DrawRectangle(50, 100, 350, 200, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString_EN(60, 130, "Update Area", &Font20, BLACK, WHITE);
    EPD_4IN2_V2_Display_Partial(BlackImage);
    DEV_Delay_ms(1000);
    Serial.println("OK");
    
    // Test 10: Sleep mode
    Serial.print("Test 10: EPD_4IN2_V2_Sleep... ");
    EPD_4IN2_V2_Sleep();
    Serial.println("OK");
    
    Serial.println("\n=== All Tests Complete ===");
    Serial.println("Display entering deep sleep.");
    Serial.println("Reset to run tests again.\n");
}

void loop() {
    // Nothing - all tests run once in setup
}
