# Watcher Design System

## Product Overview

**Watcher** is an ESP32-S3 e-paper desktop productivity companion. It lives on your desk and displays useful information on a 4.2-inch black-and-white e-paper screen. No glowing LCD, no distractions — just quiet, paper-like presence.

### Features / Screens
| Screen | Description |
|---|---|
| **Clock** | Analog or digital (7-segment style) time display with date |
| **Pomodoro Timer** | Focus/Break timer with circular dot-ring progress indicator |
| **Tasklist** | Simple text task list |
| **Calendar** | Month view calendar grid |
| **OTA** | Over-the-air firmware updates |
| **Obsidian integration** | Syncs with Obsidian notes/tasks |

### Sources Provided
- `uploads/The Watcher.png` — Screenshot sheet of all UI screens (3 rows × ~5 screens)
- `uploads/Blank Screen.svg` — Figma export of the full screen strip with all modules
- `uploads/Clock.svg` — 7-segment digit glyph SVG (4-digit display)
- `uploads/Pomodoro Timer.svg` — Pomodoro timer screen
- `uploads/SECONDS.svg` — Seconds dot-matrix row component
- `uploads/ermine logo.png` — Ermine mascot with glasses (first prototype mascot)

---

## CONTENT FUNDAMENTALS

### Tone & Voice
- **Quiet and functional.** No marketing speak, no cheerful filler copy. The UI is a tool.
- **Lowercase preferred** for labels and UI text (e.g. `focus`, `break`, `long`, `stop`)
- **ALL CAPS** used for mode labels/buttons (e.g. `FOCUS`, `BREAK`, `LONG BREAK`, `STOP`)
- **Minimal copy.** E-paper screen is small; every character counts.
- **No emoji.** The e-paper display is B/W; emoji are never used.
- **No first/second person** — the UI speaks in imperatives and labels, not "you" or "I"
- **Numeric precision** — time is shown as `HH:MM` or `HH:MM:SS`, always padded
- **Day/date format:** `Saturday, 24th Sept. 2025` — spelled-out weekday, ordinal date
- **No punctuation flourishes** — minimal periods, no exclamations

### Examples of copy in the UI
- `FOCUS` `BREAK` `LONG` `STOP` `START` `PAUSE`
- `On Break` (mixed case for status messages)
- `Session` (section label)
- `January 2022` (month/year header in calendar)
- `Sun Mon Tue Wed Thu Fri Sat` (abbreviated day headers)
- `Saturday, 24th Sept. 2025` (full date string under clock)

---

## VISUAL FOUNDATIONS

### Color System
Watcher uses a strict **4-color e-paper palette** derived from hardware constraints:

| Token | Value | Usage |
|---|---|---|
| `--ink` | `#000000` | Primary content, filled elements, active segments |
| `--paper` | `#FFFFFF` | Background, screen surface |
| `--ghost` | `#E6E6E6` | Inactive 7-segment digits, subtle borders |
| `--dim` | `#171717` | Active digit segments (near-black, slightly warm) |
| `--mid` | `#888888` | Mid-gray, dithering midtone |
| `--light` | `#CCCCCC` | Light gray for secondary elements |

**No gradients.** E-paper cannot display them. All tonal variation is achieved via **4-bit dithering patterns** (Floyd-Steinberg or ordered).

### Typography
- **Target font: Monocraft** — a pixel-art monospace font by IdreesInc, inspired by Minecraft's typeface. Available at https://github.com/IdreesInc/Monocraft (OTF self-hosted). Not yet finalized for the product.
- **Display / Clock:** Custom 7-segment digit SVG glyphs for the device; Monocraft as fallback for web.
- **Everything is monospace** — no separate sans-serif body font. The e-paper aesthetic is mono throughout.
- ⚠️ *Font not yet decided/finalized. Design system uses **Share Tech Mono** (Google Fonts) as a web placeholder until Monocraft OTF is self-hosted.*
- **Casing:** ALL CAPS for action buttons; Title Case for status; lowercase for secondary labels.
- **Scale:** Very small — ~8–12px at device resolution (400×300px). Pixel fonts read well at these sizes.

### Layout & Spacing
- **Screen canvas:** 400×300px (4.2" e-paper native resolution approximation)
- **Content area:** Full bleed, no padding wasted
- **Spacing unit:** 8px base grid
- **Sections divided by thin 1px black lines** or spatial gaps — no colored dividers
- **Fixed-width panels** — no responsive layout (fixed hardware screen)

### Shape & Corners
- **Buttons:** Pill shape — fully rounded (`border-radius: 12px` on ~24px tall buttons)
- **Digit panels:** Beveled hexagonal shape (chamfered corners via SVG paths, not CSS)
- **Calendar cells:** Square, no rounding
- **No shadows** — e-paper has no depth; shadows are never used
- **No elevation system** — everything is flat on the paper surface

### Iconography
- **Minimal.** Only a play triangle (▶) for START is used as an icon
- **No icon font** — icons are inline SVG paths
- **Unicode shapes** used sparingly (e.g. `◀ ▶` for navigation arrows)
- **No external icon library**

### Animation
- **None on device.** E-paper refresh is slow (~1–3 seconds); no animations exist in the product.
- **For design mockups/prototypes:** subtle fade transitions are acceptable to simulate screen changes.

### Imagery & Mascots
- **Ermine with glasses** — the first prototype mascot; B/W line art, thick outlines, friendly expression
- **Planned:** rotating themed animal mascots in B/W with 4-bit gray dithering
- **No photography.** All imagery is B/W illustration.
- **Style:** Clean line art, minimal fill, slightly rounded/cozy character design

### Hover & Press States (UI Kit / Web only)
- **Hover:** slight gray fill (`#F0F0F0`) on interactive elements
- **Press:** invert (black bg, white text)
- **Selected/Active:** solid black fill, white text (pill buttons)

### Cards & Containers
- No card system — the screen IS the container
- Modules separated by whitespace or thin 1px `#000` borders
- No rounded cards, no shadows, no borders on content blocks

---

## ICONOGRAPHY

Watcher uses no icon library. Icons are:
1. **Inline SVG paths** — the play button (▶) is a filled triangle path
2. **Unicode characters** — `<` `>` for prev/next navigation in Calendar header
3. **Dot matrices** — the seconds indicator is a row of 10×10px black squares
4. **Circular dot ring** — the Pomodoro progress ring is a ring of ~60 small dots

**No CDN icon library is used or recommended.** Keep icons as inline SVG or unicode to preserve the e-paper aesthetic.

### Assets Available
| File | Description |
|---|---|
| `assets/ermine-mascot.png` | Ermine mascot with glasses, B/W line art |
| `assets/digit-segments.svg` | 4-digit 7-segment display SVG (blank/ghost state) |
| `assets/screens/all-screens.png` | Full screen screenshot reference sheet |
| `assets/screens/blank-screen.svg` | Figma SVG strip of all screen modules |

---

## FILE INDEX

```
README.md                      ← This file
SKILL.md                       ← Agent skill definition
colors_and_type.css            ← CSS variables: colors, typography, spacing
assets/
  ermine-mascot.png            ← Ermine mascot illustration
  digit-segments.svg           ← 7-segment digit SVG glyphs
  screens/
    all-screens.png            ← Reference screenshot sheet
    blank-screen.svg           ← Figma SVG screen strip
preview/
  colors.html                  ← Color palette cards
  typography.html              ← Type specimens
  spacing.html                 ← Spacing & border tokens
  components-buttons.html      ← Button components
  components-digits.html       ← 7-segment digit display
  components-pomodoro.html     ← Pomodoro timer UI
  components-calendar.html     ← Calendar UI
  brand-mascot.html            ← Ermine mascot display
ui_kits/
  watcher-device/
    index.html                 ← Interactive device UI prototype
    README.md                  ← UI kit notes
```
