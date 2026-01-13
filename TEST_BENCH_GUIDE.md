# Partial Refresh Test Bench Guide

## Overview

This test bench systematically verifies partial refresh functionality with:
- **Two independent regions** that update separately
- **Double framebuffer** (Current + Previous) for delta detection
- **Region-specific buffers** for partial refresh extraction
- **XOR-based change detection** to verify what actually changed
- **Cross-contamination checks** to ensure regions don't affect each other

## Test Configuration

### Region 1: Top-Left Counter Box
- **Position**: (50, 50)
- **Size**: 100×80 pixels
- **Content**: Incrementing counter "R1:0", "R1:1", "R1:2"...
- **Update Pattern**: Every 6 seconds (alternating with Region 2)

### Region 2: Bottom-Right Pattern Box
- **Position**: (250, 170)
- **Size**: 100×80 pixels
- **Content**: Alternating between filled circles and "R2" text
- **Update Pattern**: Every 6 seconds (alternating with Region 1)

## Architecture

```
┌──────────────────────────────────────────────┐
│  PARTIAL REFRESH TEST                        │
├──────────────────────────────────────────────┤
│                                              │
│   ┌──────────┐                               │
│   │  R1:5    │  ← Region 1 (Counter)        │
│   │          │                               │
│   └──────────┘                               │
│                                              │
│                         ┌──────────┐         │
│                         │  ●   ●   │ ← R2    │
│                         │          │         │
│                         └──────────┘         │
│                                              │
│ Region 1: Counter    Region 2: Pattern      │
└──────────────────────────────────────────────┘
```

## Buffer Structure

### 1. Full Screen Buffers (15,000 bytes each)
```c
UBYTE *CurrentFrame;   // Working buffer - all drawing happens here
UBYTE *PreviousFrame;  // Last refreshed state - for delta detection
```

### 2. Region Extraction Buffers
```c
// Region 1: 100×80 pixels = 13 bytes/row × 80 rows = 1,040 bytes
UBYTE *Region1Buffer;

// Region 2: 100×80 pixels = 13 bytes/row × 80 rows = 1,040 bytes
UBYTE *Region2Buffer;
```

## Test Sequence

### Cycle 1 (t=0s): Initial Full Refresh
```
1. Clear entire screen
2. Draw static content (title, labels)
3. Draw Region 1 with counter = 0
4. Draw Region 2 with empty pattern
5. Full refresh: EPD_4IN2_V2_Display(CurrentFrame)
6. Sync: memcpy(PreviousFrame, CurrentFrame, 15000)
```

### Cycle 2 (t=3s): Update Region 1
```
1. Draw Region 1 with counter = 1 into CurrentFrame
2. XOR check: Compare CurrentFrame vs PreviousFrame for Region 1
   → Should detect CHANGE
3. Extract Region 1 → Region1Buffer
4. Partial refresh: EPD_4IN2_V2_PartialDisplay(Region1Buffer, ...)
5. Update PreviousFrame for Region 1 area only
6. Cross-check: Compare Region 2 in CurrentFrame vs PreviousFrame
   → Should detect NO CHANGE (no contamination)
```

### Cycle 3 (t=6s): Update Region 2
```
1. Draw Region 2 with filled circles into CurrentFrame
2. XOR check: Compare CurrentFrame vs PreviousFrame for Region 2
   → Should detect CHANGE
3. Extract Region 2 → Region2Buffer
4. Partial refresh: EPD_4IN2_V2_PartialDisplay(Region2Buffer, ...)
5. Update PreviousFrame for Region 2 area only
6. Cross-check: Compare Region 1 in CurrentFrame vs PreviousFrame
   → Should detect NO CHANGE (no contamination)
```

### Cycles 4-N: Alternating updates
Continues alternating between Region 1 and Region 2 every 3 seconds.

## Expected Serial Output

```
========================================
  PARTIAL REFRESH TEST BENCH
========================================

Allocated framebuffers: 2 x 15000 bytes
Allocated region buffers: R1=1040 bytes, R2=1040 bytes

Drawing initial full screen...
Initial display complete!

========================================
TEST SEQUENCE:
- Every 3 seconds, update Region 1 (counter++)
- Every 3 seconds, update Region 2 (pattern toggle)
- Alternates between regions
- XOR check before each refresh
========================================

----------------------------------------
TEST CYCLE 1
----------------------------------------
Phase: UPDATE REGION 1
Drawing Region 1: Number 1
  XOR delta check: CHANGED
  Partial refresh: X=48->152, Y=50->130
  Refresh: DONE
  Cross-check Region 2: OK

----------------------------------------
TEST CYCLE 2
----------------------------------------
Phase: UPDATE REGION 2
Drawing Region 2: Pattern FILLED
  XOR delta check: CHANGED
  Partial refresh: X=248->352, Y=170->250
  Refresh: DONE
  Cross-check Region 1: OK

----------------------------------------
TEST CYCLE 3
----------------------------------------
Phase: UPDATE REGION 1
Drawing Region 1: Number 3
  XOR delta check: CHANGED
  Partial refresh: X=48->152, Y=50->130
  Refresh: DONE
  Cross-check Region 2: OK
```

## What We're Testing

### ✅ Correct Behavior (What Should Happen)

1. **Delta Detection Works**
   - XOR between CurrentFrame and PreviousFrame correctly identifies changed regions
   - Only changed regions trigger refresh

2. **Partial Refresh Works**
   - Region extraction creates correct buffer
   - EPD_4IN2_V2_PartialDisplay updates only that region
   - Display shows updated content in correct location

3. **No Cross-Contamination**
   - Updating Region 1 doesn't affect Region 2
   - Updating Region 2 doesn't affect Region 1
   - Cross-check always reports "OK"

4. **Visual Quality**
   - No ghosting or artifacts
   - Clean transitions
   - Text and graphics render correctly

### ❌ Failure Modes (What to Watch For)

1. **Delta Detection Fails**
   - XOR reports "NO CHANGE" when region was drawn
   - XOR reports "CHANGED" when nothing was drawn
   - → Buffer copy or comparison issue

2. **Partial Refresh Corruption**
   - Wrong data appears in region
   - Region shows garbage/noise
   - → Buffer extraction issue

3. **Cross-Contamination**
   - Cross-check reports "CONTAMINATED!"
   - Updating one region changes the other
   - → Incorrect buffer boundaries or refresh coordinates

4. **Visual Artifacts**
   - Ghosting (old image remains)
   - Flickering
   - Wrong positioning
   - → EPD controller state corruption

## Key Functions Explained

### `extractRegion()`
```c
// Copies a rectangular region from CurrentFrame into a region-specific buffer
// - Handles byte alignment
// - Uses memcpy for row-by-row copy
// - Result: regionBuffer[0] = first byte of region, not full screen
```

### `hasRegionChanged()`
```c
// XOR comparison between CurrentFrame and PreviousFrame for a region
// Returns true if ANY byte differs
// This is the delta detection mechanism
```

### `updatePreviousRegion()`
```c
// After successful partial refresh, copy that region from CurrentFrame to PreviousFrame
// This syncs the two buffers for future delta detection
```

### `partialRefreshRegion()`
```c
// Complete partial refresh workflow:
// 1. Extract region → regionBuffer
// 2. Call EPD_4IN2_V2_PartialDisplay(regionBuffer, x, y, xEnd, yEnd)
// 3. Update PreviousFrame for that region
```

## How to Run

### Build and Upload
```bash
pio run -e test_partial_refresh -t upload
```

### Monitor Serial Output
```bash
pio device monitor
```

### What to Observe

1. **On the display**:
   - Region 1 should show "R1:0", then "R1:1", "R1:2", etc.
   - Region 2 should alternate between filled circles and "R2" text
   - Static content should never change
   - No ghosting or artifacts

2. **In serial output**:
   - XOR should detect CHANGED for the region being updated
   - Cross-check should always show OK
   - No "CONTAMINATED!" messages
   - Refresh coordinates should match region boundaries

## Success Criteria

✅ **Test PASSES if**:
- Both regions update independently
- XOR correctly detects changes
- No cross-contamination
- Clean visual appearance
- No display corruption

❌ **Test FAILS if**:
- Regions don't update
- XOR detection is wrong
- Cross-contamination occurs
- Ghosting or artifacts appear
- Display shows garbage

## Debugging Tips

### If regions don't update:
- Check buffer extraction (print first few bytes)
- Verify EPD_4IN2_V2_PartialDisplay coordinates
- Ensure region boundaries are byte-aligned

### If ghosting occurs:
- Verify PreviousFrame is being updated after each refresh
- Check that full screen refresh happened initially
- May need periodic full refresh

### If cross-contamination:
- Print buffer offsets during extraction
- Verify region coordinates don't overlap
- Check memcpy lengths

### If garbage appears:
- Region buffer may be too small
- Byte alignment may be wrong
- Check calculateRegionBufferSize() result
