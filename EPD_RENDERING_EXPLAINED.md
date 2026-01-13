# E-Paper Display (EPD) Rendering - Complete Technical Reference

## Hardware: Waveshare 4.2" V2 E-Paper Display

- **Resolution**: 400×300 pixels (WIDTH × HEIGHT)
- **Interface**: SPI
- **Controller**: IL0398 (similar to SSD1619)
- **Colors**: Black/White (1-bit per pixel)

---

## 1. COORDINATE SYSTEM & BUFFER LAYOUT

### Pixel Coordinate System
```
(0,0) ──────────────────────► X (400 pixels)
  │
  │
  │
  │
  ▼
  Y (300 pixels)
```

### Buffer Memory Layout

The display memory is **1-bit per pixel**, organized as:

```
Screen: 400×300 pixels = 120,000 pixels
Buffer: 400/8 × 300 = 50 bytes/row × 300 rows = 15,000 bytes

Each byte controls 8 horizontal pixels:
Byte layout: [bit7 bit6 bit5 bit4 bit3 bit2 bit1 bit0]
            ↓
            Pixels: [P7  P6  P5  P4  P3  P2  P1  P0]

Bit = 1 → White pixel
Bit = 0 → Black pixel
```

### Memory Addressing

```c
// For pixel at (x, y):
byte_index = (y * 50) + (x / 8)
bit_position = x % 8

// To access pixel:
buffer[byte_index] & (0x80 >> bit_position)

// Example: Pixel at (127, 85)
// byte_index = 85 * 50 + 127/8 = 4250 + 15 = 4265
// bit_position = 127 % 8 = 7
// buffer[4265] bit 7
```

---

## 2. DISPLAY CONTROLLER RAM

The EPD controller has **TWO internal RAM buffers**:

### RAM 0x24: New Image Data
- Holds the NEW image to display
- Command `0x24` writes to this RAM

### RAM 0x26: Previous Image Data
- Holds the PREVIOUS image for comparison
- Command `0x26` writes to this RAM

### Why Two RAMs?

The controller compares RAM 0x24 vs RAM 0x26 to determine:
- Which pixels need to change
- What waveform to apply (black→white, white→black, or no change)

---

## 3. INITIALIZATION MODES

### EPD_4IN2_V2_Init() - Regular Mode
```c
EPD_4IN2_V2_Init();
```

**Purpose**: Standard initialization for **partial refresh support**

**What it does**:
1. Sends command `0x12` (SWRESET) - Software reset
2. Sets command `0x21` with data `0x40, 0x00` - Display update control
3. Sets command `0x3C` with data `0x05` - Border waveform control
4. Sets command `0x11` with data `0x03` - Data entry mode (X-increment, Y-increment)
5. Sets display window to full screen (0,0)→(399,299)
6. Sets cursor to (0,0)

**Use for**: Partial refresh operations, normal display updates

---

### EPD_4IN2_V2_Init_Fast() - Fast Mode
```c
EPD_4IN2_V2_Init_Fast(Seconds_1S);  // or Seconds_1_5S
```

**Purpose**: Faster refresh with reduced quality

**What it does**:
1. Same as regular init BUT
2. Sets command `0x1A` (temperature register) with:
   - `0x5A` for ~1 second refresh
   - `0x6E` for ~1.5 second refresh
3. Sets command `0x22` with data `0x91` - Load temperature
4. Triggers command `0x20` - Master activation

**Trade-offs**:
- ✅ Faster updates (~1s vs ~2-3s)
- ❌ More ghosting/artifacts
- ❌ Less crisp transitions
- ⚠️ **Cannot use with partial refresh** - full screen only

---

## 4. FULL REFRESH - EPD_4IN2_V2_Display()

```c
void EPD_4IN2_V2_Display(UBYTE *Image)
{
    UWORD Width = 50;   // 400 / 8
    UWORD Height = 300;

    // Write NEW image to RAM 0x24
    EPD_4IN2_V2_SendCommand(0x24);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_4IN2_V2_SendData(Image[i + j * Width]);
        }
    }

    // Write SAME image to RAM 0x26 (previous)
    EPD_4IN2_V2_SendCommand(0x26);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_4IN2_V2_SendData(Image[i + j * Width]);
        }
    }

    // Trigger display update
    EPD_4IN2_V2_TurnOnDisplay();
}
```

### How It Works:

1. **Sends entire buffer to RAM 0x24** (new image)
2. **Sends entire buffer to RAM 0x26** (previous image)
3. **Triggers display update**: Command `0x22` with data `0xC7`, then command `0x20`
4. **Controller applies full waveform** - complete refresh sequence
5. **Waits for BUSY signal** to go LOW

**Result**: Clean, crisp image with no ghosting

**Time**: ~2-3 seconds

**Buffer requirement**: Pass full 15,000 byte buffer starting at Image[0]

---

## 5. PARTIAL REFRESH - EPD_4IN2_V2_PartialDisplay()

```c
void EPD_4IN2_V2_PartialDisplay(UBYTE *Image,
                                 UWORD Xstart, UWORD Ystart,
                                 UWORD Xend, UWORD Yend)
{
    // Convert X coordinates from PIXELS to BYTES
    if ((Xstart % 8 + Xend % 8 == 8 && Xstart % 8 > Xend % 8) ||
        Xstart % 8 + Xend % 8 == 0 ||
        (Xend - Xstart) % 8 == 0) {
        Xstart = Xstart / 8;
        Xend = Xend / 8;
    } else {
        Xstart = Xstart / 8;
        Xend = Xend % 8 == 0 ? Xend / 8 : Xend / 8 + 1;
    }

    // Calculate region size
    UWORD Width = Xend - Xstart;
    UWORD IMAGE_COUNTER = Width * (Yend - Ystart);

    // Adjust for display controller (endpoint = end-1)
    Xend -= 1;
    Yend -= 1;

    // Set border waveform
    EPD_4IN2_V2_SendCommand(0x3C);
    EPD_4IN2_V2_SendData(0x80);

    // Set display update control
    EPD_4IN2_V2_SendCommand(0x21);
    EPD_4IN2_V2_SendData(0x00);
    EPD_4IN2_V2_SendData(0x00);

    // Set border waveform again
    EPD_4IN2_V2_SendCommand(0x3C);
    EPD_4IN2_V2_SendData(0x80);

    // Set RAM X address start/end
    EPD_4IN2_V2_SendCommand(0x44);
    EPD_4IN2_V2_SendData(Xstart & 0xff);
    EPD_4IN2_V2_SendData(Xend & 0xff);

    // Set RAM Y address start/end
    EPD_4IN2_V2_SendCommand(0x45);
    EPD_4IN2_V2_SendData(Ystart & 0xff);
    EPD_4IN2_V2_SendData((Ystart >> 8) & 0x01);
    EPD_4IN2_V2_SendData(Yend & 0xff);
    EPD_4IN2_V2_SendData((Yend >> 8) & 0x01);

    // Set RAM X address counter
    EPD_4IN2_V2_SendCommand(0x4E);
    EPD_4IN2_V2_SendData(Xstart & 0xff);

    // Set RAM Y address counter
    EPD_4IN2_V2_SendCommand(0x4F);
    EPD_4IN2_V2_SendData(Ystart & 0xff);
    EPD_4IN2_V2_SendData((Ystart >> 8) & 0x01);

    // Write data to RAM 0x24
    EPD_4IN2_V2_SendCommand(0x24);
    for (i = 0; i < IMAGE_COUNTER; i++) {
        EPD_4IN2_V2_SendData(Image[i]);
    }

    // Trigger partial update
    EPD_4IN2_V2_TurnOnDisplay_Partial();
}
```

### CRITICAL: How Partial Refresh Expects the Buffer

**The Image buffer parameter MUST contain ONLY the region data, NOT the full screen!**

```
WRONG ❌:
Pass full screen buffer (15,000 bytes)
Function tries to read Image[0] to Image[IMAGE_COUNTER-1]
→ Reads wrong part of memory
→ Corruption, ghosting, artifacts

CORRECT ✅:
Extract region into separate buffer
Pass region buffer containing ONLY that region's bytes
Function reads Image[0] to Image[IMAGE_COUNTER-1] correctly
→ Clean partial update
```

### Example: Partial Refresh Region (100, 50) to (200, 150)

```c
// Region: X=100→200 (100 pixels wide), Y=50→150 (100 pixels tall)

// Step 1: Calculate byte-aligned coordinates
uint16_t XstartByte = 100 / 8 = 12  // byte column 12
uint16_t XendByte = 200 / 8 = 25    // byte column 25
uint16_t Width = 25 - 12 = 13 bytes per row
uint16_t Height = 150 - 50 = 100 rows

// Step 2: Calculate buffer size
uint16_t bufferSize = 13 bytes × 100 rows = 1,300 bytes

// Step 3: Allocate temp buffer
UBYTE *regionBuffer = (UBYTE*)malloc(1300);

// Step 4: Extract region from full screen buffer
for (uint16_t row = 0; row < 100; row++) {
    uint16_t srcOffset = (50 + row) * 50 + 12;  // Source row in full buffer
    uint16_t dstOffset = row * 13;              // Dest row in region buffer
    memcpy(&regionBuffer[dstOffset], &fullScreenBuffer[srcOffset], 13);
}

// Step 5: Call partial refresh
EPD_4IN2_V2_PartialDisplay(regionBuffer, 100, 50, 200, 150);

// Step 6: Free buffer
free(regionBuffer);
```

---

## 6. TURN ON DISPLAY COMMANDS

### Full Refresh
```c
EPD_4IN2_V2_TurnOnDisplay()
{
    EPD_4IN2_V2_SendCommand(0x22);  // Display Update Control 2
    EPD_4IN2_V2_SendData(0xC7);     // Full update sequence
    EPD_4IN2_V2_SendCommand(0x20);  // Master Activation
    EPD_4IN2_V2_ReadBusy();         // Wait for completion
}
```

### Partial Refresh
```c
EPD_4IN2_V2_TurnOnDisplay_Partial()
{
    EPD_4IN2_V2_SendCommand(0x22);  // Display Update Control 2
    EPD_4IN2_V2_SendData(0xFF);     // Partial update sequence
    EPD_4IN2_V2_SendCommand(0x20);  // Master Activation
    EPD_4IN2_V2_ReadBusy();         // Wait for completion
}
```

**Key Difference**: `0xC7` vs `0xFF`
- `0xC7`: Full waveform (clean, slow)
- `0xFF`: Partial waveform (fast, may ghost)

---

## 7. COMMON MISTAKES & SOLUTIONS

### ❌ Mistake 1: Using full buffer with partial refresh
```c
// WRONG
EPD_4IN2_V2_PartialDisplay(fullScreenBuffer, 100, 50, 200, 150);
```
**Problem**: Function reads from fullScreenBuffer[0], which is pixel (0,0), not (100,50)

**Solution**: Extract region first
```c
// CORRECT
UBYTE *region = extractRegion(fullScreenBuffer, 100, 50, 200, 150);
EPD_4IN2_V2_PartialDisplay(region, 100, 50, 200, 150);
free(region);
```

### ❌ Mistake 2: Not byte-aligning coordinates
```c
// May cause issues
EPD_4IN2_V2_PartialDisplay(buffer, 123, 50, 187, 150);
```
**Problem**: X=123 starts mid-byte, X=187 ends mid-byte

**Solution**: Align to 8-pixel boundaries
```c
// Align X to multiples of 8
uint16_t X_aligned = (123 / 8) * 8;  // = 120
uint16_t W_aligned = ((187 + 7) / 8) * 8 - X_aligned;  // Width to cover 123-187
```

### ❌ Mistake 3: Not clearing region before redraw
```c
// WRONG
drawDigit(x, y, newDigit);
partialRefresh(x, y, w, h);
```
**Problem**: Old pixels remain, creating ghost images

**Solution**: Clear then draw
```c
// CORRECT
Paint_DrawRectangle(x, y, x+w, y+h, WHITE, ..., DRAW_FILL_FULL);  // Clear
drawDigit(x, y, newDigit);  // Draw
partialRefresh(x, y, w, h);  // Refresh
```

### ❌ Mistake 4: Updating RAM 0x26 incorrectly
```c
// WRONG: Never updating RAM 0x26
EPD_4IN2_V2_Display(newImage);  // Updates both 0x24 and 0x26
partialRefresh(...);             // Only updates 0x24
partialRefresh(...);             // Only updates 0x24
// → Controller compares 0x24 vs old 0x26 → artifacts
```

**Solution**: Full refresh periodically, or manually update 0x26 after partial refresh

---

## 8. RECOMMENDED ARCHITECTURE

### Dual Framebuffer Approach
```c
UBYTE *CurrentFrame;   // Working buffer (draw here)
UBYTE *PreviousFrame;  // Last displayed state

// On full refresh:
EPD_4IN2_V2_Display(CurrentFrame);
memcpy(PreviousFrame, CurrentFrame, 15000);

// On partial refresh:
// 1. Draw into CurrentFrame
Paint_DrawSomething(...);

// 2. Extract changed region
UBYTE *region = extractRegion(CurrentFrame, x, y, x+w, y+h);

// 3. Partial refresh
EPD_4IN2_V2_PartialDisplay(region, x, y, x+w, y+h);

// 4. Update PreviousFrame for that region
updateRegion(PreviousFrame, CurrentFrame, x, y, w, h);

// 5. Free temp buffer
free(region);
```

---

## 9. WORKING EXAMPLE FROM simple_timer_bitmap.cpp

```c
void partialRefresh(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    // Byte-align
    uint16_t refreshX = (x / 8) * 8;
    uint16_t refreshWidth = ((w + 7) / 8) * 8;
    uint16_t refreshXEnd = refreshX + refreshWidth;
    uint16_t refreshYEnd = y + h;

    // Boundary check
    if (refreshXEnd > SCREEN_WIDTH) refreshXEnd = SCREEN_WIDTH;
    if (refreshYEnd > SCREEN_HEIGHT) refreshYEnd = SCREEN_HEIGHT;

    // Buffer size
    UWORD bytesPerRow = 50;  // 400/8
    UWORD regionBytes = refreshWidth / 8;
    UWORD bufferSize = regionBytes * h;

    // Allocate temp buffer
    UBYTE *buffer = (UBYTE*)malloc(bufferSize);
    if (!buffer) return;

    // Extract region using memcpy
    UWORD startX = refreshX / 8;
    for (UWORD row = 0; row < h; row++) {
        if ((y + row) >= SCREEN_HEIGHT) break;
        UWORD srcOff = (y + row) * bytesPerRow + startX;
        UWORD dstOff = row * regionBytes;
        memcpy(&buffer[dstOff], &BlackImage[srcOff], regionBytes);
    }

    // Partial refresh
    EPD_4IN2_V2_PartialDisplay(buffer, refreshX, y, refreshXEnd, refreshYEnd);

    // Free
    free(buffer);
}
```

This pattern WORKS because:
1. ✅ Extracts region into separate buffer
2. ✅ Byte-aligns coordinates
3. ✅ Uses memcpy for efficient row copy
4. ✅ Passes region buffer (not full screen)
5. ✅ Frees memory after

---

## SUMMARY

| Aspect | Full Refresh | Partial Refresh |
|--------|-------------|-----------------|
| Init | EPD_4IN2_V2_Init() | EPD_4IN2_V2_Init() |
| Function | EPD_4IN2_V2_Display() | EPD_4IN2_V2_PartialDisplay() |
| Buffer Input | Full screen (15,000 bytes) | Region only (extracted) |
| Speed | ~2-3 seconds | ~200-400ms |
| Quality | Perfect, no ghosting | May ghost over time |
| RAM Usage | Updates 0x24 and 0x26 | Updates 0x24 only |
| When to Use | Initial draw, mode change, reset | Frequent updates (timer, progress) |
