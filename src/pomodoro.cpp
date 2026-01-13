/**
 * POMODORO TIMER - ESP32-S3 + E-Paper + DS3231 RTC
 * 
 * Based on simple_timer_bitmap.cpp partial refresh approach:
 * - Digits and progress squares use separate partial refresh regions
 * - Progress updates refresh entire border area (not individual squares)
 * - Buttons have their own isolated refresh region
 * 
 * Button behavior:
 * - SW1 (Start/Pause): Toggle between START and PAUSE states
 * - SW2 (Mode): Cycle through timer durations (only when stopped)
 * - SW3 (Reset): Return to initial state
 */

#include <Arduino.h>
#include <Wire.h>
#include "EPD.h"
#include "GUI_Paint.h"

// ============================================================
// SCREEN & LAYOUT (same as simple_timer_bitmap.cpp)
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

// Progress squares (60 total, 10×10 each) - EXACT from simple_timer
#define PROGRESS_SIZE 10
#define BORDER_X      59
#define BORDER_Y      49
#define BORDER_W      282
#define BORDER_H      202

// Button area (below the timer, separate from BORDER region)
#define BUTTON_Y      265
#define BTN_START_X   20
#define BTN_MODE_X    160
#define BTN_RESET_X   290

// Pin assignments (from pinouts.txt)
#define SW1_PIN       35   // Start/Pause
#define SW2_PIN       36   // Mode
#define SW3_PIN       37   // Reset
#define RTC_SDA       8    // DS3231 SDA
#define RTC_SCL       9    // DS3231 SCL
#define DS3231_ADDR   0x68 // DS3231 I2C address

// ============================================================
// GLOBALS
// ============================================================
UBYTE *BlackImage = nullptr;

// Timer modes (minutes)
const uint8_t TIMER_MODES[] = {5, 10, 15, 20, 25};
const uint8_t NUM_MODES = 5;
uint8_t timerModeIndex = 0;

// Timer state (matching simple_timer pattern)
uint16_t remainingSeconds = 5 * 60;  // 5 minutes default
uint16_t elapsedSeconds = 0;
uint8_t lastTens = 255;
uint8_t lastOnes = 255;
uint8_t lastSecond = 255;
uint32_t lastUpdate = 0;
bool isRunning = false;
uint32_t lastStartToggleMs = 0;

// Button debounce
uint8_t lastBtnState[3] = {HIGH, HIGH, HIGH};
uint32_t lastBtnDebounce[3] = {0, 0, 0};
uint8_t stableBtnState[3] = {HIGH, HIGH, HIGH};

// Progress square positions (same structure as simple_timer)
struct SquarePos { uint16_t x, y; };
SquarePos squarePositions[60];

// 7-segment digit map (same as simple_timer)
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

// ============================================================
// DS3231 RTC FUNCTIONS
// ============================================================
bool rtcAvailable = false;

uint8_t bcdToDec(uint8_t val) { return (val / 16 * 10) + (val % 16); }
uint8_t decToBcd(uint8_t val) { return (val / 10 * 16) + (val % 10); }

void rtcInit() {
    Wire.begin(RTC_SDA, RTC_SCL);
    Wire.beginTransmission(DS3231_ADDR);
    rtcAvailable = (Wire.endTransmission() == 0);
    
    if (rtcAvailable) {
        Serial.println("DS3231 RTC detected");
    } else {
        Serial.println("DS3231 RTC not found - using millis()");
    }
}

uint32_t rtcGetSeconds() {
    if (!rtcAvailable) return millis() / 1000;
    
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00);  // Start at seconds register
    Wire.endTransmission();
    Wire.requestFrom(DS3231_ADDR, 3);
    
    uint8_t ss = bcdToDec(Wire.read() & 0x7F);
    uint8_t mm = bcdToDec(Wire.read());
    uint8_t hh = bcdToDec(Wire.read() & 0x3F);
    
    return (uint32_t)hh * 3600 + (uint32_t)mm * 60 + ss;
}

// ============================================================
// 7-SEGMENT DRAWING (EXACT copy from simple_timer_bitmap.cpp)
// ============================================================

// Segment A - Top horizontal
void drawSegmentA(uint16_t x, uint16_t y, bool on) {
    if (!on) return;
    int16_t xpts[] = {(int16_t)(x+1), (int16_t)(x+70), (int16_t)(x+57), (int16_t)(x+13), (int16_t)(x+1)};
    int16_t ypts[] = {(int16_t)(y+0), (int16_t)(y+0), (int16_t)(y+11), (int16_t)(y+11), (int16_t)(y+0)};
    Paint_DrawPolygon(xpts, ypts, 5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment B - Top right vertical
void drawSegmentB(uint16_t x, uint16_t y, bool on) {
    if (!on) return;
    int16_t xpts[] = {(int16_t)(x+70), (int16_t)(x+70), (int16_t)(x+59), (int16_t)(x+59), (int16_t)(x+70)};
    int16_t ypts[] = {(int16_t)(y+3), (int16_t)(y+64), (int16_t)(y+57), (int16_t)(y+13), (int16_t)(y+3)};
    Paint_DrawPolygon(xpts, ypts, 5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment C - Bottom right vertical
void drawSegmentC(uint16_t x, uint16_t y, bool on) {
    if (!on) return;
    int16_t xpts[] = {(int16_t)(x+70), (int16_t)(x+70), (int16_t)(x+59), (int16_t)(x+59), (int16_t)(x+70)};
    int16_t ypts[] = {(int16_t)(y+67), (int16_t)(y+129), (int16_t)(y+118), (int16_t)(y+73), (int16_t)(y+67)};
    Paint_DrawPolygon(xpts, ypts, 5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment D - Bottom horizontal
void drawSegmentD(uint16_t x, uint16_t y, bool on) {
    if (!on) return;
    int16_t xpts[] = {(int16_t)(x+67), (int16_t)(x+3), (int16_t)(x+13), (int16_t)(x+57), (int16_t)(x+67)};
    int16_t ypts[] = {(int16_t)(y+130), (int16_t)(y+130), (int16_t)(y+119), (int16_t)(y+119), (int16_t)(y+130)};
    Paint_DrawPolygon(xpts, ypts, 5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment E - Bottom left vertical
void drawSegmentE(uint16_t x, uint16_t y, bool on) {
    if (!on) return;
    int16_t xpts[] = {(int16_t)(x+0), (int16_t)(x+0), (int16_t)(x+11), (int16_t)(x+11), (int16_t)(x+0)};
    int16_t ypts[] = {(int16_t)(y+129), (int16_t)(y+66), (int16_t)(y+73), (int16_t)(y+117), (int16_t)(y+129)};
    Paint_DrawPolygon(xpts, ypts, 5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment F - Top left vertical
void drawSegmentF(uint16_t x, uint16_t y, bool on) {
    if (!on) return;
    int16_t xpts[] = {(int16_t)(x+0), (int16_t)(x+0), (int16_t)(x+11), (int16_t)(x+11), (int16_t)(x+0)};
    int16_t ypts[] = {(int16_t)(y+64), (int16_t)(y+3), (int16_t)(y+13), (int16_t)(y+57), (int16_t)(y+64)};
    Paint_DrawPolygon(xpts, ypts, 5, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment G - Middle horizontal
void drawSegmentG(uint16_t x, uint16_t y, bool on) {
    if (!on) return;
    int16_t xpts[] = {(int16_t)(x+11), (int16_t)(x+59), (int16_t)(x+69), (int16_t)(x+59), (int16_t)(x+11), (int16_t)(x+2), (int16_t)(x+11)};
    int16_t ypts[] = {(int16_t)(y+60), (int16_t)(y+60), (int16_t)(y+65), (int16_t)(y+70), (int16_t)(y+70), (int16_t)(y+65), (int16_t)(y+60)};
    Paint_DrawPolygon(xpts, ypts, 7, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Draw complete digit at position (same as simple_timer)
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
// CALCULATE 60 SQUARE POSITIONS (EXACT from simple_timer)
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

// Draw progress square (same as simple_timer)
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
// PARTIAL REFRESH (EXACT from simple_timer)
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

// Draw title at top
void drawTitle() {
    Paint_DrawString_EN(130, 15, "POMODORO", &Font20, WHITE, BLACK);
}

// Draw button labels at bottom (separate region from timer)
void drawButtons() {
    Paint_DrawString_EN(BTN_START_X, BUTTON_Y, isRunning ? "PAUSE" : "START", &Font16, WHITE, BLACK);
    Paint_DrawString_EN(BTN_MODE_X, BUTTON_Y, "MODE", &Font16, WHITE, BLACK);
    Paint_DrawString_EN(BTN_RESET_X, BUTTON_Y, "RESET", &Font16, WHITE, BLACK);
}

// Full screen draw (initial or after reset)
void drawFullScreen() {
    Paint_Clear(WHITE);

    // Draw title
    drawTitle();

    // Calculate and draw digits
    uint8_t minutes = remainingSeconds / 60;
    uint8_t tens = minutes / 10;
    uint8_t ones = minutes % 10;
    drawDigit(TENS_X, TENS_Y, tens);
    drawDigit(ONES_X, ONES_Y, ones);

    // Draw all 60 progress squares
    uint8_t secondInMinute = elapsedSeconds % 60;
    for (uint8_t i = 0; i < 60; i++) {
        drawProgressSquare(i, i < secondInMinute);
    }

    // Draw buttons
    drawButtons();

    // Full display update
    EPD_4IN2_V2_Display(BlackImage);

    // Update tracking state
    lastTens = tens;
    lastOnes = ones;
    lastSecond = 255;  // Force progress update on first tick

    Serial.printf("Full screen drawn: %d%d minutes\n", tens, ones);
}

// ============================================================
// UPDATE FUNCTIONS (Partial refresh - separated regions)
// ============================================================

// Update digits only (EXACT pattern from simple_timer)
void updateDigits() {
    uint8_t minutes = remainingSeconds / 60;
    uint8_t tens = minutes / 10;
    uint8_t ones = minutes % 10;

    if (tens != lastTens || ones != lastOnes) {
        Serial.printf("Updating digits: %d%d\n", tens, ones);

        // Clear digit areas with padding (same as simple_timer)
        Paint_DrawRectangle(TENS_X - 8, TENS_Y - 8, TENS_X + DIGIT_WIDTH + 8, TENS_Y + DIGIT_HEIGHT + 8,
                           WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawRectangle(ONES_X - 8, ONES_Y - 8, ONES_X + DIGIT_WIDTH + 8, ONES_Y + DIGIT_HEIGHT + 8,
                           WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

        // Draw new digits
        drawDigit(TENS_X, TENS_Y, tens);
        drawDigit(ONES_X, ONES_Y, ones);

        // Partial refresh both digits together (same as simple_timer)
        partialRefresh(TENS_X - 5, TENS_Y - 5,
                      (ONES_X - TENS_X) + DIGIT_WIDTH + 10,
                      DIGIT_HEIGHT + 10);

        lastTens = tens;
        lastOnes = ones;
    }
}

// Update progress squares (identical to simple_timer_bitmap.cpp)
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

        // Partial refresh border area (entire ring, same as simple_timer)
        partialRefresh(BORDER_X, BORDER_Y, BORDER_W, BORDER_H);

        lastSecond = currentSecond;
    }
}

// Update Start/Pause button only (isolated refresh region)
void updateStartButton() {
    // Clear surrounding area to avoid ghosting when toggling label
    Paint_DrawRectangle(BTN_START_X - 4, BUTTON_Y - 4, BTN_START_X + 80, BUTTON_Y + 22,
                       WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Draw the new label
    Paint_DrawString_EN(BTN_START_X, BUTTON_Y, isRunning ? "PAUSE" : "START", &Font16, WHITE, BLACK);

    // Partial refresh only the button area (below BORDER region)
    partialRefresh(BTN_START_X - 8, BUTTON_Y - 6, 104, 34);
}

// ============================================================
// BUTTON HANDLERS
// ============================================================

void handleStartPause() {
    uint32_t now = millis();
    if (now - lastStartToggleMs < 250) {
        return;
    }
    lastStartToggleMs = now;

    isRunning = !isRunning;

    if (isRunning) {
        // Starting or resuming timer - set lastUpdate to NOW + buffer
        // This prevents immediate timer tick right after button press
        lastUpdate = now;
        Serial.println("Timer STARTED/RESUMED");
    } else {
        // Pausing timer
        Serial.println("Timer PAUSED");
    }

    // Update only the button display
    updateStartButton();

    // Reset lastUpdate again after button refresh to prevent immediate timer tick
    if (isRunning) {
        lastUpdate = millis();
    }
}

void handleMode() {
    // Only allow mode change when stopped
    if (isRunning) {
        Serial.println("Cannot change mode while running");
        return;
    }

    timerModeIndex = (timerModeIndex + 1) % NUM_MODES;
    remainingSeconds = TIMER_MODES[timerModeIndex] * 60;
    elapsedSeconds = 0;

    // Force partial refresh updates for digits and progress ring
    lastTens = 255;
    lastOnes = 255;
    lastSecond = 255;

    updateDigits();
    updateProgressSquares();
    updateStartButton();

    Serial.printf("Mode changed to %d minutes\n", TIMER_MODES[timerModeIndex]);
}

void handleReset() {
    // Stop timer and reset to initial state
    isRunning = false;
    remainingSeconds = TIMER_MODES[timerModeIndex] * 60;
    elapsedSeconds = 0;
    lastTens = 255;
    lastOnes = 255;
    lastSecond = 255;

    Serial.println("Timer RESET to initial state");

    // Re-initialize the display to clear any partial refresh state
    EPD_4IN2_V2_Init();
    delay(100);

    // Hardware clear the display
    EPD_4IN2_V2_Clear();
    delay(500);  // Longer delay to ensure clear completes

    // Clear the entire frame buffer to white
    UWORD bufSize = ((SCREEN_WIDTH % 8 == 0) ? (SCREEN_WIDTH / 8) : (SCREEN_WIDTH / 8 + 1)) * SCREEN_HEIGHT;
    memset(BlackImage, 0xFF, bufSize);

    // Re-select the image buffer
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);

    // Now draw fresh content
    drawFullScreen();

    Serial.println("Reset complete - display cleared and redrawn");
}

// ============================================================
// BUTTON POLLING
// ============================================================
void checkButtons() {
    uint32_t now = millis();
    const uint8_t pins[3] = {SW1_PIN, SW2_PIN, SW3_PIN};
    const uint32_t debounceMs = 60;

    for (int i = 0; i < 3; i++) {
        uint8_t rawState = digitalRead(pins[i]);

        if (rawState != lastBtnState[i]) {
            lastBtnState[i] = rawState;
            lastBtnDebounce[i] = now;
        }

        if ((now - lastBtnDebounce[i]) > debounceMs) {
            if (rawState != stableBtnState[i]) {
                stableBtnState[i] = rawState;

                if (rawState == LOW) {  // Button pressed (active low)
                    switch (i) {
                        case 0: handleStartPause(); break;
                        case 1: handleMode(); break;
                        case 2: handleReset(); break;
                    }
                }
            }
        }
    }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("  POMODORO TIMER");
    Serial.println("========================================");

    // Initialize buttons
    pinMode(SW1_PIN, INPUT);
    pinMode(SW2_PIN, INPUT);
    pinMode(SW3_PIN, INPUT);
    const uint8_t buttonPins[3] = {SW1_PIN, SW2_PIN, SW3_PIN};
    uint32_t initDebounce = millis();
    for (int i = 0; i < 3; i++) {
        uint8_t state = digitalRead(buttonPins[i]);
        lastBtnState[i] = state;
        stableBtnState[i] = state;
        lastBtnDebounce[i] = initDebounce;
    }

    // Initialize RTC
    rtcInit();

    // Initialize display
    if (DEV_Module_Init() != 0) {
        Serial.println("ERROR: Display init failed!");
        while (1) delay(1000);
    }

    EPD_4IN2_V2_Init();

    // Allocate frame buffer
    UWORD bufSize = ((SCREEN_WIDTH % 8 == 0) ? (SCREEN_WIDTH / 8) : (SCREEN_WIDTH / 8 + 1)) * SCREEN_HEIGHT;
    BlackImage = (UBYTE*)malloc(bufSize);

    if (!BlackImage) {
        Serial.println("ERROR: Buffer allocation failed!");
        while (1) delay(1000);
    }

    // Clear display
    EPD_4IN2_V2_Clear();
    delay(500);

    // Initialize paint
    Paint_NewImage(BlackImage, SCREEN_WIDTH, SCREEN_HEIGHT, 0, WHITE);
    Paint_SelectImage(BlackImage);

    // Calculate square positions (same as simple_timer)
    calculateSquarePositions();

    // Draw initial screen
    drawFullScreen();

    Serial.println("========================================");
    Serial.printf("Ready - %d minute mode\n", TIMER_MODES[timerModeIndex]);
    Serial.println("Press START to begin");
    Serial.println("========================================\n");
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
    // Check buttons
    checkButtons();

    // Timer countdown (same pattern as simple_timer)
    uint32_t now = millis();

    if (isRunning && (now - lastUpdate >= 1000)) {
        lastUpdate = now;

        if (remainingSeconds > 0) {
            remainingSeconds--;
            elapsedSeconds++;

            // Update display using separate partial refreshes
            updateDigits();
            updateProgressSquares();

            Serial.printf("Time: %02d:%02d (elapsed: %ds)\n",
                         remainingSeconds / 60, remainingSeconds % 60, elapsedSeconds);
        } else {
            // Timer complete
            isRunning = false;
            updateStartButton();
            Serial.println("\n*** TIMER COMPLETE! ***\n");
        }
    }

    delay(50);
}
