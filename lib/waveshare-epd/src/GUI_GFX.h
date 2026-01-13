/*****************************************************************************
* | File        : GUI_GFX.h
* | Author      : Enhanced Waveshare library
* | Function    : Adafruit GFX-style interface for GUI_Paint
* | Info        : Provides Adafruit_GFX compatible API using GuiPaint backend
*----------------
* | Features:
* | - Drop-in replacement for Adafruit_GFX drawing functions
* | - Uses existing GUI_Paint implementation
* | - Full graphics primitives support
* | - Text rendering with GuiPaint fonts
* | - Compatible with GFX libraries expecting Adafruit API
******************************************************************************/
#ifndef _GUI_GFX_H_
#define _GUI_GFX_H_

#include "GUI_Paint.h"
#include "DEV_Config.h"

// Color definitions (GFX-compatible)
#define GFX_BLACK   0x0000
#define GFX_WHITE   0xFFFF
#define GFX_RED     0xF800
#define GFX_GREEN   0x07E0
#define GFX_BLUE    0x001F
#define GFX_CYAN    0x07FF
#define GFX_MAGENTA 0xF81F
#define GFX_YELLOW  0xFFE0
#define GFX_ORANGE  0xFC00

/**
 * @brief Adafruit GFX-compatible graphics class
 *
 * Provides familiar Adafruit_GFX API while using GUI_Paint backend.
 * All drawing operations map to GUI_Paint functions.
 */
class GUI_GFX {
private:
    UBYTE* _buffer;
    uint16_t _width;
    uint16_t _height;
    uint8_t _rotation;
    bool _wrap;  // Text wrapping
    int16_t _cursor_x;
    int16_t _cursor_y;
    uint16_t _text_color;
    uint16_t _text_bgcolor;
    uint8_t _text_size;
    sFONT* _font;
    bool _buffer_owned;

    // Helper: Convert 16-bit color to 1-bit B/W
    UWORD _colorTo1Bit(uint16_t color);

    // Helper: Get current font or default
    sFONT* _getFont();

public:
    // ========== Constructor / Destructor ==========

    /**
     * @brief Create GFX interface with specified dimensions
     * @param w Width in pixels
     * @param h Height in pixels
     */
    GUI_GFX(uint16_t w, uint16_t h);

    /**
     * @brief Create GFX interface with external buffer
     * @param buffer External buffer pointer
     * @param w Width in pixels
     * @param h Height in pixels
     */
    GUI_GFX(UBYTE* buffer, uint16_t w, uint16_t h);

    ~GUI_GFX();

    // ========== Initialization ==========

    /**
     * @brief Initialize GFX (allocates buffer if not provided)
     * @return true if successful
     */
    bool begin();

    /**
     * @brief Clean up resources
     */
    void end();

    // ========== Basic Drawing ==========

    /**
     * @brief Draw a single pixel
     * @param x X coordinate
     * @param y Y coordinate
     * @param color 16-bit color (converted to B/W)
     */
    void drawPixel(int16_t x, int16_t y, uint16_t color);

    /**
     * @brief Draw a line
     * @param x0 Start X
     * @param y0 Start Y
     * @param x1 End X
     * @param y1 End Y
     * @param color Line color
     */
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

    /**
     * @brief Draw a fast vertical line (optimized)
     */
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);

    /**
     * @brief Draw a fast horizontal line (optimized)
     */
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);

    /**
     * @brief Draw a rectangle outline
     */
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

    /**
     * @brief Draw a filled rectangle
     */
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

    /**
     * @brief Fill entire screen with color
     */
    void fillScreen(uint16_t color);

    /**
     * @brief Draw a circle outline
     */
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

    /**
     * @brief Draw a filled circle
     */
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

    /**
     * @brief Draw a triangle outline
     */
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                      int16_t x2, int16_t y2, uint16_t color);

    /**
     * @brief Draw a filled triangle
     */
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                      int16_t x2, int16_t y2, uint16_t color);

    /**
     * @brief Draw a rounded rectangle outline
     */
    void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                       int16_t r, uint16_t color);

    /**
     * @brief Draw a filled rounded rectangle
     */
    void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                       int16_t r, uint16_t color);

    // ========== Text Drawing ==========

    /**
     * @brief Set text cursor position
     */
    void setCursor(int16_t x, int16_t y);

    /**
     * @brief Get cursor X position
     */
    int16_t getCursorX() const { return _cursor_x; }

    /**
     * @brief Get cursor Y position
     */
    int16_t getCursorY() const { return _cursor_y; }

    /**
     * @brief Set text color (foreground only)
     */
    void setTextColor(uint16_t color);

    /**
     * @brief Set text color with background
     */
    void setTextColor(uint16_t fg, uint16_t bg);

    /**
     * @brief Set text size (scale factor)
     */
    void setTextSize(uint8_t size);

    /**
     * @brief Enable/disable text wrapping
     */
    void setTextWrap(bool wrap);

    /**
     * @brief Set font (GuiPaint sFONT*)
     */
    void setFont(sFONT* font);

    /**
     * @brief Get current font
     */
    sFONT* getFont() { return _font; }

    /**
     * @brief Print a single character
     */
    size_t write(uint8_t c);

    /**
     * @brief Print a string
     */
    size_t print(const char* str);

    /**
     * @brief Print a string with newline
     */
    size_t println(const char* str);

    /**
     * @brief Print a number
     */
    size_t print(int32_t num);

    /**
     * @brief Print a number with newline
     */
    size_t println(int32_t num);

    /**
     * @brief Get text bounds for a string
     */
    void getTextBounds(const char* str, int16_t x, int16_t y,
                       int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);

    // ========== Rotation and Display Info ==========

    /**
     * @brief Set display rotation (0-3)
     */
    void setRotation(uint8_t rotation);

    /**
     * @brief Get current rotation
     */
    uint8_t getRotation() const { return _rotation; }

    /**
     * @brief Get display width (accounting for rotation)
     */
    uint16_t width() const;

    /**
     * @brief Get display height (accounting for rotation)
     */
    uint16_t height() const;

    // ========== Buffer Access ==========

    /**
     * @brief Get pointer to frame buffer
     */
    UBYTE* getBuffer() { return _buffer; }

    /**
     * @brief Get buffer size in bytes
     */
    uint32_t getBufferSize() const {
        return (uint32_t)(_width * _height) / 8;
    }

    // ========== Advanced Drawing ==========

    /**
     * @brief Draw XBM (X BitMap) image
     * @param x X position
     * @param y Y position
     * @param bitmap Bitmap data
     * @param w Bitmap width
     * @param h Bitmap height
     * @param color Draw color
     */
    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap,
                    int16_t w, int16_t h, uint16_t color);

    /**
     * @brief Draw XBM with background color
     */
    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap,
                    int16_t w, int16_t h, uint16_t color, uint16_t bg);

    /**
     * @brief Draw a character at position
     * @param x X position
     * @param y Y position
     * @param c Character to draw
     * @param color Foreground color
     * @param bg Background color
     * @param size Text size multiplier
     */
    void drawChar(int16_t x, int16_t y, unsigned char c,
                  uint16_t color, uint16_t bg, uint8_t size);

    // ========== GuiPaint Integration ==========

    /**
     * @brief Get underlying Paint object for advanced operations
     */
    PAINT* getPaint() { return &Paint; }

    /**
     * @brief Sync with GUI_Paint (call after direct Paint operations)
     */
    void syncWithPaint();
};

#endif // _GUI_GFX_H_
