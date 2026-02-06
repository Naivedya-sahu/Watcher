/**
 * POMODORO TIMER v2 - ESP32-S3 + GxEPD2 + Buzzer
 *
 * Features:
 * - GxEPD2 library with GDEY042T81 driver (400x300 B&W)
 * - Work/Break cycles (configurable work + 5 min break)
 * - Buzzer alerts on timer completion
 * - Edge detection button algorithm (switch_observe style)
 * - Clean partial refresh UI
 *
 * Button Controls:
 * - SW1 (GPIO 35): Start/Pause toggle
 * - SW2 (GPIO 36): Mode cycle (work duration: 15, 20, 25, 30 min)
 * - SW3 (GPIO 37): Reset to initial state
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

// ============================================================
// PIN CONFIGURATION
// ============================================================
// E-Paper Display (SPI)
#define EPD_CS    10
#define EPD_DC    15
#define EPD_RST   16
#define EPD_BUSY  17
#define EPD_MOSI  11
#define EPD_SCLK  12

// Buttons (active-low with external pull-ups)
#define SW1_PIN   35   // Start/Pause
#define SW2_PIN   36   // Mode
#define SW3_PIN   37   // Reset

// Buzzer
#define BUZZER_PIN 38  // PWM capable GPIO

// RTC I2C
#define RTC_SDA   8
#define RTC_SCL   9
#define DS3231_ADDR 0x68

// ============================================================
// DISPLAY CONFIGURATION
// ============================================================
#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 300

// GxEPD2 display instance - GDEY042T81 (400x300)
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(
    GxEPD2_420_GDEY042T81(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

// ============================================================
// LAYOUT CONSTANTS
// ============================================================
// Timer display area
#define TIMER_CENTER_X   200
#define TIMER_CENTER_Y   130

// Progress ring (60 squares around border)
#define PROGRESS_SIZE    10
#define BORDER_X         59
#define BORDER_Y         49
#define BORDER_W         282
#define BORDER_H         202

// Status/mode area
#define STATUS_Y         40
#define MODE_LABEL_Y     230
#define BUTTON_LABEL_Y   275

// ============================================================
// TIMER STATES
// ============================================================
enum TimerState {
    STATE_IDLE,
    STATE_WORK,
    STATE_BREAK,
    STATE_PAUSED,
    STATE_COMPLETE
};

enum TimerPhase {
    PHASE_WORK,
    PHASE_BREAK
};

// ============================================================
// GLOBALS
// ============================================================
// Timer modes (work duration in minutes)
const uint8_t WORK_MODES[] = {15, 20, 25, 30};
const uint8_t NUM_MODES = 4;
const uint8_t BREAK_DURATION = 5;  // 5 minute break

uint8_t currentModeIndex = 2;  // Default: 25 minutes
TimerState timerState = STATE_IDLE;
TimerPhase currentPhase = PHASE_WORK;

uint16_t remainingSeconds = 25 * 60;
uint16_t totalSeconds = 25 * 60;
uint16_t elapsedSeconds = 0;
uint32_t lastTickMs = 0;

// Completed pomodoros counter
uint8_t pomodorosCompleted = 0;

// Display tracking for partial refresh
uint8_t lastDisplayMinutes = 255;
uint8_t lastDisplaySeconds = 255;
uint8_t lastProgressSquare = 255;
TimerState lastDisplayState = STATE_COMPLETE;
TimerPhase lastDisplayPhase = PHASE_WORK;

// Progress square positions
struct SquarePos { uint16_t x, y; };
SquarePos squarePositions[60];

// ============================================================
// BUTTON STATE (switch_observe style edge detection)
// ============================================================
struct ButtonState {
    uint8_t pin;
    uint8_t lastRaw;
    uint8_t stableState;
    uint32_t lastTransitionUs;
    uint32_t lastDebounceUs;
    bool pendingPress;
};

ButtonState buttons[3];
const uint32_t DEBOUNCE_US = 60000;  // 60ms debounce

// ============================================================
// BUZZER FUNCTIONS
// ============================================================
void buzzerInit() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void buzzerBeep(uint16_t frequency, uint16_t durationMs) {
    if (frequency == 0) return;

    uint32_t period = 1000000UL / frequency;
    uint32_t halfPeriod = period / 2;
    uint32_t cycles = (uint32_t)frequency * durationMs / 1000;

    for (uint32_t i = 0; i < cycles; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delayMicroseconds(halfPeriod);
        digitalWrite(BUZZER_PIN, LOW);
        delayMicroseconds(halfPeriod);
    }
}

void buzzerWorkComplete() {
    // Three rising beeps for work complete
    buzzerBeep(1000, 150);
    delay(50);
    buzzerBeep(1200, 150);
    delay(50);
    buzzerBeep(1500, 200);
}

void buzzerBreakComplete() {
    // Two short beeps for break complete
    buzzerBeep(800, 100);
    delay(100);
    buzzerBeep(800, 100);
}

void buzzerButtonPress() {
    // Quick click feedback
    buzzerBeep(2000, 20);
}

// ============================================================
// RTC FUNCTIONS
// ============================================================
bool rtcAvailable = false;

uint8_t bcdToDec(uint8_t val) { return (val / 16 * 10) + (val % 16); }

void rtcInit() {
    Wire.begin(RTC_SDA, RTC_SCL);
    Wire.beginTransmission(DS3231_ADDR);
    rtcAvailable = (Wire.endTransmission() == 0);
    Serial.printf("RTC: %s\n", rtcAvailable ? "detected" : "not found (using millis)");
}

uint32_t getSecondsTick() {
    // Use millis for timing (RTC optional for persistence)
    return millis() / 1000;
}

// ============================================================
// BUTTON FUNCTIONS (switch_observe style)
// ============================================================
void buttonsInit() {
    uint8_t pins[3] = {SW1_PIN, SW2_PIN, SW3_PIN};

    for (int i = 0; i < 3; i++) {
        pinMode(pins[i], INPUT);  // External pull-ups
        buttons[i].pin = pins[i];
        buttons[i].lastRaw = digitalRead(pins[i]);
        buttons[i].stableState = buttons[i].lastRaw;
        buttons[i].lastTransitionUs = micros();
        buttons[i].lastDebounceUs = micros();
        buttons[i].pendingPress = false;
    }

    Serial.printf("Buttons initialized: SW1=%d SW2=%d SW3=%d\n",
                  buttons[0].stableState, buttons[1].stableState, buttons[2].stableState);
}

// Edge detection with microsecond timestamps (switch_observe algorithm)
int checkButtons() {
    uint32_t nowUs = micros();

    for (int i = 0; i < 3; i++) {
        uint8_t rawState = digitalRead(buttons[i].pin);

        // Detect raw edge
        if (rawState != buttons[i].lastRaw) {
            Serial.printf("[%lu] SW%d %d->%d\n", nowUs, i+1, buttons[i].lastRaw, rawState);
            buttons[i].lastRaw = rawState;
            buttons[i].lastTransitionUs = nowUs;
        }

        // Check if transition is stable (debounced)
        if (rawState != buttons[i].stableState) {
            if ((nowUs - buttons[i].lastTransitionUs) > DEBOUNCE_US) {
                buttons[i].stableState = rawState;

                // Trigger on falling edge (button press, active-low)
                if (rawState == LOW) {
                    Serial.printf("[PRESS] SW%d confirmed at %lu us\n", i+1, nowUs);
                    buzzerButtonPress();
                    return i + 1;  // Return button number (1, 2, or 3)
                }
            }
        }
    }

    return 0;  // No button press
}

// ============================================================
// CALCULATE PROGRESS SQUARE POSITIONS
// ============================================================
void calculateSquarePositions() {
    uint8_t idx = 0;

    // Top edge - 18 squares (left to right)
    for (uint8_t i = 0; i < 18; i++) {
        squarePositions[idx].x = BORDER_X + (i * 16);
        squarePositions[idx].y = BORDER_Y;
        idx++;
    }

    // Right edge - 12 squares (top to bottom)
    for (uint8_t i = 1; i <= 12; i++) {
        squarePositions[idx].x = BORDER_X + BORDER_W - PROGRESS_SIZE;
        squarePositions[idx].y = BORDER_Y + (i * 16);
        idx++;
    }

    // Bottom edge - 18 squares (right to left)
    for (uint8_t i = 0; i < 18; i++) {
        squarePositions[idx].x = BORDER_X + BORDER_W - PROGRESS_SIZE - (i * 16);
        squarePositions[idx].y = BORDER_Y + BORDER_H - PROGRESS_SIZE;
        idx++;
    }

    // Left edge - 12 squares (bottom to top)
    for (uint8_t i = 1; i <= 12; i++) {
        squarePositions[idx].x = BORDER_X;
        squarePositions[idx].y = BORDER_Y + BORDER_H - PROGRESS_SIZE - (i * 16);
        idx++;
    }
}

// ============================================================
// DISPLAY FUNCTIONS
// ============================================================
void drawProgressSquares(uint8_t filledCount) {
    for (uint8_t i = 0; i < 60; i++) {
        uint16_t x = squarePositions[i].x;
        uint16_t y = squarePositions[i].y;

        if (i < filledCount) {
            display.fillRect(x, y, PROGRESS_SIZE, PROGRESS_SIZE, GxEPD_BLACK);
        } else {
            display.drawRect(x, y, PROGRESS_SIZE, PROGRESS_SIZE, GxEPD_BLACK);
        }
    }
}

void drawTimerDigits(uint8_t minutes, uint8_t seconds) {
    char timeStr[6];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", minutes, seconds);

    display.setFont(&FreeSansBold24pt7b);
    display.setTextColor(GxEPD_BLACK);

    // Calculate text width for centering
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);

    int16_t x = TIMER_CENTER_X - w / 2;
    int16_t y = TIMER_CENTER_Y + h / 2;

    display.setCursor(x, y);
    display.print(timeStr);
}

void drawStatusBar() {
    display.setFont(&FreeSansBold12pt7b);
    display.setTextColor(GxEPD_BLACK);

    const char* statusText;
    switch (timerState) {
        case STATE_IDLE:
            statusText = "READY";
            break;
        case STATE_WORK:
            statusText = "FOCUS TIME";
            break;
        case STATE_BREAK:
            statusText = "BREAK TIME";
            break;
        case STATE_PAUSED:
            statusText = (currentPhase == PHASE_WORK) ? "PAUSED - WORK" : "PAUSED - BREAK";
            break;
        case STATE_COMPLETE:
            statusText = "COMPLETE!";
            break;
    }

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(statusText, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(TIMER_CENTER_X - w / 2, STATUS_Y);
    display.print(statusText);
}

void drawModeLabel() {
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_BLACK);

    char modeStr[32];
    snprintf(modeStr, sizeof(modeStr), "%d min work / %d min break",
             WORK_MODES[currentModeIndex], BREAK_DURATION);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(modeStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(TIMER_CENTER_X - w / 2, MODE_LABEL_Y);
    display.print(modeStr);

    // Draw pomodoro count
    if (pomodorosCompleted > 0) {
        char countStr[16];
        snprintf(countStr, sizeof(countStr), "Completed: %d", pomodorosCompleted);
        display.getTextBounds(countStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(TIMER_CENTER_X - w / 2, MODE_LABEL_Y + 18);
        display.print(countStr);
    }
}

void drawButtonLabels() {
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_BLACK);

    // SW1 label
    const char* sw1Label = (timerState == STATE_WORK || timerState == STATE_BREAK) ? "PAUSE" : "START";
    display.setCursor(30, BUTTON_LABEL_Y);
    display.print(sw1Label);

    // SW2 label
    display.setCursor(170, BUTTON_LABEL_Y);
    display.print("MODE");

    // SW3 label
    display.setCursor(310, BUTTON_LABEL_Y);
    display.print("RESET");
}

// Full screen draw
void drawFullScreen() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Draw title
        display.setFont(&FreeSansBold12pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(140, 25);
        display.print("POMODORO");

        // Draw status
        drawStatusBar();

        // Draw timer digits
        uint8_t mins = remainingSeconds / 60;
        uint8_t secs = remainingSeconds % 60;
        drawTimerDigits(mins, secs);

        // Draw progress ring
        uint8_t progressSquares = (elapsedSeconds % 60);
        drawProgressSquares(progressSquares);

        // Draw mode label
        drawModeLabel();

        // Draw button labels
        drawButtonLabels();

    } while (display.nextPage());

    // Update tracking
    lastDisplayMinutes = remainingSeconds / 60;
    lastDisplaySeconds = remainingSeconds % 60;
    lastProgressSquare = elapsedSeconds % 60;
    lastDisplayState = timerState;
    lastDisplayPhase = currentPhase;

    Serial.println("Full screen drawn");
}

// Partial refresh for timer area only
void updateTimerPartial() {
    uint8_t mins = remainingSeconds / 60;
    uint8_t secs = remainingSeconds % 60;

    if (mins == lastDisplayMinutes && secs == lastDisplaySeconds) return;

    // Timer region (centered digits)
    int16_t x = 100;
    int16_t y = TIMER_CENTER_Y - 30;
    int16_t w = 200;
    int16_t h = 70;

    display.setPartialWindow(x, y, w, h);
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        drawTimerDigits(mins, secs);
    } while (display.nextPage());

    lastDisplayMinutes = mins;
    lastDisplaySeconds = secs;
}

// Partial refresh for progress ring
void updateProgressPartial() {
    uint8_t currentSquare = elapsedSeconds % 60;

    if (currentSquare == lastProgressSquare) return;

    display.setPartialWindow(BORDER_X - 2, BORDER_Y - 2, BORDER_W + 4, BORDER_H + 4);
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        drawProgressSquares(currentSquare);
    } while (display.nextPage());

    lastProgressSquare = currentSquare;
}

// Partial refresh for status bar
void updateStatusPartial() {
    if (timerState == lastDisplayState && currentPhase == lastDisplayPhase) return;

    int16_t x = 50;
    int16_t y = STATUS_Y - 20;
    int16_t w = 300;
    int16_t h = 30;

    display.setPartialWindow(x, y, w, h);
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        drawStatusBar();
    } while (display.nextPage());

    lastDisplayState = timerState;
    lastDisplayPhase = currentPhase;
}

// ============================================================
// TIMER LOGIC
// ============================================================
void startTimer() {
    if (timerState == STATE_IDLE || timerState == STATE_COMPLETE) {
        // Start fresh work session
        currentPhase = PHASE_WORK;
        totalSeconds = WORK_MODES[currentModeIndex] * 60;
        remainingSeconds = totalSeconds;
        elapsedSeconds = 0;
        timerState = STATE_WORK;
    } else if (timerState == STATE_PAUSED) {
        // Resume
        timerState = (currentPhase == PHASE_WORK) ? STATE_WORK : STATE_BREAK;
    }

    lastTickMs = millis();
    Serial.printf("Timer started: %s phase, %d seconds\n",
                  currentPhase == PHASE_WORK ? "WORK" : "BREAK", remainingSeconds);

    drawFullScreen();
}

void pauseTimer() {
    if (timerState == STATE_WORK || timerState == STATE_BREAK) {
        timerState = STATE_PAUSED;
        Serial.println("Timer paused");
        updateStatusPartial();
    }
}

void resetTimer() {
    timerState = STATE_IDLE;
    currentPhase = PHASE_WORK;
    totalSeconds = WORK_MODES[currentModeIndex] * 60;
    remainingSeconds = totalSeconds;
    elapsedSeconds = 0;

    Serial.println("Timer reset");
    drawFullScreen();
}

void cycleMode() {
    if (timerState != STATE_IDLE && timerState != STATE_COMPLETE) {
        Serial.println("Cannot change mode while running");
        return;
    }

    currentModeIndex = (currentModeIndex + 1) % NUM_MODES;
    totalSeconds = WORK_MODES[currentModeIndex] * 60;
    remainingSeconds = totalSeconds;
    elapsedSeconds = 0;

    Serial.printf("Mode changed to %d minutes\n", WORK_MODES[currentModeIndex]);
    drawFullScreen();
}

void timerComplete() {
    if (currentPhase == PHASE_WORK) {
        // Work session complete - start break
        pomodorosCompleted++;
        buzzerWorkComplete();

        currentPhase = PHASE_BREAK;
        totalSeconds = BREAK_DURATION * 60;
        remainingSeconds = totalSeconds;
        elapsedSeconds = 0;
        timerState = STATE_BREAK;
        lastTickMs = millis();

        Serial.printf("Work complete! Starting %d min break. Total pomodoros: %d\n",
                      BREAK_DURATION, pomodorosCompleted);
    } else {
        // Break complete - ready for next work session
        buzzerBreakComplete();

        timerState = STATE_COMPLETE;
        currentPhase = PHASE_WORK;
        totalSeconds = WORK_MODES[currentModeIndex] * 60;
        remainingSeconds = totalSeconds;
        elapsedSeconds = 0;

        Serial.println("Break complete! Ready for next pomodoro.");
    }

    drawFullScreen();
}

void timerTick() {
    if (timerState != STATE_WORK && timerState != STATE_BREAK) return;

    uint32_t now = millis();
    if (now - lastTickMs >= 1000) {
        lastTickMs = now;

        if (remainingSeconds > 0) {
            remainingSeconds--;
            elapsedSeconds++;

            // Update display
            updateTimerPartial();
            updateProgressPartial();

            Serial.printf("Time: %02d:%02d (%s)\n",
                         remainingSeconds / 60, remainingSeconds % 60,
                         currentPhase == PHASE_WORK ? "WORK" : "BREAK");
        } else {
            timerComplete();
        }
    }
}

// ============================================================
// BUTTON HANDLERS
// ============================================================
void handleButton(int buttonNum) {
    switch (buttonNum) {
        case 1:  // Start/Pause
            if (timerState == STATE_WORK || timerState == STATE_BREAK) {
                pauseTimer();
            } else {
                startTimer();
            }
            break;

        case 2:  // Mode
            cycleMode();
            break;

        case 3:  // Reset
            resetTimer();
            break;
    }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("  POMODORO TIMER v2 (GxEPD2 + Buzzer)");
    Serial.println("========================================");

    // Initialize buzzer
    buzzerInit();

    // Initialize buttons
    buttonsInit();

    // Initialize RTC
    rtcInit();

    // Initialize SPI for display
    SPI.begin(EPD_SCLK, -1, EPD_MOSI, EPD_CS);

    // Initialize display
    display.init(115200, true, 50, false);
    display.setRotation(0);
    display.setTextWrap(false);

    // Calculate progress square positions
    calculateSquarePositions();

    // Clear and draw initial screen
    display.clearScreen();
    delay(100);
    drawFullScreen();

    // Startup beep
    buzzerBeep(1000, 100);

    Serial.println("========================================");
    Serial.printf("Ready - %d min work / %d min break\n",
                  WORK_MODES[currentModeIndex], BREAK_DURATION);
    Serial.println("SW1: Start/Pause | SW2: Mode | SW3: Reset");
    Serial.println("========================================\n");
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
    // Check buttons (switch_observe style edge detection)
    int buttonPressed = checkButtons();
    if (buttonPressed > 0) {
        handleButton(buttonPressed);
    }

    // Timer tick
    timerTick();

    // Small delay to prevent busy-waiting
    delayMicroseconds(100);
}
