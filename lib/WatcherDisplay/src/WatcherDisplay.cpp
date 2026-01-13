/**
 * WatcherDisplay - Implementation
 *
 * High-level wrapper for Waveshare 4.2" V2 E-Paper display
 * Implements hybrid refresh strategy to fix partial refresh ghosting
 */

#include "WatcherDisplay.h"
#include <stdlib.h>
#include <string.h>

// ========== Constructor & Initialization ==========

WatcherDisplay::WatcherDisplay(uint8_t fullRefreshInterval)
    : screenBuffer(nullptr),
      partialRefreshCount(0),
      fullRefreshInterval(fullRefreshInterval),
      autoFullRefreshEnabled(true),
      initialized(false) {
}

bool WatcherDisplay::begin(bool fastInit) {
    // Initialize hardware
    if (DEV_Module_Init() != 0) {
        Serial.println("E-Paper hardware init failed!");
        return false;
    }

    // Allocate main screen buffer
    screenBuffer = (UBYTE*)malloc(BUFFER_SIZE);
    if (!screenBuffer) {
        Serial.println("Failed to allocate screen buffer!");
        return false;
    }

    // Initialize Paint library with our buffer
    Paint_NewImage(screenBuffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, 270, UNCOLORED);
    Paint_SelectImage(screenBuffer);

    // Initialize display
    if (fastInit) {
        EPD_4IN2_V2_Init_Fast(FULL_REFRESH);
    } else {
        EPD_4IN2_V2_Init();
    }

    initialized = true;
    partialRefreshCount = 0;

    Serial.println("WatcherDisplay initialized successfully");
    return true;
}

void WatcherDisplay::clear(uint16_t color) {
    if (!initialized || !screenBuffer) return;

    Paint_SelectImage(screenBuffer);
    Paint_Clear(color);
    EPD_4IN2_V2_Display(screenBuffer);
    partialRefreshCount = 0;

    Serial.println("Display cleared");
}

void WatcherDisplay::sleep() {
    if (!initialized) return;
    EPD_4IN2_V2_Sleep();
    Serial.println("Display sleeping");
}

// ========== Display Update Methods ==========

void WatcherDisplay::fullRefresh() {
    if (!initialized || !screenBuffer) return;

    unsigned long startTime = millis();
    EPD_4IN2_V2_Display(screenBuffer);
    partialRefreshCount = 0;

    unsigned long elapsed = millis() - startTime;
    Serial.printf("Full refresh complete (%lu ms)\n", elapsed);
}

void WatcherDisplay::updateRegion(const UIRegion& region) {
    if (!initialized || !screenBuffer) return;

    // Perform partial refresh
    partialRefresh(region);

    // Check if we need a full refresh (hybrid strategy)
    if (autoFullRefreshEnabled && partialRefreshCount >= fullRefreshInterval) {
        Serial.printf("Auto full refresh triggered (count: %d)\n", partialRefreshCount);
        fullRefresh();
    }
}

void WatcherDisplay::updateRegion(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    updateRegion(UIRegion(x, y, width, height));
}

void WatcherDisplay::partialRefresh(const UIRegion& region) {
    if (!initialized || !screenBuffer) return;

    unsigned long startTime = millis();

    // Get byte-aligned region
    UIRegion aligned = region.getByteAligned();

    // Validate coordinates
    if (aligned.x >= DISPLAY_WIDTH || aligned.y >= DISPLAY_HEIGHT) {
        Serial.println("Invalid region coordinates");
        return;
    }

    // Clamp to display bounds
    if (aligned.right() > DISPLAY_WIDTH) {
        aligned.width = DISPLAY_WIDTH - aligned.x;
    }
    if (aligned.bottom() > DISPLAY_HEIGHT) {
        aligned.height = DISPLAY_HEIGHT - aligned.y;
    }

    // Calculate region buffer size
    uint16_t regionBytesPerRow = aligned.width / 8;
    uint16_t regionBufferSize = regionBytesPerRow * aligned.height;

    // Allocate temporary region buffer
    UBYTE* regionBuffer = (UBYTE*)malloc(regionBufferSize);
    if (!regionBuffer) {
        Serial.println("Failed to allocate region buffer!");
        return;
    }

    // Extract region data from screen buffer
    extractRegionBuffer(aligned, regionBuffer);

    // Perform partial refresh
    partialRefreshRaw(aligned, regionBuffer);

    // Cleanup
    free(regionBuffer);

    // Increment counter
    partialRefreshCount++;

    unsigned long elapsed = millis() - startTime;
    Serial.printf("Partial refresh [%d,%d,%d,%d] complete (%lu ms, count: %d)\n",
                  aligned.x, aligned.y, aligned.width, aligned.height,
                  elapsed, partialRefreshCount);
}

void WatcherDisplay::maintainDisplay() {
    if (autoFullRefreshEnabled && partialRefreshCount >= fullRefreshInterval) {
        fullRefresh();
    }
}

void WatcherDisplay::resetRefreshCounter() {
    partialRefreshCount = 0;
}

// ========== Drawing Methods ==========

void WatcherDisplay::setPixel(uint16_t x, uint16_t y, uint16_t color) {
    if (!initialized || !screenBuffer) return;
    Paint_SelectImage(screenBuffer);
    Paint_SetPixel(x, y, color);
}

void WatcherDisplay::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    if (!initialized || !screenBuffer) return;
    Paint_SelectImage(screenBuffer);
    Paint_DrawLine(x1, y1, x2, y2, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
}

void WatcherDisplay::drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                              uint16_t color, bool filled) {
    if (!initialized || !screenBuffer) return;
    Paint_SelectImage(screenBuffer);

    if (filled) {
        Paint_DrawRectangle(x, y, x + width, y + height, color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    } else {
        Paint_DrawRectangle(x, y, x + width, y + height, color, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    }
}

void WatcherDisplay::drawCircle(uint16_t x, uint16_t y, uint16_t radius, uint16_t color, bool filled) {
    if (!initialized || !screenBuffer) return;
    Paint_SelectImage(screenBuffer);

    if (filled) {
        Paint_DrawCircle(x, y, radius, color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    } else {
        Paint_DrawCircle(x, y, radius, color, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    }
}

uint16_t WatcherDisplay::drawText(uint16_t x, uint16_t y, const char* text, sFONT* font, bool colored) {
    if (!initialized || !screenBuffer) return 0;
    Paint_SelectImage(screenBuffer);

    uint16_t color = colored ? COLORED : UNCOLORED;
    Paint_DrawString_EN(x, y, text, font, UNCOLORED, color);

    // Calculate text width
    return strlen(text) * font->Width;
}

uint16_t WatcherDisplay::drawNumber(uint16_t x, uint16_t y, int number, sFONT* font, bool colored) {
    if (!initialized || !screenBuffer) return 0;

    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%d", number);
    return drawText(x, y, buffer, font, colored);
}

void WatcherDisplay::clearRegion(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    fillRegion(x, y, width, height, UNCOLORED);
}

void WatcherDisplay::fillRegion(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    if (!initialized || !screenBuffer) return;
    Paint_SelectImage(screenBuffer);
    Paint_DrawRectangle(x, y, x + width, y + height, color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// ========== Advanced Drawing Methods ==========

void WatcherDisplay::draw7SegmentDigit(uint16_t x, uint16_t y, uint8_t digit,
                                       uint16_t segmentLength, uint16_t segmentThickness,
                                       uint16_t color) {
    if (!initialized || !screenBuffer) return;
    if (digit > 9) return;

    Paint_SelectImage(screenBuffer);

    // 7-segment layout:
    //     A
    //   F   B
    //     G
    //   E   C
    //     D

    // Segment definitions (which segments to light for each digit)
    const bool segments[10][7] = {
        {1,1,1,1,1,1,0}, // 0: A,B,C,D,E,F
        {0,1,1,0,0,0,0}, // 1: B,C
        {1,1,0,1,1,0,1}, // 2: A,B,D,E,G
        {1,1,1,1,0,0,1}, // 3: A,B,C,D,G
        {0,1,1,0,0,1,1}, // 4: B,C,F,G
        {1,0,1,1,0,1,1}, // 5: A,C,D,F,G
        {1,0,1,1,1,1,1}, // 6: A,C,D,E,F,G
        {1,1,1,0,0,0,0}, // 7: A,B,C
        {1,1,1,1,1,1,1}, // 8: All
        {1,1,1,1,0,1,1}  // 9: A,B,C,D,F,G
    };

    // Draw horizontal segments (A, G, D)
    if (segments[digit][0]) { // A - top
        Paint_DrawRectangle(x + segmentThickness, y,
                          x + segmentLength + segmentThickness, y + segmentThickness,
                          color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }
    if (segments[digit][6]) { // G - middle
        Paint_DrawRectangle(x + segmentThickness, y + segmentLength,
                          x + segmentLength + segmentThickness, y + segmentLength + segmentThickness,
                          color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }
    if (segments[digit][3]) { // D - bottom
        Paint_DrawRectangle(x + segmentThickness, y + 2 * segmentLength,
                          x + segmentLength + segmentThickness, y + 2 * segmentLength + segmentThickness,
                          color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }

    // Draw vertical segments (F, B, E, C)
    if (segments[digit][5]) { // F - top left
        Paint_DrawRectangle(x, y + segmentThickness,
                          x + segmentThickness, y + segmentLength,
                          color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }
    if (segments[digit][1]) { // B - top right
        Paint_DrawRectangle(x + segmentLength + segmentThickness, y + segmentThickness,
                          x + segmentLength + 2 * segmentThickness, y + segmentLength,
                          color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }
    if (segments[digit][4]) { // E - bottom left
        Paint_DrawRectangle(x, y + segmentLength + segmentThickness,
                          x + segmentThickness, y + 2 * segmentLength,
                          color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }
    if (segments[digit][2]) { // C - bottom right
        Paint_DrawRectangle(x + segmentLength + segmentThickness, y + segmentLength + segmentThickness,
                          x + segmentLength + 2 * segmentThickness, y + 2 * segmentLength,
                          color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }
}

void WatcherDisplay::drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                                     uint8_t progress, bool filled) {
    if (!initialized || !screenBuffer) return;
    if (progress > 100) progress = 100;

    Paint_SelectImage(screenBuffer);

    // Draw outer border
    Paint_DrawRectangle(x, y, x + width, y + height, COLORED, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

    // Calculate fill width
    uint16_t fillWidth = ((width - 4) * progress) / 100;

    if (filled) {
        // Draw filled progress
        if (fillWidth > 0) {
            Paint_DrawRectangle(x + 2, y + 2, x + 2 + fillWidth, y + height - 2,
                              COLORED, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        }
    } else {
        // Draw segmented progress (10 segments)
        uint16_t segmentWidth = (width - 4) / 10;
        uint16_t segmentGap = 2;
        uint8_t filledSegments = (progress + 9) / 10; // Round up

        for (uint8_t i = 0; i < filledSegments && i < 10; i++) {
            uint16_t segX = x + 2 + i * (segmentWidth + segmentGap);
            Paint_DrawRectangle(segX, y + 2, segX + segmentWidth, y + height - 2,
                              COLORED, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        }
    }
}

void WatcherDisplay::drawBitmap(uint16_t x, uint16_t y, const unsigned char* bitmap,
                                uint16_t width, uint16_t height) {
    if (!initialized || !screenBuffer) return;
    Paint_SelectImage(screenBuffer);
    Paint_DrawImage(bitmap, x, y, width, height);
}

// ========== Buffer Management ==========

UBYTE* WatcherDisplay::createSubBuffer(uint16_t width, uint16_t height) {
    // Width must be multiple of 8
    if (width % 8 != 0) {
        width = ((width + 7) / 8) * 8;
    }

    uint32_t bufferSize = (width / 8) * height;
    UBYTE* buffer = (UBYTE*)malloc(bufferSize);

    if (buffer) {
        memset(buffer, 0xFF, bufferSize); // Initialize to white
        Serial.printf("Sub-buffer created: %dx%d (%d bytes)\n", width, height, bufferSize);
    } else {
        Serial.println("Failed to allocate sub-buffer!");
    }

    return buffer;
}

void WatcherDisplay::copySubBuffer(const UBYTE* subBuffer,
                                  uint16_t srcX, uint16_t srcY,
                                  uint16_t width, uint16_t height,
                                  uint16_t destX, uint16_t destY) {
    if (!initialized || !screenBuffer || !subBuffer) return;

    // Calculate buffer parameters
    uint16_t subBufferBytesPerRow = width / 8;
    uint16_t screenBytesPerRow = DISPLAY_WIDTH / 8;

    // Copy row by row
    for (uint16_t row = 0; row < height; row++) {
        uint16_t screenY = destY + row;
        uint16_t subY = srcY + row;

        if (screenY >= DISPLAY_HEIGHT) break;

        // Calculate offsets
        uint32_t srcOffset = (subY * subBufferBytesPerRow) + (srcX / 8);
        uint32_t destOffset = (screenY * screenBytesPerRow) + (destX / 8);

        // Copy bytes
        memcpy(&screenBuffer[destOffset], &subBuffer[srcOffset], width / 8);
    }
}

// ========== Private Helper Methods ==========

void WatcherDisplay::extractRegionBuffer(const UIRegion& region, UBYTE* destBuffer) {
    if (!screenBuffer || !destBuffer) return;

    uint16_t screenBytesPerRow = DISPLAY_WIDTH / 8;
    uint16_t regionBytesPerRow = region.width / 8;

    // Extract region row by row
    for (uint16_t row = 0; row < region.height; row++) {
        uint16_t screenY = region.y + row;
        if (screenY >= DISPLAY_HEIGHT) break;

        // Calculate offsets
        uint32_t srcOffset = (screenY * screenBytesPerRow) + (region.x / 8);
        uint32_t destOffset = row * regionBytesPerRow;

        // Copy row
        memcpy(&destBuffer[destOffset], &screenBuffer[srcOffset], regionBytesPerRow);
    }
}

void WatcherDisplay::partialRefreshRaw(const UIRegion& region, const UBYTE* regionBuffer) {
    if (!regionBuffer) return;

    // Call Waveshare partial display function
    EPD_4IN2_V2_PartialDisplay(
        (UBYTE*)regionBuffer,
        region.x,
        region.y,
        region.right(),
        region.bottom()
    );
}

// ========== Custom Font Methods ==========

uint16_t WatcherDisplay::drawTextCustom(uint16_t x, uint16_t y, const char* text,
                                       const char* fontName, uint8_t fontSize, bool colored) {
    if (!initialized || !screenBuffer || !text) return 0;

    // Get custom font from FontHandler
    sFONT* customFont = FontHandler::getInstance().getFont(fontName, fontSize);
    if (!customFont) {
        Serial.printf("ERROR: Custom font '%s' %dpx not found\n", fontName, fontSize);
        return 0;
    }

    // Use existing drawText method with custom font
    return drawText(x, y, text, customFont, colored);
}

uint16_t WatcherDisplay::drawNumberCustom(uint16_t x, uint16_t y, int number,
                                         const char* fontName, uint8_t fontSize, bool colored) {
    if (!initialized || !screenBuffer) return 0;

    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%d", number);
    return drawTextCustom(x, y, buffer, fontName, fontSize, colored);
}

// ========== Geometric Drawing Functions ==========

void WatcherDisplay::drawTriangle(uint16_t x0, uint16_t y0,
                                 uint16_t x1, uint16_t y1,
                                 uint16_t x2, uint16_t y2,
                                 uint16_t color, bool filled) {
    if (!initialized || !screenBuffer) return;
    Paint_SelectImage(screenBuffer);

    if (filled) {
        // Fill triangle using scan-line algorithm
        // Sort vertices by y-coordinate (bubble sort for 3 elements)
        if (y0 > y1) { uint16_t tmp = y0; y0 = y1; y1 = tmp; tmp = x0; x0 = x1; x1 = tmp; }
        if (y1 > y2) { uint16_t tmp = y1; y1 = y2; y2 = tmp; tmp = x1; x1 = x2; x2 = tmp; }
        if (y0 > y1) { uint16_t tmp = y0; y0 = y1; y1 = tmp; tmp = x0; x0 = x1; x1 = tmp; }

        // Draw filled triangle
        for (uint16_t y = y0; y <= y2; y++) {
            // Calculate x bounds for this scanline
            int16_t xa = x0 + (y - y0) * (int32_t)(x2 - x0) / (y2 - y0 + 1);
            int16_t xb;

            if (y < y1) {
                xb = x0 + (y - y0) * (int32_t)(x1 - x0) / (y1 - y0 + 1);
            } else {
                xb = x1 + (y - y1) * (int32_t)(x2 - x1) / (y2 - y1 + 1);
            }

            if (xa > xb) { int16_t tmp = xa; xa = xb; xb = tmp; }

            // Draw horizontal line
            Paint_DrawLine(xa, y, xb, y, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        }
    } else {
        // Draw triangle outline
        Paint_DrawLine(x0, y0, x1, y1, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(x1, y1, x2, y2, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(x2, y2, x0, y0, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }
}

void WatcherDisplay::drawPolygon(const uint16_t* points, uint8_t numPoints,
                                uint16_t color, bool filled) {
    if (!initialized || !screenBuffer || !points || numPoints < 3) return;
    Paint_SelectImage(screenBuffer);

    if (filled) {
        // Simple fill using triangulation from first vertex
        for (uint8_t i = 1; i < numPoints - 1; i++) {
            drawTriangle(
                points[0], points[1],
                points[i*2], points[i*2+1],
                points[(i+1)*2], points[(i+1)*2+1],
                color, true
            );
        }
    } else {
        // Draw polygon outline
        for (uint8_t i = 0; i < numPoints; i++) {
            uint8_t next = (i + 1) % numPoints;
            Paint_DrawLine(
                points[i*2], points[i*2+1],
                points[next*2], points[next*2+1],
                color, DOT_PIXEL_1X1, LINE_STYLE_SOLID
            );
        }
    }
}

void WatcherDisplay::drawArc(uint16_t x, uint16_t y, uint16_t radius,
                            int16_t startAngle, int16_t endAngle, uint16_t color) {
    if (!initialized || !screenBuffer || radius == 0) return;
    Paint_SelectImage(screenBuffer);

    // Normalize angles to 0-360
    while (startAngle < 0) startAngle += 360;
    while (endAngle < 0) endAngle += 360;
    startAngle %= 360;
    endAngle %= 360;

    // Handle wrap-around
    if (endAngle < startAngle) endAngle += 360;

    // Draw arc using Bresenham-like algorithm
    int16_t prevX = -1, prevY = -1;
    for (int16_t angle = startAngle; angle <= endAngle; angle++) {
        float rad = angle * PI / 180.0;
        int16_t px = x + radius * cos(rad);
        int16_t py = y - radius * sin(rad);  // Negative because Y increases downward

        if (prevX != -1) {
            Paint_DrawLine(prevX, prevY, px, py, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        }

        prevX = px;
        prevY = py;
    }
}

void WatcherDisplay::drawEllipse(uint16_t x, uint16_t y,
                                uint16_t radiusX, uint16_t radiusY,
                                uint16_t color, bool filled) {
    if (!initialized || !screenBuffer) return;
    Paint_SelectImage(screenBuffer);

    // Midpoint ellipse algorithm
    int32_t rx = radiusX;
    int32_t ry = radiusY;
    int32_t x1 = 0;
    int32_t y1 = ry;

    // Region 1
    int32_t rx2 = rx * rx;
    int32_t ry2 = ry * ry;
    int32_t p = ry2 - (rx2 * ry) + (rx2 / 4);

    while ((ry2 * x1) < (rx2 * y1)) {
        if (filled) {
            Paint_DrawLine(x - x1, y + y1, x + x1, y + y1, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            Paint_DrawLine(x - x1, y - y1, x + x1, y - y1, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        } else {
            Paint_SetPixel(x + x1, y + y1, color);
            Paint_SetPixel(x - x1, y + y1, color);
            Paint_SetPixel(x + x1, y - y1, color);
            Paint_SetPixel(x - x1, y - y1, color);
        }

        x1++;
        if (p < 0) {
            p = p + (ry2 * 2 * x1) + ry2;
        } else {
            y1--;
            p = p + (ry2 * 2 * x1) - (rx2 * 2 * y1) + ry2;
        }
    }

    // Region 2
    p = (ry2 * (x1 * x1 + x1)) + (rx2 * (y1 - 1) * (y1 - 1)) - (rx2 * ry2);

    while (y1 >= 0) {
        if (filled) {
            Paint_DrawLine(x - x1, y + y1, x + x1, y + y1, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            Paint_DrawLine(x - x1, y - y1, x + x1, y - y1, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        } else {
            Paint_SetPixel(x + x1, y + y1, color);
            Paint_SetPixel(x - x1, y + y1, color);
            Paint_SetPixel(x + x1, y - y1, color);
            Paint_SetPixel(x - x1, y - y1, color);
        }

        y1--;
        if (p > 0) {
            p = p - (rx2 * 2 * y1) + rx2;
        } else {
            x1++;
            p = p + (ry2 * 2 * x1) - (rx2 * 2 * y1) + rx2;
        }
    }
}

void WatcherDisplay::drawRoundRect(uint16_t x, uint16_t y,
                                  uint16_t width, uint16_t height,
                                  uint16_t radius, uint16_t color, bool filled) {
    if (!initialized || !screenBuffer) return;
    Paint_SelectImage(screenBuffer);

    // Clamp radius to half of smallest dimension
    if (radius > width / 2) radius = width / 2;
    if (radius > height / 2) radius = height / 2;

    if (filled) {
        // Fill center rectangle
        Paint_DrawRectangle(x + radius, y, x + width - radius, y + height,
                          color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawRectangle(x, y + radius, x + radius, y + height - radius,
                          color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawRectangle(x + width - radius, y + radius, x + width, y + height - radius,
                          color, DOT_PIXEL_1X1, DRAW_FILL_FULL);

        // Fill corners with circles
        drawCircle(x + radius, y + radius, radius, color, true);
        drawCircle(x + width - radius - 1, y + radius, radius, color, true);
        drawCircle(x + radius, y + height - radius - 1, radius, color, true);
        drawCircle(x + width - radius - 1, y + height - radius - 1, radius, color, true);
    } else {
        // Draw straight edges
        Paint_DrawLine(x + radius, y, x + width - radius, y,
                      color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(x + radius, y + height - 1, x + width - radius, y + height - 1,
                      color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(x, y + radius, x, y + height - radius,
                      color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(x + width - 1, y + radius, x + width - 1, y + height - radius,
                      color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

        // Draw corner arcs
        drawArc(x + radius, y + radius, radius, 180, 270, color);
        drawArc(x + width - radius - 1, y + radius, radius, 270, 360, color);
        drawArc(x + radius, y + height - radius - 1, radius, 90, 180, color);
        drawArc(x + width - radius - 1, y + height - radius - 1, radius, 0, 90, color);
    }
}

void WatcherDisplay::drawThickLine(uint16_t x0, uint16_t y0,
                                  uint16_t x1, uint16_t y1,
                                  uint16_t thickness, uint16_t color) {
    if (!initialized || !screenBuffer || thickness == 0) return;
    Paint_SelectImage(screenBuffer);

    if (thickness == 1) {
        Paint_DrawLine(x0, y0, x1, y1, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        return;
    }

    // Calculate perpendicular offset
    int16_t dx = x1 - x0;
    int16_t dy = y1 - y0;
    float len = sqrt(dx * dx + dy * dy);

    if (len == 0) return;

    float offsetX = -dy * thickness / (2.0 * len);
    float offsetY = dx * thickness / (2.0 * len);

    // Draw filled polygon for thick line
    uint16_t points[8] = {
        (uint16_t)(x0 + offsetX), (uint16_t)(y0 + offsetY),
        (uint16_t)(x0 - offsetX), (uint16_t)(y0 - offsetY),
        (uint16_t)(x1 - offsetX), (uint16_t)(y1 - offsetY),
        (uint16_t)(x1 + offsetX), (uint16_t)(y1 + offsetY)
    };

    drawPolygon(points, 4, color, true);
}

void WatcherDisplay::drawBezier(uint16_t x0, uint16_t y0,
                               uint16_t x1, uint16_t y1,
                               uint16_t x2, uint16_t y2,
                               uint16_t color) {
    if (!initialized || !screenBuffer) return;
    Paint_SelectImage(screenBuffer);

    // Quadratic Bezier curve using parametric equation
    uint16_t prevX = x0, prevY = y0;

    for (float t = 0.0; t <= 1.0; t += 0.02) {
        float t2 = t * t;
        float mt = 1.0 - t;
        float mt2 = mt * mt;

        uint16_t x = mt2 * x0 + 2 * mt * t * x1 + t2 * x2;
        uint16_t y = mt2 * y0 + 2 * mt * t * y1 + t2 * y2;

        Paint_DrawLine(prevX, prevY, x, y, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

        prevX = x;
        prevY = y;
    }
}

void WatcherDisplay::drawStar(uint16_t x, uint16_t y,
                             uint16_t outerRadius, uint16_t innerRadius,
                             uint8_t numPoints, uint16_t color, bool filled) {
    if (!initialized || !screenBuffer || numPoints < 3) return;
    Paint_SelectImage(screenBuffer);

    uint16_t points[20 * 2];  // Max 10 points (20 coordinates)
    if (numPoints > 10) numPoints = 10;

    uint8_t totalVertices = numPoints * 2;
    float angleStep = 2.0 * PI / totalVertices;
    float angleOffset = -PI / 2.0;  // Start at top

    // Generate star vertices (alternating outer/inner)
    for (uint8_t i = 0; i < totalVertices; i++) {
        float angle = angleOffset + i * angleStep;
        uint16_t radius = (i % 2 == 0) ? outerRadius : innerRadius;

        points[i * 2] = x + radius * cos(angle);
        points[i * 2 + 1] = y + radius * sin(angle);
    }

    drawPolygon(points, totalVertices, color, filled);
}

void WatcherDisplay::drawHexagon(uint16_t x, uint16_t y, uint16_t radius,
                                uint16_t color, bool filled) {
    if (!initialized || !screenBuffer) return;

    uint16_t points[12];
    float angleStep = PI / 3.0;  // 60 degrees

    for (uint8_t i = 0; i < 6; i++) {
        float angle = i * angleStep;
        points[i * 2] = x + radius * cos(angle);
        points[i * 2 + 1] = y + radius * sin(angle);
    }

    drawPolygon(points, 6, color, filled);
}

void WatcherDisplay::floodFill(uint16_t x, uint16_t y, uint16_t color, uint16_t boundary) {
    if (!initialized || !screenBuffer) return;

    // Simple recursive flood fill (warning: can be slow and stack-intensive)
    // Get current pixel color
    uint16_t currentColor = UNCOLORED;
    uint32_t byteIndex = (y * (DISPLAY_WIDTH / 8)) + (x / 8);
    uint8_t bitPosition = 7 - (x % 8);

    if (byteIndex >= BUFFER_SIZE) return;

    currentColor = (screenBuffer[byteIndex] & (1 << bitPosition)) ? UNCOLORED : COLORED;

    // Stop if already the target color or hit boundary
    if (currentColor == color || currentColor == boundary) return;

    // Fill this pixel
    Paint_SelectImage(screenBuffer);
    Paint_SetPixel(x, y, color);

    // Recursive fill (4-way)
    if (x > 0) floodFill(x - 1, y, color, boundary);
    if (x < DISPLAY_WIDTH - 1) floodFill(x + 1, y, color, boundary);
    if (y > 0) floodFill(x, y - 1, color, boundary);
    if (y < DISPLAY_HEIGHT - 1) floodFill(x, y + 1, color, boundary);
}
