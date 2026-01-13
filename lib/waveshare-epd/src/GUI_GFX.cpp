/*****************************************************************************
* | File        : GUI_GFX.cpp
* | Author      : Enhanced Waveshare library
* | Function    : Adafruit GFX-style implementation using GUI_Paint
******************************************************************************/
#include "GUI_GFX.h"
#include <stdlib.h>
#include <string.h>

// ========== Constructor / Destructor ==========

GUI_GFX::GUI_GFX(uint16_t w, uint16_t h) :
    _buffer(nullptr),
    _width(w),
    _height(h),
    _rotation(0),
    _wrap(true),
    _cursor_x(0),
    _cursor_y(0),
    _text_color(GFX_BLACK),
    _text_bgcolor(GFX_WHITE),
    _text_size(1),
    _font(&Font20),  // Default font
    _buffer_owned(false)
{
}

GUI_GFX::GUI_GFX(UBYTE* buffer, uint16_t w, uint16_t h) :
    _buffer(buffer),
    _width(w),
    _height(h),
    _rotation(0),
    _wrap(true),
    _cursor_x(0),
    _cursor_y(0),
    _text_color(GFX_BLACK),
    _text_bgcolor(GFX_WHITE),
    _text_size(1),
    _font(&Font20),
    _buffer_owned(false)
{
}

GUI_GFX::~GUI_GFX() {
    end();
}

bool GUI_GFX::begin() {
    if (!_buffer) {
        // Allocate buffer
        uint32_t buffer_size = (_width * _height) / 8;
        _buffer = (UBYTE*)malloc(buffer_size);

        if (!_buffer) {
            return false;
        }

        _buffer_owned = true;
    }

    // Initialize Paint library
    Paint_NewImage(_buffer, _width, _height, 0, WHITE);

    return true;
}

void GUI_GFX::end() {
    if (_buffer_owned && _buffer) {
        free(_buffer);
        _buffer = nullptr;
        _buffer_owned = false;
    }
}

// ========== Helper Functions ==========

UWORD GUI_GFX::_colorTo1Bit(uint16_t color) {
    // Convert 16-bit RGB565 to 1-bit B/W
    // Simple threshold: if any color component > 50%, it's white
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;

    // Weight by intensity
    uint16_t intensity = (r * 299 + g * 587 + b * 114) / 1000;

    return (intensity > 16) ? WHITE : BLACK;
}

sFONT* GUI_GFX::_getFont() {
    if (!_font) {
        return &Font20;  // Fallback
    }
    return _font;
}

// ========== Basic Drawing ==========

void GUI_GFX::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || y < 0 || x >= width() || y >= height()) {
        return;
    }

    UWORD paint_color = _colorTo1Bit(color);
    Paint_SetPixel(x, y, paint_color);
}

void GUI_GFX::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    UWORD paint_color = _colorTo1Bit(color);
    Paint_DrawLine(x0, y0, x1, y1, paint_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
}

void GUI_GFX::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    UWORD paint_color = _colorTo1Bit(color);
    Paint_DrawVLine(x, y, h, paint_color);
}

void GUI_GFX::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    UWORD paint_color = _colorTo1Bit(color);
    Paint_DrawHLine(x, y, w, paint_color);
}

void GUI_GFX::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    UWORD paint_color = _colorTo1Bit(color);
    Paint_DrawRectangle(x, y, x + w - 1, y + h - 1, paint_color,
                        DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
}

void GUI_GFX::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    UWORD paint_color = _colorTo1Bit(color);
    Paint_FillRect(x, y, w, h, paint_color);
}

void GUI_GFX::fillScreen(uint16_t color) {
    UWORD paint_color = _colorTo1Bit(color);
    Paint_Clear(paint_color);
}

void GUI_GFX::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    UWORD paint_color = _colorTo1Bit(color);
    Paint_DrawCircle(x0, y0, r, paint_color, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
}

void GUI_GFX::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    UWORD paint_color = _colorTo1Bit(color);
    Paint_DrawCircle(x0, y0, r, paint_color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

void GUI_GFX::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                           int16_t x2, int16_t y2, uint16_t color) {
    UWORD paint_color = _colorTo1Bit(color);

    // Draw three lines
    Paint_DrawLine(x0, y0, x1, y1, paint_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(x1, y1, x2, y2, paint_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(x2, y2, x0, y0, paint_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
}

void GUI_GFX::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                            int16_t x2, int16_t y2, uint16_t color) {
    UWORD paint_color = _colorTo1Bit(color);

    // Use polygon drawing if available
    int16_t xPoints[] = {x0, x1, x2};
    int16_t yPoints[] = {y0, y1, y2};
    Paint_DrawPolygon(xPoints, yPoints, 3, paint_color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

void GUI_GFX::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                             int16_t r, uint16_t color) {
    // Draw rectangle with rounded corners (simplified)
    drawRect(x + r, y, w - 2*r, h, color);
    drawRect(x, y + r, w, h - 2*r, color);

    // Draw corner circles
    drawCircle(x + r, y + r, r, color);
    drawCircle(x + w - r - 1, y + r, r, color);
    drawCircle(x + r, y + h - r - 1, r, color);
    drawCircle(x + w - r - 1, y + h - r - 1, r, color);
}

void GUI_GFX::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                              int16_t r, uint16_t color) {
    // Fill rectangle with rounded corners
    fillRect(x + r, y, w - 2*r, h, color);
    fillRect(x, y + r, w, h - 2*r, color);

    // Fill corner circles
    fillCircle(x + r, y + r, r, color);
    fillCircle(x + w - r - 1, y + r, r, color);
    fillCircle(x + r, y + h - r - 1, r, color);
    fillCircle(x + w - r - 1, y + h - r - 1, r, color);
}

// ========== Text Drawing ==========

void GUI_GFX::setCursor(int16_t x, int16_t y) {
    _cursor_x = x;
    _cursor_y = y;
}

void GUI_GFX::setTextColor(uint16_t color) {
    _text_color = color;
    // Don't change background - transparent
}

void GUI_GFX::setTextColor(uint16_t fg, uint16_t bg) {
    _text_color = fg;
    _text_bgcolor = bg;
}

void GUI_GFX::setTextSize(uint8_t size) {
    _text_size = (size > 0) ? size : 1;
}

void GUI_GFX::setTextWrap(bool wrap) {
    _wrap = wrap;
}

void GUI_GFX::setFont(sFONT* font) {
    if (font) {
        _font = font;
    }
}

size_t GUI_GFX::write(uint8_t c) {
    sFONT* font = _getFont();

    if (c == '\n') {
        _cursor_y += font->Height * _text_size;
        _cursor_x = 0;
    } else if (c == '\r') {
        // Skip carriage return
    } else {
        UWORD fg_color = _colorTo1Bit(_text_color);
        UWORD bg_color = _colorTo1Bit(_text_bgcolor);

        // Check wrapping
        if (_wrap && (_cursor_x + font->Width * _text_size > width())) {
            _cursor_x = 0;
            _cursor_y += font->Height * _text_size;
        }

        // Draw character
        if (_text_size == 1) {
            Paint_DrawChar(_cursor_x, _cursor_y, c, font, fg_color, bg_color);
        } else {
            // Scaled character (simplified - draw multiple times)
            for (uint8_t sy = 0; sy < _text_size; sy++) {
                for (uint8_t sx = 0; sx < _text_size; sx++) {
                    Paint_DrawChar(_cursor_x + sx * font->Width,
                                   _cursor_y + sy * font->Height,
                                   c, font, fg_color, bg_color);
                }
            }
        }

        _cursor_x += font->Width * _text_size;
    }

    return 1;
}

size_t GUI_GFX::print(const char* str) {
    if (!str) return 0;

    size_t n = 0;
    while (*str) {
        n += write(*str++);
    }
    return n;
}

size_t GUI_GFX::println(const char* str) {
    size_t n = print(str);
    n += write('\n');
    return n;
}

size_t GUI_GFX::print(int32_t num) {
    UWORD fg_color = _colorTo1Bit(_text_color);
    UWORD bg_color = _colorTo1Bit(_text_bgcolor);
    sFONT* font = _getFont();

    Paint_DrawNum(_cursor_x, _cursor_y, num, font, fg_color, bg_color);

    // Update cursor (approximate)
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", (int)num);
    _cursor_x += strlen(buffer) * font->Width * _text_size;

    return strlen(buffer);
}

size_t GUI_GFX::println(int32_t num) {
    size_t n = print(num);
    n += write('\n');
    return n;
}

void GUI_GFX::getTextBounds(const char* str, int16_t x, int16_t y,
                             int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
    if (!str) return;

    sFONT* font = _getFont();
    uint16_t len = strlen(str);

    *x1 = x;
    *y1 = y;
    *w = len * font->Width * _text_size;
    *h = font->Height * _text_size;
}

// ========== Rotation and Display Info ==========

void GUI_GFX::setRotation(uint8_t rotation) {
    _rotation = rotation % 4;

    // Update Paint library
    Paint_SetRotate(_rotation * 90);
}

uint16_t GUI_GFX::width() const {
    if (_rotation == 1 || _rotation == 3) {
        return _height;
    }
    return _width;
}

uint16_t GUI_GFX::height() const {
    if (_rotation == 1 || _rotation == 3) {
        return _width;
    }
    return _height;
}

// ========== Advanced Drawing ==========

void GUI_GFX::drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap,
                          int16_t w, int16_t h, uint16_t color) {
    UWORD paint_color = _colorTo1Bit(color);

    // Draw bitmap using Paint_DrawImage
    Paint_DrawImage(bitmap, x, y, w, h);
}

void GUI_GFX::drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap,
                          int16_t w, int16_t h, uint16_t color, uint16_t bg) {
    // For B/W, just draw the bitmap
    drawBitmap(x, y, bitmap, w, h, color);
}

void GUI_GFX::drawChar(int16_t x, int16_t y, unsigned char c,
                        uint16_t color, uint16_t bg, uint8_t size) {
    UWORD fg_color = _colorTo1Bit(color);
    UWORD bg_color = _colorTo1Bit(bg);
    sFONT* font = _getFont();

    if (size == 1) {
        Paint_DrawChar(x, y, c, font, fg_color, bg_color);
    } else {
        // Scaled drawing
        for (uint8_t sy = 0; sy < size; sy++) {
            for (uint8_t sx = 0; sx < size; sx++) {
                Paint_DrawChar(x + sx * font->Width,
                               y + sy * font->Height,
                               c, font, fg_color, bg_color);
            }
        }
    }
}

// ========== GuiPaint Integration ==========

void GUI_GFX::syncWithPaint() {
    // Sync cursor and settings with Paint library
    // (Currently Paint library doesn't track cursor, so this is a no-op)
}
