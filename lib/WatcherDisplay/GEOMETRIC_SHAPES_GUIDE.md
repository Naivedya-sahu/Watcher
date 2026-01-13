## Geometric Drawing Functions Guide

Complete reference for all geometric shapes and advanced drawing capabilities in WatcherDisplay.

## Overview

WatcherDisplay provides extensive geometric drawing primitives beyond basic rectangles and circles:

✅ **Basic Shapes**: Triangles, Polygons, Ellipses
✅ **Advanced Shapes**: Stars, Hexagons, Rounded Rectangles
✅ **Curves**: Arcs, Bezier Curves
✅ **Lines**: Thick Lines, Multi-segment Paths
✅ **Fill Operations**: Flood Fill, Solid/Outline modes

---

## Table of Contents

1. [Triangles](#triangles)
2. [Polygons](#polygons)
3. [Arcs](#arcs)
4. [Ellipses](#ellipses)
5. [Rounded Rectangles](#rounded-rectangles)
6. [Thick Lines](#thick-lines)
7. [Bezier Curves](#bezier-curves)
8. [Stars](#stars)
9. [Hexagons](#hexagons)
10. [Flood Fill](#flood-fill)
11. [Complex Designs](#complex-designs)

---

## Triangles

Draw three-sided polygons with outline or filled.

### Syntax

```cpp
void drawTriangle(uint16_t x0, uint16_t y0,    // Vertex 1
                  uint16_t x1, uint16_t y1,    // Vertex 2
                  uint16_t x2, uint16_t y2,    // Vertex 3
                  uint16_t color,
                  bool filled = false);
```

### Examples

```cpp
// Outline triangle
display.drawTriangle(50, 20, 30, 60, 70, 60, COLORED, false);

// Filled triangle
display.drawTriangle(100, 20, 80, 60, 120, 60, COLORED, true);

// Equilateral triangle
int centerX = 200;
int centerY = 50;
int radius = 30;
display.drawTriangle(
    centerX, centerY - radius,                        // Top
    centerX - radius * 0.866, centerY + radius / 2,   // Bottom-left
    centerX + radius * 0.866, centerY + radius / 2,   // Bottom-right
    COLORED, false
);
```

### Use Cases

- **Up/down arrows** for UI navigation
- **Play/pause icons** for media controls
- **Warning symbols** (outline triangle + exclamation)
- **Mountain/terrain graphics**

---

## Polygons

Draw custom shapes with any number of sides (3+).

### Syntax

```cpp
void drawPolygon(const uint16_t* points,  // [x0,y0, x1,y1, x2,y2, ...]
                 uint8_t numPoints,       // Number of vertices
                 uint16_t color,
                 bool filled = false);
```

### Examples

```cpp
// Pentagon (5 sides)
uint16_t pentagon[10] = {
    200, 50,   // Point 1
    180, 70,   // Point 2
    190, 90,   // Point 3
    210, 90,   // Point 4
    220, 70    // Point 5
};
display.drawPolygon(pentagon, 5, COLORED, true);

// Octagon (8 sides)
uint16_t octagon[16] = {
    100, 50,
    115, 45,
    125, 50,
    130, 65,
    125, 80,
    115, 85,
    100, 80,
    95, 65
};
display.drawPolygon(octagon, 8, COLORED, false);

// Diamond (4 sides)
uint16_t diamond[8] = {
    150, 100,   // Top
    130, 120,   // Left
    150, 140,   // Bottom
    170, 120    // Right
};
display.drawPolygon(diamond, 4, COLORED, true);
```

### Helper Function: Regular Polygon

```cpp
void drawRegularPolygon(uint16_t x, uint16_t y, uint16_t radius,
                       uint8_t sides, uint16_t color, bool filled) {
    if (sides < 3) return;

    uint16_t points[sides * 2];
    float angleStep = 2.0 * PI / sides;
    float angleOffset = -PI / 2.0;  // Start at top

    for (uint8_t i = 0; i < sides; i++) {
        float angle = angleOffset + i * angleStep;
        points[i * 2] = x + radius * cos(angle);
        points[i * 2 + 1] = y + radius * sin(angle);
    }

    display.drawPolygon(points, sides, color, filled);
}

// Usage:
drawRegularPolygon(200, 150, 40, 6, COLORED, true);  // Hexagon
drawRegularPolygon(300, 150, 35, 5, COLORED, false); // Pentagon
```

### Use Cases

- **Custom UI shapes** (speech bubbles, badges)
- **Game objects** (houses, rockets)
- **Data visualization** (pie chart segments)
- **Icons** (stop sign = octagon, home = pentagon)

---

## Arcs

Draw circular segments (portions of a circle).

### Syntax

```cpp
void drawArc(uint16_t x, uint16_t y,      // Center point
             uint16_t radius,
             int16_t startAngle,          // Degrees (0 = right)
             int16_t endAngle,            // Degrees
             uint16_t color);
```

**Angle Convention:**
- `0°` = Right (3 o'clock)
- `90°` = Up (12 o'clock)
- `180°` = Left (9 o'clock)
- `270°` = Down (6 o'clock)

### Examples

```cpp
// Quarter circle (top-right)
display.drawArc(100, 100, 40, 0, 90, COLORED);

// Semi-circle (top half)
display.drawArc(200, 150, 50, 0, 180, COLORED);

// Three-quarter circle
display.drawArc(300, 100, 35, 45, 315, COLORED);

// Pac-Man mouth
display.drawCircle(150, 200, 30, COLORED, true);
display.drawArc(150, 200, 30, 30, 330, UNCOLORED);  // Erase mouth
```

### Use Cases

- **Progress indicators** (circular progress: 0-360°)
- **Gauges/meters** (speedometer arc)
- **Pie chart segments**
- **Rounded corners** (use 4 arcs for rounded rect corners)

### Example: Progress Gauge

```cpp
void drawProgressGauge(uint16_t x, uint16_t y, uint8_t progress) {
    // Background arc (full circle outline)
    display.drawArc(x, y, 50, 0, 360, COLORED);

    // Progress arc (filled)
    int16_t endAngle = (progress * 360) / 100;
    for (int r = 40; r <= 50; r++) {
        display.drawArc(x, y, r, -90, -90 + endAngle, COLORED);
    }

    // Center text
    char text[5];
    snprintf(text, sizeof(text), "%d%%", progress);
    display.drawText(x - 15, y - 8, text, &Font16, true);
}
```

---

## Ellipses

Draw oval shapes with independent horizontal/vertical radii.

### Syntax

```cpp
void drawEllipse(uint16_t x, uint16_t y,     // Center point
                 uint16_t radiusX,           // Horizontal radius
                 uint16_t radiusY,           // Vertical radius
                 uint16_t color,
                 bool filled = false);
```

### Examples

```cpp
// Horizontal ellipse
display.drawEllipse(100, 100, 50, 30, COLORED, false);

// Vertical ellipse
display.drawEllipse(200, 100, 20, 40, COLORED, true);

// Circle (same radii)
display.drawEllipse(300, 100, 35, 35, COLORED, false);
```

### Use Cases

- **Speech bubbles** (ellipse + triangle pointer)
- **Eyes** for characters/faces
- **Buttons** with pill shape
- **Logos** and decorative elements

### Example: Speech Bubble

```cpp
void drawSpeechBubble(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* text) {
    // Main bubble (ellipse)
    display.drawEllipse(x, y, width/2, height/2, COLORED, false);

    // Pointer (triangle)
    uint16_t pointer[6] = {
        x - 10, y + height/2 - 5,
        x - 20, y + height/2 + 10,
        x - 5, y + height/2 + 5
    };
    display.drawPolygon(pointer, 3, COLORED, true);

    // Text inside
    display.drawText(x - width/3, y - 8, text, &Font16, true);
}

// Usage:
drawSpeechBubble(150, 100, 80, 50, "Hello!");
```

---

## Rounded Rectangles

Rectangles with smooth, rounded corners.

### Syntax

```cpp
void drawRoundRect(uint16_t x, uint16_t y,
                   uint16_t width, uint16_t height,
                   uint16_t radius,          // Corner radius
                   uint16_t color,
                   bool filled = false);
```

### Examples

```cpp
// Slightly rounded
display.drawRoundRect(50, 50, 100, 60, 5, COLORED, false);

// Very rounded (pill-shaped)
display.drawRoundRect(200, 50, 120, 40, 20, COLORED, true);

// Button-style
display.drawRoundRect(100, 150, 80, 35, 10, COLORED, false);
display.drawText(120, 160, "OK", &Font16, true);
```

### Use Cases

- **UI buttons** (modern rounded style)
- **Cards** and containers
- **Status badges**
- **Notification boxes**

### Example: Modern Button

```cpp
void drawButton(uint16_t x, uint16_t y, const char* label, bool pressed) {
    uint16_t width = strlen(label) * 12 + 20;
    uint16_t height = 30;

    if (pressed) {
        // Filled when pressed
        display.drawRoundRect(x, y, width, height, 8, COLORED, true);
        display.drawText(x + 10, y + 8, label, &Font16, false);  // White text
    } else {
        // Outline when not pressed
        display.drawRoundRect(x, y, width, height, 8, COLORED, false);
        display.drawText(x + 10, y + 8, label, &Font16, true);   // Black text
    }
}

// Usage:
drawButton(50, 100, "Start", false);
drawButton(150, 100, "Stop", true);
```

---

## Thick Lines

Draw lines with variable thickness.

### Syntax

```cpp
void drawThickLine(uint16_t x0, uint16_t y0,    // Start point
                   uint16_t x1, uint16_t y1,    // End point
                   uint16_t thickness,          // Line width
                   uint16_t color);
```

### Examples

```cpp
// Thin line
display.drawThickLine(50, 50, 150, 50, 1, COLORED);

// Medium line
display.drawThickLine(50, 70, 150, 70, 4, COLORED);

// Thick line
display.drawThickLine(50, 100, 150, 100, 8, COLORED);

// Diagonal thick line
display.drawThickLine(200, 50, 300, 150, 6, COLORED);
```

### Use Cases

- **Dividers** and separators
- **Underlines** for headers
- **Progress bars** (horizontal thick line)
- **Graph axes**

### Example: Underline Effect

```cpp
void drawHeaderWithUnderline(uint16_t x, uint16_t y, const char* text) {
    display.drawText(x, y, text, &Font24, true);

    uint16_t textWidth = strlen(text) * 14;  // Font24 width ~14px
    display.drawThickLine(x, y + 28, x + textWidth, y + 28, 3, COLORED);
}

// Usage:
drawHeaderWithUnderline(50, 50, "SETTINGS");
```

---

## Bezier Curves

Draw smooth curves using quadratic Bezier equations.

### Syntax

```cpp
void drawBezier(uint16_t x0, uint16_t y0,    // Start point
                uint16_t x1, uint16_t y1,    // Control point
                uint16_t x2, uint16_t y2,    // End point
                uint16_t color);
```

### Examples

```cpp
// Gentle curve
display.drawBezier(50, 100, 100, 50, 150, 100, COLORED);

// S-curve
display.drawBezier(200, 50, 250, 100, 200, 150, COLORED);

// Steep curve
display.drawBezier(300, 100, 300, 50, 350, 100, COLORED);
```

### Visualization

```
(x0,y0) ────────── (x2,y2)
  │                     │
  │                     │
  └───── (x1,y1) ───────┘
         Control Point
```

The curve is "pulled" toward the control point.

### Use Cases

- **Connecting lines** for flowcharts/diagrams
- **Decorative elements**
- **Smooth transitions** between UI elements
- **Graph curves**

### Example: Connection Lines

```cpp
void drawConnection(uint16_t fromX, uint16_t fromY,
                   uint16_t toX, uint16_t toY) {
    // Calculate control point (midpoint offset)
    uint16_t midX = (fromX + toX) / 2;
    uint16_t midY = (fromY + toY) / 2 - 30;  // Offset upward

    display.drawBezier(fromX, fromY, midX, midY, toX, toY, COLORED);

    // Draw end points
    display.drawCircle(fromX, fromY, 5, COLORED, true);
    display.drawCircle(toX, toY, 5, COLORED, true);
}

// Usage:
drawConnection(50, 100, 200, 150);
```

---

## Stars

Draw multi-pointed star shapes.

### Syntax

```cpp
void drawStar(uint16_t x, uint16_t y,       // Center point
              uint16_t outerRadius,         // Tip radius
              uint16_t innerRadius,         // Inner vertex radius
              uint8_t numPoints,            // Number of star points
              uint16_t color,
              bool filled = false);
```

### Examples

```cpp
// 5-pointed star (classic)
display.drawStar(100, 100, 40, 15, 5, COLORED, true);

// 6-pointed star (Star of David)
display.drawStar(200, 100, 35, 12, 6, COLORED, false);

// 4-pointed star (compass rose)
display.drawStar(300, 100, 30, 10, 4, COLORED, true);

// 8-pointed star (decorative)
display.drawStar(150, 200, 25, 10, 8, COLORED, false);
```

### Use Cases

- **Rating systems** (5-star reviews)
- **Decorative icons**
- **Achievement badges**
- **Navigation symbols** (compass)

### Example: Star Rating

```cpp
void drawStarRating(uint16_t x, uint16_t y, uint8_t rating) {
    for (uint8_t i = 0; i < 5; i++) {
        bool filled = (i < rating);
        display.drawStar(x + i*30, y, 10, 4, 5, COLORED, filled);
    }
}

// Usage:
drawStarRating(50, 100, 4);  // 4 out of 5 stars
```

---

## Hexagons

Draw six-sided regular polygons.

### Syntax

```cpp
void drawHexagon(uint16_t x, uint16_t y,    // Center point
                 uint16_t radius,           // Distance to vertex
                 uint16_t color,
                 bool filled = false);
```

### Examples

```cpp
// Outline hexagon
display.drawHexagon(100, 100, 40, COLORED, false);

// Filled hexagon
display.drawHexagon(200, 100, 35, COLORED, true);

// Honeycomb pattern
for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 4; col++) {
        int x = 50 + col * 45;
        int y = 50 + row * 40 + (col % 2) * 20;
        display.drawHexagon(x, y, 20, COLORED, false);
    }
}
```

### Use Cases

- **Game tiles** (board games, strategy games)
- **Honeycomb patterns**
- **Tech/futuristic UI elements**
- **Icons** (settings, network nodes)

### Example: Hex Grid Map

```cpp
void drawHexGrid(uint16_t startX, uint16_t startY, uint8_t rows, uint8_t cols) {
    uint16_t hexSize = 20;
    uint16_t hexWidth = hexSize * sqrt(3);
    uint16_t hexHeight = hexSize * 2;

    for (uint8_t row = 0; row < rows; row++) {
        for (uint8_t col = 0; col < cols; col++) {
            uint16_t x = startX + col * hexWidth + (row % 2) * (hexWidth / 2);
            uint16_t y = startY + row * hexHeight * 0.75;

            display.drawHexagon(x, y, hexSize, COLORED, false);
        }
    }
}

// Usage:
drawHexGrid(20, 20, 5, 8);  // 5 rows, 8 columns
```

---

## Flood Fill

Fill an enclosed area with color.

### Syntax

```cpp
void floodFill(uint16_t x, uint16_t y,      // Starting point
               uint16_t color,              // Fill color
               uint16_t boundary);          // Boundary color
```

### ⚠️ Warning

Flood fill is **recursive** and can be slow/memory-intensive. Use with caution on large areas.

### Examples

```cpp
// Draw a shape
display.drawCircle(100, 100, 40, COLORED, false);

// Fill it
display.floodFill(100, 100, COLORED, COLORED);

// Draw and fill rectangle
display.drawRect(200, 80, 80, 50, COLORED, false);
display.floodFill(240, 105, COLORED, COLORED);
```

### Use Cases

- **Filling irregular shapes**
- **Paint bucket tool** for user drawing apps
- **Coloring game elements**

### Better Alternative: Pre-fill Shapes

Instead of using flood fill, draw shapes filled from the start:

```cpp
// Don't do this (slow):
display.drawCircle(100, 100, 40, COLORED, false);
display.floodFill(100, 100, COLORED, COLORED);

// Do this (fast):
display.drawCircle(100, 100, 40, COLORED, true);  // filled=true
```

---

## Complex Designs

Combine multiple shapes to create complex graphics.

### Example: Weather Icons

```cpp
void drawSunIcon(uint16_t x, uint16_t y) {
    // Center circle
    display.drawCircle(x, y, 15, COLORED, true);

    // 8 rays around sun
    for (int i = 0; i < 8; i++) {
        float angle = i * PI / 4.0;
        int16_t x1 = x + 20 * cos(angle);
        int16_t y1 = y + 20 * sin(angle);
        int16_t x2 = x + 28 * cos(angle);
        int16_t y2 = y + 28 * sin(angle);
        display.drawThickLine(x1, y1, x2, y2, 2, COLORED);
    }
}

void drawCloudIcon(uint16_t x, uint16_t y) {
    // Cloud shape using overlapping ellipses
    display.drawEllipse(x, y, 18, 12, COLORED, true);
    display.drawEllipse(x + 15, y + 3, 15, 10, COLORED, true);
    display.drawEllipse(x + 28, y, 16, 11, COLORED, true);
}

void drawRainIcon(uint16_t x, uint16_t y) {
    drawCloudIcon(x, y);

    // Rain drops (short lines)
    for (int i = 0; i < 5; i++) {
        int16_t dropX = x - 10 + i * 10;
        display.drawThickLine(dropX, y + 15, dropX + 2, y + 22, 2, COLORED);
    }
}
```

### Example: Progress Ring

```cpp
void drawProgressRing(uint16_t x, uint16_t y, uint8_t progress) {
    // Background ring (outline)
    display.drawCircle(x, y, 50, COLORED, false);
    display.drawCircle(x, y, 45, COLORED, false);

    // Progress arc (filled ring segment)
    int16_t endAngle = (progress * 360) / 100 - 90;
    for (int r = 45; r <= 50; r++) {
        display.drawArc(x, y, r, -90, endAngle, COLORED);
    }

    // Center percentage text
    char text[5];
    snprintf(text, sizeof(text), "%d%%", progress);
    display.drawText(x - 15, y - 8, text, &Font16, true);
}

// Usage:
drawProgressRing(200, 150, 75);  // 75% complete
```

### Example: Custom UI Widget

```cpp
void drawNotificationCard(uint16_t x, uint16_t y, const char* title, const char* message) {
    // Card background
    display.drawRoundRect(x, y, 180, 70, 8, COLORED, false);

    // Icon (star)
    display.drawStar(x + 15, y + 35, 12, 5, 5, COLORED, true);

    // Title
    display.drawText(x + 35, y + 10, title, &Font16, true);

    // Separator line
    display.drawThickLine(x + 35, y + 28, x + 165, y + 28, 1, COLORED);

    // Message
    display.drawText(x + 35, y + 35, message, &Font12, true);
}

// Usage:
drawNotificationCard(50, 100, "Alert", "Task completed!");
```

---

## Performance Tips

### 1. Minimize Partial Refreshes

Group shape drawing and update regions in batches:

```cpp
// Bad - multiple updates
display.drawTriangle(10, 10, 30, 30, 50, 10, COLORED, true);
display.updateRegion(10, 10, 50, 30);

display.drawCircle(100, 20, 15, COLORED, true);
display.updateRegion(85, 5, 30, 30);

// Good - single update
display.drawTriangle(10, 10, 30, 30, 50, 10, COLORED, true);
display.drawCircle(100, 20, 15, COLORED, true);
display.updateRegion(10, 5, 125, 35);  // Encompassing region
```

### 2. Use Simple Shapes for Large Areas

```cpp
// Filled polygon is faster than many lines
// Bad:
for (int i = 0; i < 50; i++) {
    display.drawLine(10, 10 + i, 100, 10 + i, COLORED);
}

// Good:
display.fillRegion(10, 10, 90, 50, COLORED);
```

### 3. Cache Complex Calculations

```cpp
// Pre-calculate star points once
void setup() {
    calculateStarPoints();  // Store in global array
}

void loop() {
    // Use pre-calculated points
    display.drawPolygon(starPoints, 10, COLORED, true);
}
```

---

## Common Patterns

### Pattern: Icon Set

Create reusable icon functions:

```cpp
void drawCheckIcon(uint16_t x, uint16_t y) {
    display.drawThickLine(x, y + 10, x + 5, y + 15, 3, COLORED);
    display.drawThickLine(x + 5, y + 15, x + 15, y, 3, COLORED);
}

void drawCrossIcon(uint16_t x, uint16_t y) {
    display.drawThickLine(x, y, x + 15, y + 15, 3, COLORED);
    display.drawThickLine(x, y + 15, x + 15, y, 3, COLORED);
}

void drawInfoIcon(uint16_t x, uint16_t y) {
    display.drawCircle(x + 8, y + 8, 10, COLORED, false);
    display.drawText(x + 6, y + 4, "i", &Font16, true);
}
```

### Pattern: Animated Progress

```cpp
void updateProgressAnimation(uint8_t progress) {
    static uint8_t lastProgress = 0;

    if (progress != lastProgress) {
        // Clear old
        display.clearRegion(50, 100, 300, 60);

        // Draw new progress
        drawProgressRing(200, 130, progress);

        // Update
        display.updateRegion(50, 100, 300, 60);
        lastProgress = progress;
    }
}
```

---

## See Also

- [WatcherDisplay README](README.md) - Main library docs
- [GeometricShapesExample.cpp](examples/GeometricShapesExample.cpp) - Full code example
- [Custom Fonts Guide](CUSTOM_FONTS_GUIDE.md) - Font integration

---

**Draw amazing UIs!** 🎨📐
