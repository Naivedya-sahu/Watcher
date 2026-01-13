# The Watcher - Figma Design Analysis & Export Requirements

## Overview
Analysis of The Watcher.png Figma mockup showing 15 screen variations across 5 core screen types.

Display Target: 400×300px (Waveshare 4.2" e-paper, B/W)

---

## Screen Inventory (15 total variations)

### 1. CLOCK SCREENS (5 variations)

#### Clock-Analog-Simple (Row 1, Col 1)
- **Elements**: Hour/minute/second hands, hour markers (1-12), tick marks
- **State**: Static display, no date
- **Export Status**: ✓ EXISTS as "Clock (Default) [Analog].svg"
- **Additional Exports Needed**: NONE

#### Clock-Analog-Detailed (Row 2, Col 1)
- **Elements**: Same as simple + minute tick marks between hours
- **State**: More detailed face with second hand sweep position
- **Export Status**: ✗ NOT EXPORTED
- **Additional Exports Needed**: 
  - clock-analog-detailed.svg (full screen)
  - OR components: hour-hand.svg, minute-hand.svg, second-hand.svg, clock-face.svg

#### Clock-Digital-Simple (Row 1, Col 2)
- **Elements**: 4×7-segment digits (HH:MM), colon separator
- **State**: Minimalist, no date
- **Export Status**: ✓ EXISTS as "Clock (Default) [Digital].svg"
- **Additional Exports Needed**: NONE

#### Clock-Digital-With-Date (Row 2, Col 2)
- **Elements**: Large 7-segment time (18:36), date text below ("Saturday, 20th Sept, 2025")
- **State**: Production-ready with full context
- **Export Status**: PARTIAL (existing SVG doesn't include date section)
- **Additional Exports Needed**:
  - clock-digital-with-date.svg (complete layout)
  - OR reuse existing + text rendering region coordinates

#### Clock-Digital-With-Week (Row 3, Col 1)
- **Elements**: Time display + weekday labels below
- **State**: Shows current day emphasis
- **Export Status**: ✗ NOT EXPORTED
- **Additional Exports Needed**:
  - clock-digital-with-week.svg
  - weekday-labels.svg (component)

---

### 2. CALENDAR SCREENS (3 variations)

#### Calendar-Light (Row 1, Col 3)
- **Elements**: Month grid, day headers (Sun-Sat), nav arrows
- **State**: Light theme, empty dates, month/year selector arrows
- **Export Status**: ✓ EXISTS as "Calendar [Light].svg"
- **Additional Exports Needed**: NONE

#### Calendar-With-Date-Selected (Row 2, Col 3)
- **Elements**: January 2022, date "2" highlighted/selected, full grid populated
- **State**: Interactive state showing date selection
- **Export Status**: PARTIAL (light version exists but no selection indicator)
- **Additional Exports Needed**:
  - date-selection-box.svg (component for highlighting)
  - filled-calendar-grid.svg (with all dates rendered)

#### Calendar-Dark (Row 3, Col 3)
- **Elements**: Same grid layout, inverted colors (white on black)
- **State**: Dark theme variant
- **Export Status**: ✗ NOT EXPORTED
- **Additional Exports Needed**:
  - calendar-dark.svg (full screen)
  - This may be OPTIONAL if you generate dark mode programmatically via color inversion

---

### 3. TIMER SCREENS (3 variations)

#### Timer-Idle (Row 1, Col 4)
- **Elements**: Circular progress ring (empty), center "START" and "STOP" buttons
- **State**: Timer not running, ready to start
- **Export Status**: ✓ EXISTS as "Timer.svg"
- **Additional Exports Needed**: NONE

#### Timer-Active (Row 2, Col 4)
- **Elements**: Time "00:24:17" center, partial progress ring (roughly 40% filled), START/STOP buttons
- **State**: Timer actively counting
- **Export Status**: PARTIAL (existing SVG likely shows idle state)
- **Additional Exports Needed**:
  - progress-segment.svg (for drawing variable progress)
  - OR mathematical approach: calculate arc coordinates in code

#### Timer-Components (Row 3, Col 2)
- **Elements**: Individual digit display showing "0 1 2 9 4" and "5 6 1 8 0 3"
- **State**: Design reference for digit spacing/sizing
- **Export Status**: ✓ EXISTS (digits 0-9.svg already exported individually)
- **Additional Exports Needed**: NONE

---

### 4. POMODORO SCREENS (4 variations)

#### Pomodoro-Idle (Row 1, Col 5)
- **Elements**: Circular progress ring, "FOCUS", "BREAK", "LONG" buttons, "START"/"STOP"
- **State**: Session not started
- **Export Status**: ✓ EXISTS as "Pomodoro Timer.svg"
- **Additional Exports Needed**: NONE

#### Pomodoro-Active (Row 2, Col 5)
- **Elements**: Time "24:17" center, "On Break" label, partial progress, START/STOP
- **State**: Break session running
- **Export Status**: PARTIAL (existing SVG likely shows idle)
- **Additional Exports Needed**:
  - session-label.svg (component: "On Break", "Focusing", etc.)
  - Same progress rendering as Timer

#### Pomodoro-Session-Variants (Row 2, Col 6 + Row 3, Col 3)
- **Elements**: Same layout, different time displays
- **State**: Different session types/states
- **Export Status**: Redundant with Pomodoro-Active
- **Additional Exports Needed**: NONE (covered by session-label component)

#### Session-Manager (Row 3, Col 4)
- **Elements**: Full digit set (0-9), "On Break" label, all control buttons:
  - FOCUS, BREAK, LONG, START, PAUSE, CONTINUE, STOP
- **State**: Master reference for all button types
- **Export Status**: PARTIAL
- **Additional Exports Needed**:
  - button-focus.svg
  - button-break.svg
  - button-long.svg
  - button-start.svg
  - button-pause.svg
  - button-continue.svg
  - button-stop.svg
  - (All buttons as individual components)

---

### 5. YEAR/MONTH SELECTOR (1 screen)

#### Date-Selector (Row 3, Col 5)
- **Elements**: Month grid with navigation arrows, all months listed
- **State**: UI for selecting month/year
- **Export Status**: ✗ NOT EXPORTED
- **Additional Exports Needed**:
  - month-selector.svg (full screen)
  - month-list-component.svg (reusable list layout)
  - nav-arrow-left.svg, nav-arrow-right.svg

---

## COMPONENT BREAKDOWN

### Already Exported (✓)
- [x] Digits 0-9 (10 files)
- [x] Colon separator
- [x] Clock (Analog - simple variant)
- [x] Clock (Digital - simple variant)
- [x] Calendar (Light theme)
- [x] Timer (idle state)
- [x] Pomodoro (idle state)

### Missing Critical Exports (✗)

#### Core Screens
- [ ] Clock-Digital-With-Date (full layout)
- [ ] Clock-Digital-With-Week (full layout)
- [ ] Calendar-Dark (full screen)
- [ ] Month-Selector (full screen)

#### UI Components
- [ ] Date selection indicator box
- [ ] Session state labels ("On Break", "Focusing", "Long Break")
- [ ] Navigation arrows (left/right)
- [ ] All button types (8 unique buttons)

#### Analog Clock Components (OPTIONAL)
- [ ] Clock hands (hour/minute/second) as separate SVGs
- [ ] Clock face with different detail levels
- Note: These could be rendered programmatically instead

#### Progress Ring (OPTIONAL)
- [ ] Progress arc segments
- Note: Can be calculated mathematically in code

---

## DESIGN COMPLETENESS ASSESSMENT

### ✓ Design is Complete For:
1. **Clock (Digital - Basic)**: Fully defined, ready to implement
2. **Timer (Basic)**: Idle state complete, progress can be rendered dynamically
3. **Pomodoro (Basic)**: Idle state complete, same dynamic progress as Timer
4. **Calendar (Light)**: Structure complete, selection indicator needed
5. **Individual Digits**: All 10 digits cleanly exported

### ⚠️ Design Needs Clarification For:
1. **Progress Ring Rendering**:
   - Are progress segments pre-rendered at intervals (e.g., 0%, 25%, 50%, 75%, 100%)?
   - OR calculated as arcs dynamically in code?
   - **Recommendation**: Dynamic calculation (less memory, smooth updates)

2. **Button States**:
   - Are there pressed/hover states for buttons?
   - OR just on/off binary states (likely for e-paper)?
   - **Recommendation**: Binary states only (active/inactive)

3. **Date Selection Indicator**:
   - Border thickness, corner radius, color?
   - Should match existing calendar grid spacing
   - **Recommendation**: Simple rectangular border, 2-3px thick

### ✗ Design Missing Entirely:
1. **Settings Screen**: Not visible in mockup
   - Likely needs: menu list, value selectors, save/cancel buttons
2. **Error/Status Messages**: No indication of notification UI
3. **Battery/System Indicators**: If device is battery-powered
4. **WiFi/Sync Status**: If network features planned

---

## EXPORT PRIORITY RECOMMENDATIONS

### Priority 1 (Implement First):
1. Clock-Digital-With-Date.svg - Most common use case
2. Button components (all 8) - Needed for interactivity
3. Date selection indicator - Required for calendar navigation

### Priority 2 (Implement Next):
4. Session state labels (3 variants) - Pomodoro functionality
5. Navigation arrows (left/right) - Calendar/settings navigation
6. Clock-Digital-With-Week.svg - Nice-to-have variant

### Priority 3 (Optional/Later):
7. Clock-Analog-Detailed.svg - Analog clock is less critical
8. Calendar-Dark.svg - Can be generated via color inversion
9. Month-Selector.svg - Advanced feature, lower priority
10. Analog clock hands as components - Use vector rendering instead

---

## CUSTOM_FONT_HANDLER INTEGRATION

### Repository Details
- **URL**: https://github.com/Naivedya-sahu/Custom_Font_Handler
- **Purpose**: Convert TTF/OTF fonts to C bitmap arrays for e-paper displays
- **Language**: Python
- **Target Platform**: Arduino/ESP32

### Integration Strategy

#### Phase 1: Static Bitmap Digits (Current)
- Use existing 0-9.svg exports converted to bitmaps
- Fast rendering, minimal processing
- Fixed size: ~52×96px per digit

#### Phase 2: Dynamic Text Rendering (Future)
- Integrate Custom_Font_Handler for date strings, labels, UI text
- Allow user-uploadable fonts via web interface
- Generate bitmap arrays at compile-time or runtime

### Font Requirements Analysis

Based on mockup text elements:
1. **Date Strings**: "Saturday, 20th Sept, 2025" - ~20-24pt
2. **Session Labels**: "On Break", "Focusing" - ~16-20pt
3. **Button Labels**: "START", "STOP", "FOCUS", etc. - ~12-16pt
4. **Weekday Labels**: "Mon", "Tue", "Wed" - ~10-12pt
5. **Calendar Dates**: "1", "2", "31" - ~12-14pt (numeric only)

### Custom_Font_Handler Use Cases

#### Use Case 1: Compile-Time Font Generation
```
Input:  Roboto-Regular.ttf, size 20pt, chars "0-9, A-Z, a-z, :,"
Output: font_roboto_20.h (C bitmap array)
Usage:  #include "font_roboto_20.h" in firmware
```

#### Use Case 2: User-Uploaded Fonts (Advanced)
```
1. User uploads TTF via web interface
2. ESP32 stores TTF in flash/SPIFFS
3. On-device conversion: TTF → bitmap arrays
4. Update display with new font
5. Persist font selection in EEPROM
```

### Implementation Recommendation

**Immediate (Phase 1)**:
- Use Waveshare GUI_Paint built-in fonts for text rendering
- Focus on getting digit-based screens working
- Bitmap digits from 0-9.svg for clock/timer

**Short-Term (Phase 2-3)**:
- Integrate Custom_Font_Handler as build tool
- Pre-generate 2-3 font sizes at compile-time
- Add to tools/font-handler/ directory

**Long-Term (Phase 4+)**:
- Web interface for font upload
- On-device font conversion (if flash/RAM permits)
- User font library management

---

## MISSING DESIGN ELEMENTS TO CREATE

If you want feature-complete Figma exports, create these:

### Critical Missing Screens:
1. **Settings Screen**
   - Menu structure: WiFi, Time, Display, Habits, About
   - Value adjustment UI (rotary encoder simulation)
   - Save/Cancel confirmation

2. **Status/Notification Overlay**
   - Connection status (WiFi icon)
   - Sync status (cloud icon)
   - Battery level (if applicable)
   - Error messages ("Sync Failed", "Low Battery")

3. **Habit Tracking Screen** (if planned)
   - Habit list view
   - Checkboxes/completion indicators
   - Streak counter
   - Progress visualization

### Nice-to-Have Components:
4. **Loading Indicator** - For sync/update operations
5. **Confirmation Dialogs** - "Are you sure?" type overlays
6. **Input Feedback** - Visual feedback for button presses

---

## FINAL RECOMMENDATIONS

### What You Have:
✓ Complete core screen layouts (5 types)
✓ All digit components (0-9)
✓ Multiple state variations showing usage context
✓ Clear visual design language (B/W, high contrast)

### What You Need to Export:
1. **8 Button Components** (highest priority)
2. **Date Selection Indicator** (for calendar)
3. **Session Labels** (for pomodoro)
4. **Navigation Arrows** (for calendar/menus)

### What You Can Skip:
- Analog clock variants (use vector rendering)
- Dark theme calendar (color inversion in code)
- Progress ring segments (mathematical calculation)
- Clock hand components (vector drawing)

### Design Phase Assessment:
**Overall Status**: ~70% complete for MVP implementation

**Blockers**: Only button components are truly blocking. All screens can function without additional exports if you:
- Calculate progress arcs in code
- Use simple rectangle for date selection
- Render session labels as text with built-in fonts

**Recommendation**: Export the 8 buttons, then start implementation. Iterate on design details during development rather than blocking on comprehensive pre-export.

---

## NEXT STEPS

1. **Run watcher_reorganization.cmd** to create directory structure
2. **Export Missing Components** (buttons, selection indicator, arrows, labels)
3. **Manual File Migration** following script checklist
4. **Clone Custom_Font_Handler** into tools/font-handler/
5. **Begin Screen Implementation** (Clock-Digital-With-Date first)

---

## APPENDIX: File Naming Convention

When exporting additional components, use this naming:

```
Screens:
  clock-analog-simple.svg
  clock-analog-detailed.svg
  clock-digital-simple.svg
  clock-digital-with-date.svg
  clock-digital-with-week.svg
  calendar-light.svg
  calendar-dark.svg
  calendar-month-selector.svg
  timer-idle.svg
  pomodoro-idle.svg

Components - Digits:
  digit-0.svg through digit-9.svg
  colon.svg

Components - Buttons:
  btn-start.svg
  btn-stop.svg
  btn-pause.svg
  btn-continue.svg
  btn-focus.svg
  btn-break.svg
  btn-long.svg
  btn-generic.svg (if needed)

Components - UI Elements:
  selection-box.svg
  nav-arrow-left.svg
  nav-arrow-right.svg
  label-on-break.svg
  label-focusing.svg
  label-long-break.svg

Components - Icons:
  icon-wifi.svg
  icon-battery.svg
  icon-sync.svg
  icon-settings.svg
```

Naming convention: lowercase, hyphen-separated, descriptive, grouped by type.
