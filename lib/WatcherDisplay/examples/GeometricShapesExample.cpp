/**
 * Geometric Shapes Example
 *
 * Demonstrates all geometric drawing functions in WatcherDisplay:
 * - Triangles (filled/outline)
 * - Polygons (custom shapes)
 * - Arcs (circular segments)
 * - Ellipses (filled/outline)
 * - Rounded rectangles
 * - Thick lines
 * - Bezier curves
 * - Stars
 * - Hexagons
 */

#include <WatcherDisplay.h>

WatcherDisplay display;

void setup() {
    Serial.begin(115200);
    Serial.println("Geometric Shapes Example");

    // Initialize display
    if (!display.begin()) {
        Serial.println("Display init failed!");
        return;
    }

    display.clear();
    drawAllShapes();
}

void drawAllShapes() {
    Serial.println("Drawing all geometric shapes...");

    // ========== TRIANGLES ==========
    Serial.println("1. Triangles");

    // Filled triangle
    display.drawTriangle(30, 20, 10, 50, 50, 50, COLORED, true);

    // Outline triangle
    display.drawTriangle(80, 20, 60, 50, 100, 50, COLORED, false);

    display.updateRegion(0, 10, 120, 50);
    delay(1000);

    // ========== POLYGONS ==========
    Serial.println("2. Polygons");

    // Pentagon (filled)
    uint16_t pentagon[10] = {
        160, 35,    // Point 1
        145, 50,    // Point 2
        152, 70,    // Point 3
        168, 70,    // Point 4
        175, 50     // Point 5
    };
    display.drawPolygon(pentagon, 5, COLORED, true);

    // Octagon (outline)
    uint16_t octagon[16] = {
        220, 30,
        230, 25,
        240, 30,
        245, 40,
        240, 50,
        230, 55,
        220, 50,
        215, 40
    };
    display.drawPolygon(octagon, 8, COLORED, false);

    display.updateRegion(130, 20, 140, 60);
    delay(1000);

    // ========== ELLIPSES ==========
    Serial.println("3. Ellipses");

    // Horizontal ellipse (filled)
    display.drawEllipse(50, 110, 40, 20, COLORED, true);

    // Vertical ellipse (outline)
    display.drawEllipse(140, 110, 15, 30, COLORED, false);

    display.updateRegion(0, 80, 170, 60);
    delay(1000);

    // ========== ARCS ==========
    Serial.println("4. Arcs");

    // Quarter circle (0-90 degrees)
    display.drawArc(230, 110, 30, 0, 90, COLORED);

    // Semi-circle (180-360 degrees)
    display.drawArc(310, 110, 25, 180, 360, COLORED);

    // Three-quarter arc
    display.drawArc(370, 110, 20, 45, 315, COLORED);

    display.updateRegion(200, 75, 195, 70);
    delay(1000);

    // ========== ROUNDED RECTANGLES ==========
    Serial.println("5. Rounded Rectangles");

    // Filled rounded rect
    display.drawRoundRect(10, 160, 80, 40, 10, COLORED, true);

    // Outline rounded rect with larger radius
    display.drawRoundRect(110, 160, 80, 40, 15, COLORED, false);

    display.updateRegion(0, 155, 200, 50);
    delay(1000);

    // ========== STARS ==========
    Serial.println("6. Stars");

    // 5-pointed star (filled)
    display.drawStar(250, 180, 25, 10, 5, COLORED, true);

    // 6-pointed star (outline)
    display.drawStar(320, 180, 20, 8, 6, COLORED, false);

    display.updateRegion(220, 150, 125, 65);
    delay(1000);

    // ========== HEXAGONS ==========
    Serial.println("7. Hexagons");

    // Filled hexagon
    display.drawHexagon(40, 240, 25, COLORED, true);

    // Outline hexagon
    display.drawHexagon(110, 240, 20, COLORED, false);

    display.updateRegion(10, 210, 120, 60);
    delay(1000);

    // ========== THICK LINES ==========
    Serial.println("8. Thick Lines");

    // Various line thicknesses
    display.drawThickLine(180, 220, 260, 220, 2, COLORED);
    display.drawThickLine(180, 230, 260, 230, 4, COLORED);
    display.drawThickLine(180, 245, 260, 245, 6, COLORED);

    display.updateRegion(175, 215, 90, 40);
    delay(1000);

    // ========== BEZIER CURVES ==========
    Serial.println("9. Bezier Curves");

    // Quadratic Bezier curves
    display.drawBezier(290, 220, 330, 200, 370, 240, COLORED);
    display.drawBezier(290, 250, 310, 270, 350, 250, COLORED);

    display.updateRegion(285, 195, 90, 80);
    delay(1000);

    // ========== COMPLEX SHAPE COMBINATION ==========
    Serial.println("10. Complex Design");

    // Create a simple icon using multiple shapes
    // Sun icon: circle + triangular rays
    display.drawCircle(50, 290, 15, COLORED, true);

    // Draw 8 rays around the sun
    for (int i = 0; i < 8; i++) {
        float angle = i * PI / 4.0;
        int16_t x1 = 50 + 20 * cos(angle);
        int16_t y1 = 290 + 20 * sin(angle);
        int16_t x2 = 50 + 28 * cos(angle);
        int16_t y2 = 290 + 28 * sin(angle);
        display.drawThickLine(x1, y1, x2, y2, 2, COLORED);
    }

    // Cloud shape using ellipses
    display.drawEllipse(150, 285, 18, 12, COLORED, true);
    display.drawEllipse(165, 288, 15, 10, COLORED, true);
    display.drawEllipse(178, 285, 16, 11, COLORED, true);

    // House shape using polygons
    uint16_t houseBase[8] = {240, 290, 240, 260, 280, 260, 280, 290};
    display.drawPolygon(houseBase, 4, COLORED, false);

    uint16_t roof[6] = {235, 260, 260, 240, 285, 260};
    display.drawPolygon(roof, 3, COLORED, true);

    // Door
    display.drawRect(252, 275, 16, 15, COLORED, true);

    // Window
    display.drawRect(245, 265, 10, 8, COLORED, false);

    display.updateRegion(20, 230, 280, 70);
    delay(2000);

    Serial.println("All shapes drawn! Example complete.");
}

void loop() {
    // Could add animation or shape cycling here
}
