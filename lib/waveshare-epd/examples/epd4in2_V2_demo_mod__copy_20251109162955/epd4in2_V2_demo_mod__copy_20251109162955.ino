#include "DEV_Config.h"
#include "EPD.h"
#include <stdlib.h>
#include <Adafruit_GFX.h>
#include <GUI_Paint.h>
#include "fonts.h"

#define W 400
#define H 300

UBYTE *testImage;

void testAllGUI_PaintFunctions() {
    Serial.println("\n=== GUI_Paint Complete Function Test ===\n");
    
    // Allocate buffer
    testImage = (UBYTE*)malloc(W * H / 8);
    if(!testImage) {
        Serial.println("Memory allocation failed!");
        return;
    }
    
    // Initialize hardware
    if(DEV_Module_Init() != 0) {
        Serial.println("Hardware init failed!");
        free(testImage);
        return;
    }
    EPD_4IN2_V2_Init();
    
    // Test 1: Paint_NewImage & Paint_Clear
    Serial.print("1. Paint_NewImage + Paint_Clear... ");
    Paint_NewImage(testImage, W, H, 270, WHITE);
    Paint_Clear(WHITE);
    EPD_4IN2_V2_Display(testImage);
    DEV_Delay_ms(1500);
    Serial.println("OK");
    
    // Test 2: Paint_SetPixel - Single pixels
    Serial.print("2. Paint_SetPixel (diagonal line)... ");
    Paint_Clear(WHITE);
    for(int i = 0; i < 100; i++) {
        Paint_SetPixel(i, i, BLACK);
    }
    EPD_4IN2_V2_Display(testImage);
    DEV_Delay_ms(1500);
    Serial.println("OK");
    
    // Test 3: Paint_DrawPoint - Different sizes
    Serial.print("3. Paint_DrawPoint (various sizes)... ");
    Paint_Clear(WHITE);
    Paint_DrawPoint(50, 50, BLACK, DOT_PIXEL_1X1, DOT_FILL_AROUND);
    Paint_DrawPoint(100, 50, BLACK, DOT_PIXEL_3X3, DOT_FILL_AROUND);
    Paint_DrawPoint(150, 50, BLACK, DOT_PIXEL_5X5, DOT_FILL_AROUND);
    Paint_DrawPoint(200, 50, BLACK, DOT_PIXEL_8X8, DOT_FILL_AROUND);
    EPD_4IN2_V2_Display(testImage);
    DEV_Delay_ms(1500);
    Serial.println("OK");
    
    // Test 4: Paint_DrawLine - Solid & Dotted
    Serial.print("4. Paint_DrawLine (solid & dotted)... ");
    Paint_Clear(WHITE);
    Paint_DrawLine(10, 10, 300, 10, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(10, 30, 300, 30, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(10, 60, 300, 60, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawLine(10, 80, 300, 200, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    EPD_4IN2_V2_Display(testImage);
    DEV_Delay_ms(1500);
    Serial.println("OK");
    
    // Test 5: Paint_DrawRectangle - Empty & Filled
    Serial.print("5. Paint_DrawRectangle (empty & filled)... ");
    Paint_Clear(WHITE);
    Paint_DrawRectangle(10, 10, 100, 80, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(120, 10, 210, 80, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(230, 10, 320, 80, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    EPD_4IN2_V2_Display(testImage);
    DEV_Delay_ms(1500);
    Serial.println("OK");
    
    // Test 6: Paint_DrawCircle - Empty & Filled
    Serial.print("6. Paint_DrawCircle (empty & filled)... ");
    Paint_Clear(WHITE);
    Paint_DrawCircle(60, 140, 40, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(170, 140, 40, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawCircle(280, 140, 40, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    EPD_4IN2_V2_Display(testImage);
    DEV_Delay_ms(1500);
    Serial.println("OK");
    
    // Test 7: Paint_DrawChar - FIXED: Only use available fonts
    Serial.print("7. Paint_DrawChar... ");
    Paint_Clear(WHITE);
    Paint_DrawChar(10, 10, 'A', &Font24, WHITE, BLACK);
    Paint_DrawChar(40, 10, 'B', &Font20, WHITE, BLACK);
    Paint_DrawChar(70, 10, '1', &Font24, WHITE, BLACK);  // Changed to Font24
    Paint_DrawChar(100, 10, '2', &Font20, WHITE, BLACK); // Changed to Font20
    EPD_4IN2_V2_Display(testImage);
    DEV_Delay_ms(1500);
    Serial.println("OK");
    
    // Test 8: Paint_DrawString_EN - FIXED: Only use available fonts
    Serial.print("8. Paint_DrawString_EN... ");
    Paint_Clear(WHITE);
    Paint_DrawString_EN(10, 10, "Font24 Test", &Font24, WHITE, BLACK);
    Paint_DrawString_EN(10, 40, "Font20 Test", &Font20, WHITE, BLACK);
    //Paint_DrawString_EN(10, 70, "Font8 Test", &Font8, WHITE, BLACK);   // Changed to Font8
    //Paint_DrawString_EN(10, 100, "Font8 Small", &Font8, WHITE, BLACK); // Changed to Font8
    EPD_4IN2_V2_Display(testImage);
    DEV_Delay_ms(1500);
    Serial.println("OK");
    
    // Test 9: Paint_DrawNum
    Serial.print("9. Paint_DrawNum... ");
    Paint_Clear(WHITE);
    Paint_DrawNum(10, 10, 12345, &Font24, WHITE, BLACK);
    Paint_DrawNum(10, 50, 67890, &Font20, WHITE, BLACK);
    //Paint_DrawNum(10, 90, 999, &Font8, WHITE, BLACK);  // Changed to positive, use Font8
    EPD_4IN2_V2_Display(testImage);
    DEV_Delay_ms(1500);
    Serial.println("OK");
    
    // Test 10: Paint_DrawTime
    Serial.print("10. Paint_DrawTime... ");
    Paint_Clear(WHITE);
    PAINT_TIME testTime = {2024, 11, 10, 14, 30, 45};
    Paint_DrawTime(10, 10, &testTime, &Font24, WHITE, BLACK);
    EPD_4IN2_V2_Display(testImage);
    DEV_Delay_ms(1500);
    Serial.println("OK");
    
    // Test 11: Paint_ClearWindows
    Serial.print("11. Paint_ClearWindows... ");
    Paint_Clear(BLACK);
    Paint_ClearWindows(50, 50, 250, 150, WHITE);
    EPD_4IN2_V2_Display(testImage);
    DEV_Delay_ms(1500);
    Serial.println("OK");
    
    // Test 12: Paint_SetRotate
    Serial.print("12. Paint_SetRotate... ");
    Paint_NewImage(testImage, W, H, 0, WHITE);
    Paint_Clear(WHITE);
    Paint_DrawString_EN(10, 10, "Rotation 0", &Font20, WHITE, BLACK);
    EPD_4IN2_V2_Display(testImage);
    DEV_Delay_ms(1500);
    Serial.println("OK");
    
    // Test 13: Paint_SetMirroring
    Serial.print("13. Paint_SetMirroring... ");
    Paint_SetMirroring(MIRROR_HORIZONTAL);
    Paint_Clear(WHITE);
    Paint_DrawString_EN(10, 10, "Mirrored", &Font20, WHITE, BLACK);
    EPD_4IN2_V2_Display(testImage);
    Paint_SetMirroring(MIRROR_NONE);
    DEV_Delay_ms(1500);
    Serial.println("OK");
    
    // Test 14: Paint_SetScale (grayscale)
    Serial.print("14. Paint_SetScale (4-level gray)... ");
    Paint_SetScale(4);
    Paint_Clear(GRAY4);
    Paint_DrawRectangle(10, 10, 100, 100, GRAY1, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawRectangle(110, 10, 200, 100, GRAY2, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawRectangle(210, 10, 300, 100, GRAY3, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    EPD_4IN2_V2_Display(testImage);
    Paint_SetScale(2); // Reset to 1-bit
    DEV_Delay_ms(1500);
    Serial.println("OK");
    
    // Test 15: Complex pattern - FIXED: Cast i%2 to DRAW_FILL enum
    Serial.print("15. Complex pattern test... ");
    Paint_Clear(WHITE);
    for(int i = 0; i < 10; i++) {
        Paint_DrawCircle(50 + i*35, 150, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    }
    for(int i = 0; i < 5; i++) {
        DRAW_FILL fillMode = (i % 2 == 0) ? DRAW_FILL_EMPTY : DRAW_FILL_FULL;
        Paint_DrawRectangle(10 + i*70, 10, 60 + i*70, 80, BLACK, DOT_PIXEL_1X1, fillMode);
    }
    EPD_4IN2_V2_Display(testImage);
    DEV_Delay_ms(1500);
    Serial.println("OK");
    
    // Cleanup
    Serial.println("\n=== All GUI_Paint Tests Complete ===\n");
    EPD_4IN2_V2_Sleep();
    free(testImage);
}
void setup()
{
    Serial.begin(115200);
    delay(50);
    Serial.println("Starting DEV module init...");

    if (DEV_Module_Init() != 0) {Serial.println("DEV_Module_Init failed. Check wiring and pin defines.");
        while (1) { delay(1000); }
    }

    // EPD_4IN2_V2_Init();
    // EPD_4IN2_V2_Clear();
    DEV_Delay_ms(500);

    testAllGUI_PaintFunctions();

    //free(BlackImage);

}

void loop(){}


