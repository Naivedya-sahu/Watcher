/**
 * FontHandler - Implementation
 */

#include "FontHandler.h"
#include <string.h>

bool FontHandler::registerFont(const char* name, uint8_t size, sFONT* font) {
    if (!name || !font) {
        Serial.println("ERROR: Invalid font name or pointer");
        return false;
    }

    // Check if already registered
    int existingIndex = findFont(name, size);
    if (existingIndex >= 0) {
        Serial.printf("WARNING: Font '%s' size %d already registered, updating...\n", name, size);
        fonts[existingIndex].font = font;
        return true;
    }

    // Find free slot
    int slot = findFreeSlot();
    if (slot < 0) {
        Serial.printf("ERROR: Font registry full (max %d fonts)\n", MAX_CUSTOM_FONTS);
        return false;
    }

    // Register font
    fonts[slot].name = name;
    fonts[slot].size = size;
    fonts[slot].font = font;
    fonts[slot].active = true;
    fontCount++;

    Serial.printf("✓ Registered font: '%s' %dpx (%dx%d)\n",
                  name, size, font->Width, font->Height);
    return true;
}

sFONT* FontHandler::getFont(const char* name, uint8_t size) {
    int index = findFont(name, size);
    if (index >= 0) {
        return fonts[index].font;
    }
    return nullptr;
}

bool FontHandler::hasFont(const char* name, uint8_t size) {
    return findFont(name, size) >= 0;
}

void FontHandler::unregisterFont(const char* name, uint8_t size) {
    if (!name) return;

    for (int i = 0; i < MAX_CUSTOM_FONTS; i++) {
        if (fonts[i].active && strcmp(fonts[i].name, name) == 0) {
            if (size == 0 || fonts[i].size == size) {
                fonts[i].active = false;
                fonts[i].name = nullptr;
                fonts[i].font = nullptr;
                fontCount--;
                Serial.printf("Unregistered font: '%s' %dpx\n", name, size);

                if (size != 0) break; // Only unregister one if size specified
            }
        }
    }
}

void FontHandler::clear() {
    for (int i = 0; i < MAX_CUSTOM_FONTS; i++) {
        fonts[i].active = false;
        fonts[i].name = nullptr;
        fonts[i].font = nullptr;
    }
    fontCount = 0;
    Serial.println("All fonts unregistered");
}

void FontHandler::listFonts() const {
    Serial.println("========== Registered Fonts ==========");
    if (fontCount == 0) {
        Serial.println("  (none)");
    } else {
        for (int i = 0; i < MAX_CUSTOM_FONTS; i++) {
            if (fonts[i].active) {
                Serial.printf("  %-15s %2dpx  (%2dx%2d)  %d bytes\n",
                              fonts[i].name,
                              fonts[i].size,
                              fonts[i].font->Width,
                              fonts[i].font->Height,
                              fonts[i].font->Width * fonts[i].font->Height / 8 * 95);
            }
        }
    }
    Serial.printf("Total: %d/%d fonts\n", fontCount, MAX_CUSTOM_FONTS);
    Serial.println("======================================");
}

const CustomFontEntry* FontHandler::getFontAt(uint8_t index) const {
    uint8_t activeCount = 0;
    for (int i = 0; i < MAX_CUSTOM_FONTS; i++) {
        if (fonts[i].active) {
            if (activeCount == index) {
                return &fonts[i];
            }
            activeCount++;
        }
    }
    return nullptr;
}

// Private helper methods

int FontHandler::findFreeSlot() {
    for (int i = 0; i < MAX_CUSTOM_FONTS; i++) {
        if (!fonts[i].active) {
            return i;
        }
    }
    return -1;
}

int FontHandler::findFont(const char* name, uint8_t size) {
    if (!name) return -1;

    for (int i = 0; i < MAX_CUSTOM_FONTS; i++) {
        if (fonts[i].active && strcmp(fonts[i].name, name) == 0) {
            // If size is 0, return first match
            if (size == 0 || fonts[i].size == size) {
                return i;
            }
        }
    }
    return -1;
}
