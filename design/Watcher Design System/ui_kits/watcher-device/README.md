# Watcher Device UI Kit

## Overview
High-fidelity interactive recreation of the Watcher e-paper device UI.
Canvas: 400×300px (matches 4.2" e-paper display).

## Screens
- **Clock** — analog + digital modes, date string
- **Digital Clock** — 7-segment style display  
- **Pomodoro Timer** — dot-ring progress, mode buttons
- **Calendar** — month grid, nav arrows
- **Task List** — checkbox tasks

## Notes
- Fonts substituted: IBM Plex Mono + Space Grotesk (originals not provided)
- 7-segment digits implemented in CSS/JS (faithful to SVG geometry)
- No shadows, no gradients — strict e-paper palette
- Interaction: click mode buttons to switch screens; timer runs live
