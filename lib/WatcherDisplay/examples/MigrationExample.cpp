/**
 * Migration Example: Converting pomodoro.cpp to use WatcherDisplay
 *
 * This example shows how to migrate from raw Waveshare library
 * to the WatcherDisplay wrapper.
 */

#include <WatcherDisplay.h>

// ========== BEFORE (Raw Waveshare Library) ==========

/*
// Manual buffer allocation
UBYTE *BlackImage;
UWORD Imagesize = ((EPD_4IN2_V2_WIDTH % 8 == 0) ? (EPD_4IN2_V2_WIDTH / 8) : (EPD_4IN2_V2_WIDTH / 8 + 1)) * EPD_4IN2_V2_HEIGHT;

void setup() {
    // Initialize hardware
    if (DEV_Module_Init() != 0) {
        Serial.println("Init failed!");
        return;
    }

    // Allocate buffer manually
    BlackImage = (UBYTE*)malloc(Imagesize);
    if (BlackImage == NULL) {
        Serial.println("Failed to allocate memory!");
        return;
    }

    // Initialize Paint library
    Paint_NewImage(BlackImage, EPD_4IN2_V2_WIDTH, EPD_4IN2_V2_HEIGHT, 270, UNCOLORED);
    Paint_SelectImage(BlackImage);

    // Initialize display
    EPD_4IN2_V2_Init();
    EPD_4IN2_V2_Clear();
}

// Manual partial refresh with counter tracking
int updatesSinceFullRefresh = 0;
const int FULL_REFRESH_INTERVAL = 5;

void updateDisplay() {
    // Clear region
    Paint_SelectImage(BlackImage);
    Paint_DrawRectangle(50, 50, 250, 150, UNCOLORED, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Draw content
    Paint_DrawString_EN(100, 100, "25:00", &Font24, UNCOLORED, COLORED);

    // Manual partial refresh
    partialRefresh(50, 50, 200, 100);

    // Manual full refresh cycle
    if (++updatesSinceFullRefresh >= FULL_REFRESH_INTERVAL) {
        EPD_4IN2_V2_Display(BlackImage);  // Manual full refresh
        updatesSinceFullRefresh = 0;
    }
}

// Manual partial refresh function
void partialRefresh(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    // Byte align
    uint16_t refreshX = (x / 8) * 8;
    uint16_t refreshWidth = ((w + 7) / 8) * 8;
    uint16_t refreshXEnd = refreshX + refreshWidth;
    uint16_t refreshYEnd = y + h;

    // Allocate temp buffer
    uint16_t regionBytes = (refreshWidth / 8);
    uint16_t bufferSize = regionBytes * h;
    UBYTE *buffer = (UBYTE*)malloc(bufferSize);

    // Extract region
    UWORD bytesPerRow = EPD_4IN2_V2_WIDTH / 8;
    UWORD startX = refreshX / 8;
    for (UWORD row = 0; row < h; row++) {
        UWORD srcOff = (y + row) * bytesPerRow + startX;
        UWORD dstOff = row * regionBytes;
        memcpy(&buffer[dstOff], &BlackImage[srcOff], regionBytes);
    }

    // Call EPD function
    EPD_4IN2_V2_PartialDisplay(buffer, refreshX, y, refreshXEnd, refreshYEnd);
    free(buffer);
}
*/

// ========== AFTER (WatcherDisplay Library) ==========

// Simple initialization
WatcherDisplay display;  // Auto-manages buffers and hybrid refresh

void setup() {
    Serial.begin(115200);

    // One-line initialization
    if (!display.begin()) {
        Serial.println("Init failed!");
        return;
    }

    // One-line clear
    display.clear();
}

// Simple update - hybrid refresh handled automatically
void updateDisplay() {
    // Clear region
    display.clearRegion(50, 50, 200, 100);

    // Draw content
    display.drawText(100, 100, "25:00", &Font24, true);

    // Auto-hybrid refresh (no manual counter tracking needed!)
    display.updateRegion(50, 50, 200, 100);
    // That's it! Library automatically:
    // - Handles byte alignment
    // - Allocates/frees temp buffers
    // - Tracks refresh counter
    // - Does full refresh when needed
}

// ========== COMPLETE TIMER EXAMPLE ==========

int minutes = 25;
int seconds = 0;
bool isRunning = false;
unsigned long lastUpdate = 0;

void loop() {
    // Button handling (simplified)
    if (digitalRead(35) == LOW) {  // Start/pause button
        isRunning = !isRunning;
        delay(200);  // Debounce
    }

    // Timer countdown
    if (isRunning && (millis() - lastUpdate >= 1000)) {
        lastUpdate = millis();

        // Countdown logic
        if (seconds == 0) {
            if (minutes > 0) {
                minutes--;
                seconds = 59;
            }
        } else {
            seconds--;
        }

        // Update display
        updateTimer();
    }
}

void updateTimer() {
    // Clear timer area
    display.clearRegion(50, 60, 300, 80);

    // Draw MM:SS with 7-segment digits
    int min1 = minutes / 10;
    int min2 = minutes % 10;
    int sec1 = seconds / 10;
    int sec2 = seconds % 10;

    display.draw7SegmentDigit(50, 60, min1, 30, 5, COLORED);
    display.draw7SegmentDigit(110, 60, min2, 30, 5, COLORED);

    // Colon
    display.drawCircle(175, 80, 4, COLORED, true);
    display.drawCircle(175, 100, 4, COLORED, true);

    display.draw7SegmentDigit(190, 60, sec1, 30, 5, COLORED);
    display.draw7SegmentDigit(250, 60, sec2, 30, 5, COLORED);

    // Auto-hybrid refresh - no counter tracking needed!
    display.updateRegion(50, 60, 300, 80);
}

// ========== CODE REDUCTION COMPARISON ==========

/*
BEFORE (Raw Waveshare):
  - Manual buffer allocation: ~15 lines
  - Manual Paint initialization: ~5 lines
  - Manual partialRefresh function: ~30 lines
  - Manual refresh counter tracking: ~5 lines
  - Manual byte alignment: ~10 lines
  - Total: ~65 lines of boilerplate

AFTER (WatcherDisplay):
  - display.begin(): 1 line
  - display.updateRegion(): 1 line
  - Total: ~2 lines (97% reduction!)

BENEFITS:
  ✅ 97% less boilerplate code
  ✅ No manual buffer management
  ✅ No manual refresh counter tracking
  ✅ No manual byte alignment
  ✅ Automatic ghosting prevention
  ✅ Cleaner, more maintainable code
  ✅ Easy to extend with new UI features
*/
