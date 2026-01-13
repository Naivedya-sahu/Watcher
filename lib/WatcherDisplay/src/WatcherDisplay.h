/**
 * WatcherDisplay - High-level E-Paper Display Wrapper
 *
 * A robust wrapper around the Waveshare EPD library that:
 * - Automatically manages hybrid refresh strategy (partial + periodic full)
 * - Fixes partial refresh ghosting through automatic full refresh cycles
 * - Provides high-level UI primitives for easy graphics rendering
 * - Manages multiple buffers for different UI regions
 * - Simplifies display updates with smart region tracking
 *
 * Hardware: Waveshare 4.2" V2 E-Paper (400x300, UC8176 controller)
 *
 * Usage:
 *   WatcherDisplay display;
 *   display.begin();
 *   display.clear();
 *   display.drawText(10, 10, "Hello", Font20, false);
 *   display.updateRegion(10, 10, 100, 30);  // Auto-hybrid refresh
 */

#ifndef WATCHER_DISPLAY_H
#define WATCHER_DISPLAY_H

#include <Arduino.h>
#include "utility/EPD_4in2_V2.h"
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "FontHandler.h"

// Display constants
#define DISPLAY_WIDTH  400
#define DISPLAY_HEIGHT 300
#define BUFFER_SIZE    15000  // (400/8) * 300 = 50 * 300

// Default refresh strategy
#define DEFAULT_FULL_REFRESH_INTERVAL 5  // Full refresh every N partial updates

/**
 * UIRegion - Represents a rectangular region on the display
 */
struct UIRegion {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;

    UIRegion() : x(0), y(0), width(0), height(0) {}
    UIRegion(uint16_t _x, uint16_t _y, uint16_t _w, uint16_t _h)
        : x(_x), y(_y), width(_w), height(_h) {}

    // Get byte-aligned region for partial refresh
    UIRegion getByteAligned() const {
        UIRegion aligned;
        aligned.x = (x / 8) * 8;
        aligned.y = y;
        aligned.width = ((width + 7) / 8) * 8;
        aligned.height = height;
        return aligned;
    }

    uint16_t right() const { return x + width; }
    uint16_t bottom() const { return y + height; }
};

/**
 * WatcherDisplay - Main display controller class
 */
class WatcherDisplay {
public:
    /**
     * Constructor
     * @param fullRefreshInterval Number of partial updates before forcing full refresh (default: 5)
     */
    WatcherDisplay(uint8_t fullRefreshInterval = DEFAULT_FULL_REFRESH_INTERVAL);

    /**
     * Initialize the display hardware and clear screen
     * @param fastInit Use fast initialization mode (default: false)
     * @return true if successful
     */
    bool begin(bool fastInit = false);

    /**
     * Clear the entire display and reset buffers
     * @param color COLORED (black) or UNCOLORED (white)
     */
    void clear(uint16_t color = UNCOLORED);

    /**
     * Put display into deep sleep mode to save power
     */
    void sleep();

    // ========== Display Update Methods ==========

    /**
     * Force a full screen refresh (slow, ~2-3 seconds, but cleans ghosting)
     */
    void fullRefresh();

    /**
     * Update a specific region with automatic hybrid refresh strategy
     * Performs partial refresh, but automatically does full refresh when needed
     * @param region The region to update
     */
    void updateRegion(const UIRegion& region);

    /**
     * Update a specific region (convenience overload)
     * @param x X coordinate
     * @param y Y coordinate
     * @param width Region width
     * @param height Region height
     */
    void updateRegion(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

    /**
     * Force a partial refresh of a region (fast, ~300-400ms, may ghost after many updates)
     * Use sparingly - prefer updateRegion() which handles ghosting automatically
     * @param region The region to update
     */
    void partialRefresh(const UIRegion& region);

    /**
     * Force a full refresh if too many partial updates have occurred
     * Call this periodically if you're managing updates manually
     */
    void maintainDisplay();

    /**
     * Reset the partial refresh counter (call after manual full refresh)
     */
    void resetRefreshCounter();

    /**
     * Get number of partial refreshes since last full refresh
     */
    uint8_t getPartialRefreshCount() const { return partialRefreshCount; }

    // ========== Drawing Methods ==========

    /**
     * Set a pixel
     * @param x X coordinate
     * @param y Y coordinate
     * @param color COLORED (black) or UNCOLORED (white)
     */
    void setPixel(uint16_t x, uint16_t y, uint16_t color);

    /**
     * Draw a line
     */
    void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

    /**
     * Draw a rectangle
     * @param filled true to fill, false for outline only
     */
    void drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                  uint16_t color, bool filled = false);

    /**
     * Draw a circle
     * @param filled true to fill, false for outline only
     */
    void drawCircle(uint16_t x, uint16_t y, uint16_t radius, uint16_t color, bool filled = false);

    /**
     * Draw text
     * @param x X coordinate
     * @param y Y coordinate
     * @param text Text string
     * @param font Font to use (Font8, Font12, Font16, Font20, Font24)
     * @param colored true for black text, false for white text
     * @return Width of rendered text in pixels
     */
    uint16_t drawText(uint16_t x, uint16_t y, const char* text, sFONT* font, bool colored = true);

    /**
     * Draw a number
     * @return Width of rendered number in pixels
     */
    uint16_t drawNumber(uint16_t x, uint16_t y, int number, sFONT* font, bool colored = true);

    /**
     * Clear a rectangular region (fill with white)
     */
    void clearRegion(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

    /**
     * Fill a region with a specific color
     */
    void fillRegion(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

    // ========== Advanced Drawing Methods ==========

    /**
     * Draw a 7-segment style digit (for large timer displays)
     * @param x X coordinate
     * @param y Y coordinate
     * @param digit Digit to draw (0-9)
     * @param segmentLength Length of each segment
     * @param segmentThickness Thickness of segments
     * @param color Segment color
     */
    void draw7SegmentDigit(uint16_t x, uint16_t y, uint8_t digit,
                           uint16_t segmentLength, uint16_t segmentThickness,
                           uint16_t color);

    /**
     * Draw a progress bar
     * @param x X coordinate
     * @param y Y coordinate
     * @param width Bar width
     * @param height Bar height
     * @param progress Progress percentage (0-100)
     * @param filled true for filled bar, false for segmented
     */
    void drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                        uint8_t progress, bool filled = true);

    /**
     * Draw a bitmap image
     * @param x X coordinate
     * @param y Y coordinate
     * @param bitmap Bitmap data
     * @param width Bitmap width
     * @param height Bitmap height
     */
    void drawBitmap(uint16_t x, uint16_t y, const unsigned char* bitmap,
                    uint16_t width, uint16_t height);

    // ========== Custom Font Methods ==========

    /**
     * Draw text with custom font (registered via FontHandler)
     * @param x X coordinate
     * @param y Y coordinate
     * @param text Text string
     * @param fontName Custom font name (e.g., "Monocraft")
     * @param fontSize Font size in pixels
     * @param colored true for black text, false for white text
     * @return Width of rendered text in pixels (0 if font not found)
     */
    uint16_t drawTextCustom(uint16_t x, uint16_t y, const char* text,
                           const char* fontName, uint8_t fontSize, bool colored = true);

    /**
     * Draw number with custom font
     * @return Width of rendered number in pixels (0 if font not found)
     */
    uint16_t drawNumberCustom(uint16_t x, uint16_t y, int number,
                             const char* fontName, uint8_t fontSize, bool colored = true);

    /**
     * Get FontHandler instance for direct font registration
     * @return Reference to FontHandler singleton
     */
    FontHandler& getFontHandler() { return FontHandler::getInstance(); }

    // ========== Geometric Drawing Functions ==========

    /**
     * Draw a triangle
     * @param x0, y0 First vertex
     * @param x1, y1 Second vertex
     * @param x2, y2 Third vertex
     * @param color Line color
     * @param filled true to fill, false for outline only
     */
    void drawTriangle(uint16_t x0, uint16_t y0,
                     uint16_t x1, uint16_t y1,
                     uint16_t x2, uint16_t y2,
                     uint16_t color, bool filled = false);

    /**
     * Draw a polygon
     * @param points Array of x,y coordinates [x0,y0,x1,y1,x2,y2,...]
     * @param numPoints Number of vertices (minimum 3)
     * @param color Line color
     * @param filled true to fill, false for outline only
     */
    void drawPolygon(const uint16_t* points, uint8_t numPoints,
                    uint16_t color, bool filled = false);

    /**
     * Draw an arc (portion of a circle)
     * @param x Center X coordinate
     * @param y Center Y coordinate
     * @param radius Arc radius
     * @param startAngle Start angle in degrees (0 = right, 90 = up)
     * @param endAngle End angle in degrees
     * @param color Arc color
     */
    void drawArc(uint16_t x, uint16_t y, uint16_t radius,
                int16_t startAngle, int16_t endAngle, uint16_t color);

    /**
     * Draw an ellipse
     * @param x Center X coordinate
     * @param y Center Y coordinate
     * @param radiusX Horizontal radius
     * @param radiusY Vertical radius
     * @param color Line color
     * @param filled true to fill, false for outline only
     */
    void drawEllipse(uint16_t x, uint16_t y,
                    uint16_t radiusX, uint16_t radiusY,
                    uint16_t color, bool filled = false);

    /**
     * Draw a rounded rectangle
     * @param x X coordinate
     * @param y Y coordinate
     * @param width Rectangle width
     * @param height Rectangle height
     * @param radius Corner radius
     * @param color Line color
     * @param filled true to fill, false for outline only
     */
    void drawRoundRect(uint16_t x, uint16_t y,
                      uint16_t width, uint16_t height,
                      uint16_t radius, uint16_t color, bool filled = false);

    /**
     * Draw a thick line
     * @param x0, y0 Start point
     * @param x1, y1 End point
     * @param thickness Line thickness in pixels
     * @param color Line color
     */
    void drawThickLine(uint16_t x0, uint16_t y0,
                      uint16_t x1, uint16_t y1,
                      uint16_t thickness, uint16_t color);

    /**
     * Draw a quadratic Bezier curve
     * @param x0, y0 Start point
     * @param x1, y1 Control point
     * @param x2, y2 End point
     * @param color Curve color
     */
    void drawBezier(uint16_t x0, uint16_t y0,
                   uint16_t x1, uint16_t y1,
                   uint16_t x2, uint16_t y2,
                   uint16_t color);

    /**
     * Draw a star
     * @param x Center X coordinate
     * @param y Center Y coordinate
     * @param outerRadius Outer radius (tip of star)
     * @param innerRadius Inner radius (inner vertices)
     * @param numPoints Number of star points (minimum 3)
     * @param color Star color
     * @param filled true to fill, false for outline only
     */
    void drawStar(uint16_t x, uint16_t y,
                 uint16_t outerRadius, uint16_t innerRadius,
                 uint8_t numPoints, uint16_t color, bool filled = false);

    /**
     * Draw a hexagon
     * @param x Center X coordinate
     * @param y Center Y coordinate
     * @param radius Distance from center to vertex
     * @param color Hexagon color
     * @param filled true to fill, false for outline only
     */
    void drawHexagon(uint16_t x, uint16_t y, uint16_t radius,
                    uint16_t color, bool filled = false);

    /**
     * Flood fill area starting from a point
     * @param x Starting X coordinate
     * @param y Starting Y coordinate
     * @param color Fill color
     * @param boundary Boundary color (stops filling at this color)
     * WARNING: Can be slow and memory-intensive, use with caution
     */
    void floodFill(uint16_t x, uint16_t y, uint16_t color, uint16_t boundary);

    // ========== Buffer Management ==========

    /**
     * Get direct access to the main screen buffer (for advanced use)
     * @return Pointer to 15KB screen buffer
     */
    UBYTE* getBuffer() { return screenBuffer; }

    /**
     * Create a sub-buffer for isolated drawing
     * Useful for complex UI elements that need their own buffer
     * @param width Buffer width (must be multiple of 8)
     * @param height Buffer height
     * @return Pointer to allocated buffer (caller must free())
     */
    UBYTE* createSubBuffer(uint16_t width, uint16_t height);

    /**
     * Copy a sub-buffer region to the main screen buffer
     * @param subBuffer Source buffer
     * @param srcX Source X in sub-buffer
     * @param srcY Source Y in sub-buffer
     * @param width Region width
     * @param height Region height
     * @param destX Destination X on screen
     * @param destY Destination Y on screen
     */
    void copySubBuffer(const UBYTE* subBuffer,
                      uint16_t srcX, uint16_t srcY,
                      uint16_t width, uint16_t height,
                      uint16_t destX, uint16_t destY);

    // ========== Configuration ==========

    /**
     * Set the full refresh interval
     * @param interval Number of partial updates before forcing full refresh
     */
    void setFullRefreshInterval(uint8_t interval) { fullRefreshInterval = interval; }

    /**
     * Get the full refresh interval
     */
    uint8_t getFullRefreshInterval() const { return fullRefreshInterval; }

    /**
     * Enable or disable automatic full refresh
     * @param enabled true to auto-refresh, false to disable
     */
    void setAutoFullRefresh(bool enabled) { autoFullRefreshEnabled = enabled; }

private:
    // Display state
    UBYTE* screenBuffer;           // Main 15KB screen buffer
    uint8_t partialRefreshCount;   // Counter for hybrid refresh strategy
    uint8_t fullRefreshInterval;   // Partial updates before full refresh
    bool autoFullRefreshEnabled;   // Auto-refresh enabled flag
    bool initialized;              // Initialization state

    // Internal helper methods
    void extractRegionBuffer(const UIRegion& region, UBYTE* destBuffer);
    void partialRefreshRaw(const UIRegion& region, const UBYTE* regionBuffer);
};

#endif // WATCHER_DISPLAY_H
