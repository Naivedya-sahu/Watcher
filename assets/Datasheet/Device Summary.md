# Desktop Productivity Device - Complete Design Specification

## Project Overview
**Objective**: Desktop productivity companion device with e-ink display for distraction-free operation
**Target Features**: Clock, Calendar, Tasks, Alarm, Timer, Pomodoro, Settings
**Design Philosophy**: Minimalist, grayscale palette, high contrast for e-ink compatibility

## Hardware Architecture

### Prototype Configuration
- **Main Controller**: nrf52840 dongle
- **Display**: 3.5" TFT (480×320) - Arduino Uno compatible shield
- **Coprocessor**: Arduino Uno (handles display refresh, button debouncing)
- **Input**: 3 mechanical switches + 1 rotary encoder with switch
- **Power**: AC powered (desktop use)

### Production Target
- **Controller**: ESP32-S3 or nrf52840
- **Display**: 4.2" E-ink (600×448)
- **Power**: Battery powered with ultra-low consumption

### Physical Layout
```
[Top Switch - Red] - Alarm/Timer control (start/stop/acknowledge)

[Main Display Area - 480×320]

[Side Panel]:
  [Upper Switch] - Menu/Screen selection
  [Rotary Encoder] - Value adjustment + confirm button
  [Lower Switch] - Back/Previous
```

## User Interface Architecture

### Navigation System
**Modal Navigation with Two States:**

1. **Browse Mode** (Default)
   - Menu button cycles: Clock → Calendar → Pomodoro → Timer → Tasks → Alarm → Settings
   - Back button: Returns to previous screen in sequence
   - Visual indicator: Browse icon in top-right

2. **Edit Mode** (Parameter adjustment)
   - Triggered: Encoder button press
   - Rotary encoder: Adjusts values
   - Back button: Exits Edit mode or cycles parameter hierarchy
   - Visual indicator: Edit icon + focus box around selected parameter

### Screen Sequence & Functions

#### 1. Clock (Default Boot Screen)
- **Display**: Large time, date below
- **Modes**: Analog/Digital toggle (user-defined, persistent)
- **Navigation**: View-only, menu button for screen change
- **Edit**: No editing capability

#### 2. Calendar
- **Display**: Month grid view
- **Navigation**: Focus box on dates
- **Edit Mode Hierarchy**: Year → Month → Day → Exit
- **Controls**: 
  - Browse: Rotary selects date, menu/back for navigation
  - Edit: Rotary adjusts values, encoder button switches units

#### 3. Pomodoro
- **Display**: Current time remaining (large), session info (small)
- **Progress**: Bottom progress bar (not circular for e-ink compatibility)
- **Session Data**: Saved to non-volatile memory
- **Background**: Continues running when navigating to other screens

#### 4. Timer
- **Display**: Countdown time remaining
- **Controls**: Top button starts/stops/resets
- **Edit**: Time setting via rotary encoder with unit switching
- **Background**: Runs in background with status indicator

#### 5. Tasks (Future Notion Integration)
- **Display**: Simple list with checkboxes
- **Navigation**: Focus box selects task row
- **Actions**: Encoder press toggles completion
- **Categories**: Fixed tasks vs temporary tasks

#### 6. Alarm
- **Display**: List view of all alarms
- **Navigation**: Focus box selects alarm row
- **Edit**: Encoder press enters time editing for selected alarm
- **Control**: Top button turns alarms on/off

#### 7. Settings
- **Display**: Menu list format showing current values
- **Categories**: 
  - Device settings (persistent, explicit save)
  - Visual/accessibility settings (explicit save)
  - Screen-based changes (auto-save)

### Input Control Mapping

#### Hardware Buttons
1. **Top Switch (Red)**: Context-sensitive primary action
   - Clock/Calendar: No function
   - Timers/Pomodoro: Start/Stop/Reset
   - Alarm: Acknowledge/Snooze off
   - Settings: Apply/Save

2. **Upper Switch**: Menu navigation
   - Browse Mode: Cycle through main screens
   - Edit Mode: No function (prevents accidental navigation)

3. **Rotary Encoder**: 
   - **Rotation**: 
     - Browse Mode: Navigate options within screen
     - Edit Mode: Adjust selected parameter value
     - Direction: Clockwise = increase, Counter-clockwise = decrease
   - **Button Press**: Enter/exit Edit mode, switch parameter focus

4. **Lower Switch**: Back/Previous
   - Browse Mode: Previous screen in sequence
   - Edit Mode: Exit current parameter level or Edit mode entirely

### Visual Feedback System

#### Focus Indication
- **Selected Parameter**: Thick border box with tick mark
- **Available Parameters**: Thin border
- **Focus Animation**: Box shifts smoothly between parameters

#### Status Indicators
- **Mode Indicator**: Top-right corner (Browse/Edit icons)
- **Background Processes**: Bottom status line with process indicators
  - Dot = running
  - Exclamation = completed/attention needed
- **Save Confirmation**: "Saved" text display for 1-2 seconds

#### Value Boundaries
- **Time Wrapping**: 23:59 → 00:00 with visual flash
- **Date Wrapping**: December → January with year increment
- **Visual Feedback**: Brief reverse animation on boundary wrap

### Background Process Management

#### Priority System
1. **Alarm** (Highest) - Always interrupts other processes
2. **Pomodoro** - Session-based priority
3. **Timer** (Lowest) - Basic countdown functionality

#### Process States
- **Single Active Rule**: Only one process can demand attention (sound/notification)
- **Silent Background**: Other processes run without interruption
- **Auto-Return**: Screen returns to active process when completed
- **User Control**: Top button acknowledges completion, returns to previous screen

### Data Persistence Strategy

#### Non-Volatile Storage (Flash Memory)
- **Pomodoro Sessions**: Complete session history with timestamps
- **Device Settings**: User preferences, display modes
- **Alarm Configurations**: Time settings, enabled states
- **Task Data**: Local cache with sync timestamps

#### Volatile Memory
- **Current Session**: Active timer/pomodoro progress
- **Navigation State**: Last screen, edit mode status
- **Power Loss Behavior**: Resets to default state, shows "Session Reset" message

### Error Handling & Edge Cases

#### Error Display Pattern
- **Visual**: Center "!" icon with descriptive text
- **Recovery**: Any button press or 3-second timeout returns to normal
- **Common Errors**: "No Alarms Set", "Invalid Time", "Sync Failed"

#### Input Conflict Resolution
1. **Safety Priority**: Top button (alarm off) always takes precedence
2. **Simultaneous Inputs**: First input registered, others ignored for 200ms
3. **Encoder During Button**: Rotation ignored during button press

#### Value Validation
- **Time Limits**: Automatic boundary wrapping with visual feedback
- **Parameter Bounds**: Clear indication of minimum/maximum values
- **Invalid States**: Automatic correction with user notification

## E-ink Design Constraints

### Refresh Strategy
- **Static Updates**: Major changes only (no smooth animations)
- **Update Frequency**: 
  - Clock: 5-10 minute intervals
  - Timers: Minute-level updates only
  - Status changes: Immediate but minimal

### Visual Adaptations
- **Progress Indicators**: Linear bars instead of circular
- **Animations**: Discrete state changes, no smooth transitions
- **Content Updates**: Full screen refresh for major changes

## Display Specifications

### TFT Prototype (480×320)
- **Main Content Area**: 400×280 pixels (centered)
- **Status Bar**: 480×40 pixels (bottom)
- **Button Indicators**: 80×280 pixels (right edge)
- **Grid System**: 40px base unit (12×8 grid)

### E-ink Production (600×448)
- **Grid System**: 50px base unit (12×9 grid)
- **Content Scaling**: Proportional scaling maintaining aspect ratios
- **Contrast Optimization**: Pure black/white with minimal grays

## Future Integration Points

### Web Configuration Interface
- **Access Method**: Local WiFi network (router-style setup)
- **Configuration Pages**: WiFi setup, task management, device settings
- **Task Categories**: Fixed tasks vs temporary tasks
- **Sync Control**: Manual trigger, frequency settings

### Notion API Integration (Future)
- **Authentication**: Token-based, stored in secure flash
- **Sync Strategy**: Pull-only initially, potential bidirectional
- **Conflict Resolution**: Notion source of truth
- **Offline Resilience**: Local cache survives connectivity loss

## Development Phases

### Phase 1: Core Functionality (Current)
- Basic UI navigation system
- Timer/Pomodoro/Alarm functionality
- Local data persistence
- TFT prototype validation

### Phase 2: Hardware Optimization
- E-ink display integration
- Power optimization for battery operation
- Physical device enclosure design

### Phase 3: Connectivity Features
- Web configuration interface
- Basic task management
- WiFi setup and management

### Phase 4: Advanced Integration
- Notion API integration
- Advanced task synchronization
- Analytics and usage insights

## Key Design Principles

1. **Distraction-Free**: No unnecessary animations, notifications, or visual noise
2. **E-ink First**: All interactions designed around slow refresh constraints  
3. **Physical Controls**: Reliable hardware inputs over touch/gesture
4. **Persistent State**: Critical data survives power loss
5. **Single Focus**: One primary task/timer at a time
6. **Clear Hierarchy**: Obvious navigation paths and exit strategies
7. **Immediate Utility**: Functional clock display within seconds of power-on

## Critical Success Metrics

- **Boot Time**: <3 seconds to functional clock display
- **Battery Life**: >1 month typical use (e-ink version)
- **Navigation Efficiency**: <3 button presses to reach any function
- **Error Recovery**: All error states have clear exit paths
- **Data Integrity**: No data loss during normal power cycles