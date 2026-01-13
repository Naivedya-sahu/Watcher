# Buffer Architecture - Pomodoro V2

## Problem Statement

The original Pomodoro implementation experienced visualization and interaction glitches due to:

1. **Shared buffer conflicts**: Single buffer used for all UI regions
2. **Undefined property errors**: Buffer reuse without proper isolation
3. **Display artifacts**: Partial refresh regions overlapping
4. **Memory confusion**: Manual buffer extraction with potential off-by-one errors

## Solution: Buffer-Isolated Architecture

Pomodoro V2 implements **dedicated buffers for each UI region** to ensure clean, glitch-free rendering.

### Buffer Allocation Strategy

```
Total Memory Budget: ~26KB
├─ screenBuffer:        15,000 bytes  (400×300 full screen)
├─ digitsBuffer:         2,800 bytes  (160×140 digit region)
├─ progressBuffer:       7,488 bytes  (288×208 progress border)
├─ buttonLeftBuffer:       256 bytes  (64×32 button label)
└─ buttonRightBuffer:      256 bytes  (64×32 button label)
```

### Buffer Regions

#### 1. Screen Buffer (Full Refresh Only)

```cpp
UBYTE *screenBuffer = nullptr;  // 400×300 = 15,000 bytes

// Used for:
// - Initial display
// - State changes (IDLE ↔ RUNNING ↔ PAUSED ↔ COMPLETE)
// - Mode changes
// - Anti-ghosting full refresh
```

**Region**: Entire screen (0,0) → (400,300)

#### 2. Digits Buffer (Partial Refresh)

```cpp
UBYTE *digitsBuffer = nullptr;  // 160×140 = 2,800 bytes

// Used for:
// - Timer countdown updates (every second)
// - Both tens and ones digits
```

**Region**: (117,80) → (277,220)
**Contents**: Two 7-segment digits (70×130 each) with padding

**Update Frequency**: 1 Hz (every second while running)

#### 3. Progress Buffer (Partial Refresh)

```cpp
UBYTE *progressBuffer = nullptr;  // 288×208 = 7,488 bytes

// Used for:
// - Progress square updates (60 squares, 1 per second)
// - Border frame
```

**Region**: (56,49) → (344,257)
**Contents**: 60 progress squares (10×10) arranged in border pattern

**Update Frequency**: 1 Hz (every second while running)

#### 4. Button Buffers (Partial Refresh)

```cpp
UBYTE *buttonLeftBuffer = nullptr;   // 64×32 = 256 bytes
UBYTE *buttonRightBuffer = nullptr;  // 64×32 = 256 bytes

// Used for:
// - Button label changes (START/PAUSE/RESET)
```

**Left Region**: (46,266) → (110,298)
**Right Region**: (306,266) → (370,298)

**Update Frequency**: On state transition only

## Rendering Pipeline

### Full Screen Render

```cpp
void renderFullScreen() {
    Paint_NewImage(screenBuffer, SCREEN_WIDTH, SCREEN_HEIGHT, 0, WHITE);
    Paint_SelectImage(screenBuffer);
    Paint_Clear(WHITE);

    // Draw all UI elements...

    EPD_4IN2_V2_Display(screenBuffer);  // ~2000ms
}
```

**When**: Initial boot, mode change, anti-ghosting, reset

### Partial Render - Digits

```cpp
void renderDigitsPartial() {
    // 1. Select dedicated buffer
    Paint_NewImage(digitsBuffer, DIGITS_BUFFER_WIDTH, DIGITS_BUFFER_HEIGHT, 0, WHITE);
    Paint_SelectImage(digitsBuffer);
    Paint_Clear(WHITE);

    // 2. Draw in LOCAL coordinates (0,0 origin)
    drawDigit(5, 5, tens);                    // Local coordinates
    drawDigit(5 + 70 + 15, 5, ones);

    // 3. Partial refresh to SCREEN coordinates
    uint16_t refreshX = ((TENS_X - 5) / 8) * 8;  // 8-pixel alignment
    uint16_t refreshY = TENS_Y - 5;
    uint16_t refreshWidth = ((DIGITS_BUFFER_WIDTH + 7) / 8) * 8;

    EPD_4IN2_V2_PartialDisplay(digitsBuffer,
                               refreshX, refreshY,
                               refreshX + refreshWidth,
                               refreshY + DIGITS_BUFFER_HEIGHT);
}
```

**When**: Every second while timer is running (~400ms refresh)

### Partial Render - Progress

```cpp
void renderProgressPartial() {
    // 1. Select dedicated buffer
    Paint_NewImage(progressBuffer, PROGRESS_BUFFER_WIDTH, PROGRESS_BUFFER_HEIGHT, 0, WHITE);
    Paint_SelectImage(progressBuffer);
    Paint_Clear(WHITE);

    // 2. Draw border and squares in LOCAL coordinates
    Paint_DrawRectangle(0, 0, BORDER_W, BORDER_H, BLACK, ...);

    for (int i = 0; i < elapsedSeconds && i < 60; i++) {
        uint16_t localX = squarePositions[i].x - BORDER_X;
        uint16_t localY = squarePositions[i].y - BORDER_Y;
        Paint_DrawRectangle(localX, localY,
                          localX + PROGRESS_SIZE,
                          localY + PROGRESS_SIZE, ...);
    }

    // 3. Partial refresh to SCREEN coordinates
    uint16_t refreshX = ((BORDER_X / 8) * 8);
    EPD_4IN2_V2_PartialDisplay(progressBuffer, ...);
}
```

**When**: Every second while timer is running (~400ms refresh)

### Partial Render - Buttons

```cpp
void renderButtonLeftPartial() {
    Paint_NewImage(buttonLeftBuffer, BUTTON_BUFFER_WIDTH, BUTTON_BUFFER_HEIGHT, 0, WHITE);
    Paint_SelectImage(buttonLeftBuffer);
    Paint_Clear(WHITE);

    // Draw label in LOCAL coordinates
    Paint_DrawString_EN(4, 8, "PAUSE", &Font16, WHITE, BLACK);

    // Partial refresh
    uint16_t refreshX = ((BTN_LEFT_X - 4) / 8) * 8;
    EPD_4IN2_V2_PartialDisplay(buttonLeftBuffer, ...);
}
```

**When**: State change (IDLE↔RUNNING↔PAUSED)

## Key Design Principles

### 1. Buffer Isolation

Each UI component has its own buffer - **never shared, never reused**.

```cpp
// ❌ BAD (old approach)
Paint_SelectImage(BlackImage);  // Single shared buffer
drawDigit(...);
partialRefresh(digitRegion);
drawProgress(...);             // Overwrites digit data!
partialRefresh(progressRegion);

// ✅ GOOD (new approach)
Paint_SelectImage(digitsBuffer);    // Dedicated buffer
drawDigit(...);
partialRefresh(digitRegion);

Paint_SelectImage(progressBuffer);  // Different buffer
drawProgress(...);
partialRefresh(progressRegion);
```

### 2. Local Coordinate System

All drawing happens in **local (buffer-relative) coordinates**.

```cpp
// Buffer origin is (0,0)
// Screen position handled separately during display

// Drawing phase (local coords)
Paint_SelectImage(digitsBuffer);
drawDigit(5, 5, digit);  // (5,5) in buffer space

// Display phase (screen coords)
EPD_4IN2_V2_PartialDisplay(digitsBuffer,
                           TENS_X - 5,    // Screen X
                           TENS_Y - 5,    // Screen Y
                           ...);
```

### 3. UC8176 Alignment

All partial refresh regions are **8-pixel aligned** (UC8176 requirement).

```cpp
// Align to 8-pixel boundaries
uint16_t refreshX = ((screenX / 8) * 8);
uint16_t refreshWidth = ((bufferWidth + 7) / 8) * 8;

// Example:
// Screen X = 122 → Aligned X = 120 (122/8 = 15, 15*8 = 120)
// Width = 160 → Aligned Width = 160 (already aligned)
```

### 4. Anti-Ghosting

Track partial refresh count and force full refresh periodically.

```cpp
uint8_t partialRefreshCount = 0;
#define MAX_PARTIAL_REFRESHES 8

void renderDigitsPartial() {
    // ... partial refresh ...
    partialRefreshCount++;
}

void updateTimer() {
    if (partialRefreshCount >= MAX_PARTIAL_REFRESHES) {
        renderFullScreen();  // Clears ghosting
        partialRefreshCount = 0;
    }
}
```

## Memory Management

### Allocation (setup)

```cpp
bool allocateBuffers() {
    screenBuffer = (UBYTE*)malloc((SCREEN_WIDTH * SCREEN_HEIGHT) / 8);
    if (!screenBuffer) return false;

    digitsBuffer = (UBYTE*)malloc((DIGITS_BUFFER_WIDTH * DIGITS_BUFFER_HEIGHT) / 8);
    if (!digitsBuffer) return false;

    // ... allocate remaining buffers ...

    return true;
}
```

### Deallocation (never called in embedded context)

```cpp
void freeBuffers() {
    if (screenBuffer) free(screenBuffer);
    if (digitsBuffer) free(digitsBuffer);
    // ... free remaining buffers ...
}
```

## Performance Characteristics

| Operation | Buffer Used | Display Time | Frequency |
|-----------|-------------|--------------|-----------|
| Full screen | screenBuffer | ~2000ms | Boot, mode change, anti-ghosting |
| Update digits | digitsBuffer | ~400ms | 1 Hz while running |
| Update progress | progressBuffer | ~400ms | 1 Hz while running |
| Update button | buttonLeftBuffer | ~400ms | State change only |

**Total refresh time per second** (while running): ~800ms (digits + progress in sequence)

## Benefits Over Original Implementation

### Before (Single Buffer)

```cpp
UBYTE *BlackImage;  // Single 15KB buffer

// Problems:
// 1. Manual region extraction (memcpy + alignment math)
// 2. Buffer corruption if regions overlap
// 3. Undefined behavior if Paint_SelectImage not called
// 4. Complex partial refresh calculations
```

### After (Dedicated Buffers)

```cpp
UBYTE *digitsBuffer;    // Dedicated 2.8KB
UBYTE *progressBuffer;  // Dedicated 7.5KB
UBYTE *buttonBuffers;   // Dedicated 256B each

// Benefits:
// ✅ No buffer conflicts - each region isolated
// ✅ No memcpy needed - direct drawing to target buffer
// ✅ Simple local coordinates - no screen offset math
// ✅ Predictable behavior - no undefined state
// ✅ Easier debugging - each buffer serves single purpose
```

## Troubleshooting

### Symptom: Digits not updating

**Check**:
1. `Paint_SelectImage(digitsBuffer)` called before drawing?
2. Partial refresh X/Y coordinates correct?
3. `partialRefreshCount` tracking correct?

### Symptom: Display artifacts/ghosting

**Check**:
1. 8-pixel alignment correct?
2. Anti-ghosting counter incrementing?
3. `Paint_Clear(WHITE)` called before drawing?

### Symptom: Button labels missing

**Check**:
1. Button buffer allocated successfully?
2. Empty label check (`strlen(label) > 0`)?
3. Font rendering coordinates within buffer bounds?

### Symptom: Heap allocation failure

**Check**:
1. ESP32 PSRAM enabled? (`-DBOARD_HAS_PSRAM`)
2. Total buffer size ~26KB - verify available heap
3. No memory leaks from previous allocations?

## Code Organization

```
pomodoro_v2.cpp
├─ Buffer Allocation (lines 260-310)
│   ├─ allocateBuffers()
│   └─ freeBuffers()
├─ Full Screen Rendering (lines 340-420)
│   └─ renderFullScreen()
├─ Partial Rendering (lines 425-600)
│   ├─ renderDigitsPartial()
│   ├─ renderProgressPartial()
│   ├─ renderButtonLeftPartial()
│   └─ renderButtonRightPartial()
└─ Main Loop (lines 700-750)
    ├─ processButtons()
    └─ updateTimer()
```

## Future Improvements

1. **Double buffering**: Eliminate flicker during partial updates
2. **Lazy rendering**: Only update if value actually changed
3. **Coalesced updates**: Batch multiple partial updates into one
4. **DMA transfers**: Hardware-accelerated buffer copy to display

---

**Summary**: Pomodoro V2 uses dedicated buffers for each UI component, eliminating display glitches through complete buffer isolation and local coordinate systems.
