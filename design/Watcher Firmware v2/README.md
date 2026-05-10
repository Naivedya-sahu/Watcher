# Watcher — Original Web Console screens

Two deliverables in one project:

1. **Web prototypes** — `Watcher Original Screens v2.html`
   Pannable Design Canvas with one 400×300 artboard per screen (Clock,
   Alarm, Pomodoro, Calendar, Tasks, Settings). Open the file and toggle
   **Tweaks** in the toolbar to play with digit sizes, ring modes, Pomodoro
   minutes/mode, etc.

2. **ESP32 firmware** — `firmware/`
   Mirrors the prototypes on real hardware. Targets the Lilygo T5 4.2"
   e-paper board out of the box; portable to any ESP32/ESP32-S3 wired to
   a 400×300 GxEPD2 panel. Three buttons: PREV / NEXT / SELECT.

## Building the firmware

```
cd firmware
pio run -t upload   # flash
pio device monitor  # 115200 serial
```

Default pin mapping (override in `src/main.cpp`):

| Function | GPIO |
|----------|------|
| PREV     | 39   |
| NEXT     | 34   |
| SELECT   | 35   |

## Layout system

Both renderers share a single coordinate system so the prototypes are pixel-
faithful to the device:

* **DAY_RING** — 60-square perimeter (18 cols × 14 rows), 10 px squares,
  22 px x-pitch, 21 px y-pitch, 8 px inset. Top-left → clockwise.
* **Seconds wave** — 120 s cycle. First 60 s clears one square per second;
  second 60 s repaints them. Default visual is a fully filled ring.
* **7-segment digits** — native 62 × 110, scaled by a `size` (height) param.
  Width = `size × 62/110`.
* **Header bar** — 28 px tall, single hairline below.

If you change geometry, change it in *both* the JSX (`screens.jsx` /
`ui/DayRing` constants) and the C++ (`firmware/src/ui/DayRing.cpp`).
