#include <Arduino.h>
#include "EPD.h"
#include "GUI_Paint.h"

// ============================================================
// SIMPLE COUNTDOWN TIMER
// - 2 digits (tens and ones) showing minutes 00-99
// - 60 progress squares around border (one per second)
// - Polygon-based 7-segment rendering from ONES.svg paths
// - Partial refresh for smooth updates
// - No buttons, no title - minimal & clean
// ============================================================

#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 300

// Digit dimensions (from ONES.svg: 70×130)
#define DIGIT_WIDTH   70
#define DIGIT_HEIGHT  130

// Digit positions (centered on screen)
#define TENS_X        122
#define TENS_Y        85
#define ONES_X        207
#define ONES_Y        85

// Progress squares (60 total, 10×10 each)
#define PROGRESS_SIZE 10
#define BORDER_X      59
#define BORDER_Y      49
#define BORDER_W      282
#define BORDER_H      202

// Display buffer
UBYTE *BlackImage;

// Timer state
uint16_t remainingSeconds = 5 * 60;  // 5 minutes
uint16_t elapsedSeconds = 0;
uint8_t lastTens = 255;
uint8_t lastOnes = 255;
uint8_t lastSecond = 255;
uint32_t lastUpdate = 0;
bool isRunning = true;  // Auto-start

// Progress square positions (60 squares total)
struct SquarePos { uint16_t x, y; };
SquarePos squarePositions[60];

// ============================================================
// 7-SEGMENT DEFINITIONS (from ONES.svg)
// Segments: A=top, B=top-right, C=bottom-right, D=bottom,
//           E=bottom-left, F=top-left, G=middle
// ============================================================

// Segment A - Top horizontal
// Path: M0.673096 0H70L57.2116 10.8333H12.7885L0.673096 0Z
void drawSegmentA(uint16_t x, uint16_t y, bool on) {
    if (!on) return;
    int16_t xpts[] = {(int16_t)(x+1), (int16_t)(x+70), (int16_t)(x+57), (int16_t)(x+13), (int16_t)(x+1)};
    int16_t ypts[] = {(int16_t)(y+0), (int16_t)(y+0), (int16_t)(y+11), (int16_t)(y+11), (int16_t)(y+0)};
    Paint_DrawPolygon(xpts, ypts, 5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment B - Top right vertical
// Path: M70 3.38541L70 63.6458L59.2308 56.875L59.2308 12.8646L70 3.38541Z
void drawSegmentB(uint16_t x, uint16_t y, bool on) {
    if (!on) return;
    int16_t xpts[] = {(int16_t)(x+70), (int16_t)(x+70), (int16_t)(x+59), (int16_t)(x+59), (int16_t)(x+70)};
    int16_t ypts[] = {(int16_t)(y+3), (int16_t)(y+64), (int16_t)(y+57), (int16_t)(y+13), (int16_t)(y+3)};
    Paint_DrawPolygon(xpts, ypts, 5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment C - Bottom right vertical
// Path: M70 67.0312L70 128.646L59.2308 117.812L59.2308 73.125L70 67.0312Z
void drawSegmentC(uint16_t x, uint16_t y, bool on) {
    if (!on) return;
    int16_t xpts[] = {(int16_t)(x+70), (int16_t)(x+70), (int16_t)(x+59), (int16_t)(x+59), (int16_t)(x+70)};
    int16_t ypts[] = {(int16_t)(y+67), (int16_t)(y+129), (int16_t)(y+118), (int16_t)(y+73), (int16_t)(y+67)};
    Paint_DrawPolygon(xpts, ypts, 5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment D - Bottom horizontal
// Path: M67.3076 130L2.69223 130L12.7884 119.167L57.2115 119.167L67.3076 130Z
void drawSegmentD(uint16_t x, uint16_t y, bool on) {
    if (!on) return;
    int16_t xpts[] = {(int16_t)(x+67), (int16_t)(x+3), (int16_t)(x+13), (int16_t)(x+57), (int16_t)(x+67)};
    int16_t ypts[] = {(int16_t)(y+130), (int16_t)(y+130), (int16_t)(y+119), (int16_t)(y+119), (int16_t)(y+130)};
    Paint_DrawPolygon(xpts, ypts, 5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment E - Bottom left vertical
// Path: M0 128.646L8.12023e-06 66.3542L10.7692 73.125L10.7692 117.135L0 128.646Z
void drawSegmentE(uint16_t x, uint16_t y, bool on) {
    if (!on) return;
    int16_t xpts[] = {(int16_t)(x+0), (int16_t)(x+0), (int16_t)(x+11), (int16_t)(x+11), (int16_t)(x+0)};
    int16_t ypts[] = {(int16_t)(y+129), (int16_t)(y+66), (int16_t)(y+73), (int16_t)(y+117), (int16_t)(y+129)};
    Paint_DrawPolygon(xpts, ypts, 5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment F - Top left vertical
// Path: M0 63.6458L-2.47137e-06 2.70835L10.7692 12.8646L10.7692 56.875L0 63.6458Z
void drawSegmentF(uint16_t x, uint16_t y, bool on) {
    if (!on) return;
    int16_t xpts[] = {(int16_t)(x+0), (int16_t)(x+0), (int16_t)(x+11), (int16_t)(x+11), (int16_t)(x+0)};
    int16_t ypts[] = {(int16_t)(y+64), (int16_t)(y+3), (int16_t)(y+13), (int16_t)(y+57), (int16_t)(y+64)};
    Paint_DrawPolygon(xpts, ypts, 5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment G - Middle horizontal
// Path: M10.7693 59.5833H59.2308L68.6539 65L59.2308 70.4167H10.7693L2.01929 65L10.7693 59.5833Z
void drawSegmentG(uint16_t x, uint16_t y, bool on) {
    if (!on) return;
    int16_t xpts[] = {(int16_t)(x+11), (int16_t)(x+59), (int16_t)(x+69), (int16_t)(x+59), (int16_t)(x+11), (int16_t)(x+2), (int16_t)(x+11)};
    int16_t ypts[] = {(int16_t)(y+60), (int16_t)(y+60), (int16_t)(y+65), (int16_t)(y+70), (int16_t)(y+70), (int16_t)(y+65), (int16_t)(y+60)};
    Paint_DrawPolygon(xpts, ypts, 7, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Digit segment map (which segments to light for each digit)
const bool DIGIT_SEGS[10][7] = {
    //  A      B      C      D      E      F      G
    {true,  true,  true,  true,  true,  true,  false}, // 0
    {false, true,  true,  false, false, false, false}, // 1
    {true,  true,  false, true,  true,  false, true},  // 2
    {true,  true,  true,  true,  false, false, true},  // 3
    {false, true,  true,  false, false, true,  true},  // 4
    {true,  false, true,  true,  false, true,  true},  // 5
    {true,  false, true,  true,  true,  true,  true},  // 6
    {true,  true,  true,  false, false, false, false}, // 7
    {true,  true,  true,  true,  true,  true,  true},  // 8
    {true,  true,  true,  true,  false, true,  true}   // 9
};

// Draw complete digit at position
void drawDigit(uint16_t x, uint16_t y, uint8_t digit) {
    if (digit > 9) return;
    const bool* segs = DIGIT_SEGS[digit];
    drawSegmentA(x, y, segs[0]);
    drawSegmentB(x, y, segs[1]);
    drawSegmentC(x, y, segs[2]);
    drawSegmentD(x, y, segs[3]);
    drawSegmentE(x, y, segs[4]);
    drawSegmentF(x, y, segs[5]);
    drawSegmentG(x, y, segs[6]);
}

// ============================================================
// CALCULATE 60 SQUARE POSITIONS (Clockwise from top-left)
// ============================================================
void calculateSquarePositions() {
    uint8_t idx = 0;

    // Top edge - 18 squares
    for (uint8_t i = 0; i < 18; i++) {
        squarePositions[idx].x = BORDER_X + (i * 16);
        squarePositions[idx].y = BORDER_Y;
        idx++;
    }

    // Right edge - 12 squares
    for (uint8_t i = 1; i <= 12; i++) {
        squarePositions[idx].x = BORDER_X + BORDER_W - PROGRESS_SIZE;
        squarePositions[idx].y = BORDER_Y + (i * 16);
        idx++;
    }

    // Bottom edge - 18 squares
    for (uint8_t i = 0; i < 18; i++) {
        squarePositions[idx].x = BORDER_X + BORDER_W - PROGRESS_SIZE - (i * 16);
        squarePositions[idx].y = BORDER_Y + BORDER_H - PROGRESS_SIZE;
        idx++;
    }

    // Left edge - 12 squares
    for (uint8_t i = 1; i <= 12; i++) {
        squarePositions[idx].x = BORDER_X;
        squarePositions[idx].y = BORDER_Y + BORDER_H - PROGRESS_SIZE - (i * 16);
        idx++;
    }

    Serial.printf("Calculated %d square positions\n", idx);
}

// Draw progress square
void drawProgressSquare(uint8_t index, bool filled) {
    if (index >= 60) return;
    uint16_t x = squarePositions[index].x;
    uint16_t y = squarePositions[index].y;

    if (filled) {
        Paint_DrawRectangle(x, y, x + PROGRESS_SIZE, y + PROGRESS_SIZE,
                          BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    } else {
        Paint_DrawRectangle(x, y, x + PROGRESS_SIZE, y + PROGRESS_SIZE,
                          BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    }
}

// ============================================================
// PARTIAL REFRESH
// ============================================================
void partialRefresh(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint16_t refreshX = (x / 8) * 8;
    uint16_t refreshWidth = ((w + 7) / 8) * 8;
    uint16_t refreshXEnd = refreshX + refreshWidth;
    uint16_t refreshYEnd = y + h;

    if (refreshXEnd > SCREEN_WIDTH) refreshXEnd = SCREEN_WIDTH;
    if (refreshYEnd > SCREEN_HEIGHT) refreshYEnd = SCREEN_HEIGHT;

    UWORD bytesPerRow = ((SCREEN_WIDTH % 8 == 0) ? (SCREEN_WIDTH / 8) : (SCREEN_WIDTH / 8 + 1));
    UWORD regionBytes = refreshWidth / 8;
    UWORD bufferSize = regionBytes * h;

    UBYTE *buffer = (UBYTE*)malloc(bufferSize);
    if (!buffer) return;

    UWORD startX = refreshX / 8;
    for (UWORD row = 0; row < h; row++) {
        if ((y + row) >= SCREEN_HEIGHT) break;
        UWORD srcOff = (y + row) * bytesPerRow + startX;
        UWORD dstOff = row * regionBytes;
        memcpy(&buffer[dstOff], &BlackImage[srcOff], regionBytes);
    }

    EPD_4IN2_V2_PartialDisplay(buffer, refreshX, y, refreshXEnd, refreshYEnd);
    free(buffer);
}

// ============================================================
// DISPLAY FUNCTIONS
// ============================================================
void drawFullScreen() {
    Paint_Clear(WHITE);

    // Calculate digits
    uint8_t minutes = remainingSeconds / 60;
    uint8_t tens = minutes / 10;
    uint8_t ones = minutes % 10;

    // Draw digits
    drawDigit(TENS_X, TENS_Y, tens);
    drawDigit(ONES_X, ONES_Y, ones);

    // Draw all 60 progress squares
    uint8_t secondInMinute = elapsedSeconds % 60;
    for (uint8_t i = 0; i < 60; i++) {
        drawProgressSquare(i, i < secondInMinute);
    }

    EPD_4IN2_V2_Display(BlackImage);

    lastTens = tens;
    lastOnes = ones;
    lastSecond = 255;  // Force update on first tick
}

void updateDigits() {
    uint8_t minutes = remainingSeconds / 60;
    uint8_t tens = minutes / 10;
    uint8_t ones = minutes % 10;

    if (tens != lastTens || ones != lastOnes) {
        Serial.printf("Updating digits: %d%d\n", tens, ones);

        // Clear digit areas
        Paint_DrawRectangle(TENS_X - 8, TENS_Y - 8, TENS_X + DIGIT_WIDTH + 8, TENS_Y + DIGIT_HEIGHT + 8,
                           WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawRectangle(ONES_X - 8, ONES_Y - 8, ONES_X + DIGIT_WIDTH + 8, ONES_Y + DIGIT_HEIGHT + 8,
                           WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

        // Draw new digits
        drawDigit(TENS_X, TENS_Y, tens);
        drawDigit(ONES_X, ONES_Y, ones);

        // Partial refresh both digits
        partialRefresh(TENS_X - 5, TENS_Y - 5,
                      (ONES_X - TENS_X) + DIGIT_WIDTH + 10,
                      DIGIT_HEIGHT + 10);

        lastTens = tens;
        lastOnes = ones;
    }
}

void updateProgressSquares() {
    uint8_t currentSecond = elapsedSeconds % 60;

    if (currentSecond != lastSecond) {
        // Clear all squares
        for (uint8_t i = 0; i < 60; i++) {
            uint16_t x = squarePositions[i].x;
            uint16_t y = squarePositions[i].y;
            Paint_DrawRectangle(x, y, x + PROGRESS_SIZE, y + PROGRESS_SIZE,
                              WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        }

        // Redraw filled squares
        for (uint8_t i = 0; i < currentSecond; i++) {
            drawProgressSquare(i, true);
        }

        // Draw empty squares
        for (uint8_t i = currentSecond; i < 60; i++) {
            drawProgressSquare(i, false);
        }

        // Partial refresh border area
        partialRefresh(BORDER_X, BORDER_Y, BORDER_W, BORDER_H);

        lastSecond = currentSecond;
    }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("  SIMPLE COUNTDOWN TIMER");
    Serial.println("========================================");

    // Display
    if (DEV_Module_Init() != 0) {
        Serial.println("ERROR: Display init failed!");
        while(1) delay(1000);
    }

    EPD_4IN2_V2_Init();

    UWORD size = ((SCREEN_WIDTH % 8 == 0) ? (SCREEN_WIDTH / 8) : (SCREEN_WIDTH / 8 + 1)) * SCREEN_HEIGHT;
    BlackImage = (UBYTE*)malloc(size);

    if (!BlackImage) {
        Serial.println("ERROR: Buffer allocation failed!");
        while(1) delay(1000);
    }

    EPD_4IN2_V2_Clear();
    delay(500);

    Paint_NewImage(BlackImage, SCREEN_WIDTH, SCREEN_HEIGHT, 0, WHITE);
    Paint_SelectImage(BlackImage);

    // Calculate square positions
    calculateSquarePositions();

    // Draw initial screen
    drawFullScreen();

    Serial.println("========================================");
    Serial.println("Timer started - counting down from 05:00");
    Serial.println("========================================\n");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
    uint32_t now = millis();

    // Timer countdown
    if (isRunning && (now - lastUpdate >= 1000)) {
        lastUpdate = now;

        if (remainingSeconds > 0) {
            remainingSeconds--;
            elapsedSeconds++;
            updateDigits();
            updateProgressSquares();
            Serial.printf("Time: %02d:%02d (elapsed: %ds)\n",
                         remainingSeconds / 60, remainingSeconds % 60, elapsedSeconds);
        } else {
            isRunning = false;
            Serial.println("\n*** TIMER COMPLETE! ***\n");
        }
    }

    delay(50);
}
