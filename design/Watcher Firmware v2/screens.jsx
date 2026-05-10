/* global React */
const { useEffect, useMemo, useRef, useState } = React;

// ── shared helpers ──────────────────────────────────────────────────────────
const MONTHS = ['January','February','March','April','May','June','July','August','September','October','November','December'];
const WDAYS = ['Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday'];
const WDAYS3 = ['Sun','Mon','Tue','Wed','Thu','Fri','Sat'];
const ORD = n => n + (n%10===1&&n!==11?'st':n%10===2&&n!==12?'nd':n%10===3&&n!==13?'rd':'th');

// ─── Screen frame ───────────────────────────────────────────────────────────
function Screen({ children, style }) {
  return (
    <div style={{
      width: 400, height: 300, background: '#fff', color: '#000',
      position: 'relative', overflow: 'hidden',
      fontFamily: "'Share Tech Mono','Courier New',monospace",
      ...style,
    }}>{children}</div>
  );
}

// ── 7-seg digit ─────────────────────────────────────────────────────────────
const SEG_MAP = {
  '0':[1,1,1,0,1,1,1],'1':[0,0,1,0,0,1,0],'2':[1,0,1,1,1,0,1],
  '3':[1,0,1,1,0,1,1],'4':[0,1,1,1,0,1,0],'5':[1,1,0,1,0,1,1],
  '6':[1,1,0,1,1,1,1],'7':[1,0,1,0,0,1,0],'8':[1,1,1,1,1,1,1],'9':[1,1,1,1,0,1,1]
};
const SEG_NAMES = [['seg-top','sh'],['seg-tl','sv'],['seg-tr','sv'],['seg-mid','sh'],['seg-bl','sv'],['seg-br','sv'],['seg-bot','sh']];

function Digit({ ch, scale = 1 }) {
  const segs = SEG_MAP[ch] || [0,0,0,0,0,0,0];
  return (
    <div className="digit" style={{
      transform: scale !== 1 ? `scale(${scale})` : undefined,
      transformOrigin: 'top left',
      width: 62, height: 110, position: 'relative', flexShrink: 0,
    }}>
      {segs.map((on, i) => (
        <div key={i}
          className={`seg ${SEG_NAMES[i][1]} ${SEG_NAMES[i][0]}`}
          style={{ background: on ? '#111' : '#E6E6E6' }}
        />
      ))}
    </div>
  );
}

function Colon({ visible = true, scale = 1 }) {
  return (
    <div style={{
      transform: scale !== 1 ? `scale(${scale})` : undefined,
      transformOrigin: 'top left',
      display: 'flex', flexDirection: 'column',
      justifyContent: 'center', alignItems: 'center', gap: 16,
      width: 18, height: 110, paddingBottom: 6,
    }}>
      <div style={{ width: 9, height: 9, background: visible ? '#111' : 'transparent' }} />
      <div style={{ width: 9, height: 9, background: visible ? '#111' : 'transparent' }} />
    </div>
  );
}

// ─── SVG-based 7-seg (scales cleanly via viewBox) ───────────────────────────
// Native units: digit 62×110, colon 18×110. Pass `size` (height in px); width is computed.
function SvgDigit({ ch, size = 110, on = '#111', off = '#E6E6E6' }) {
  const segs = SEG_MAP[ch] || [0,0,0,0,0,0,0];
  const w = 62 * (size / 110);
  // segment shapes in 62×110 native space (matching CSS clip-paths)
  const shapes = [
    // 0 top:     left:8 width:46 height:9 chamfered
    "M14,0 L48,0 L54,4.5 L48,9 L14,9 L8,4.5 Z",
    // 1 tl:      left:0 top:9 width:9 height:41 chamfered vertical
    "M4.5,14 L0,18.5 L0,45 L4.5,50 L9,45 L9,18.5 Z",
    // 2 tr:      right:0 top:9 (x = 53..62, y=9..50)
    "M57.5,14 L53,18.5 L53,45 L57.5,50 L62,45 L62,18.5 Z",
    // 3 mid:     top:50
    "M14,50 L48,50 L54,54.5 L48,59 L14,59 L8,54.5 Z",
    // 4 bl:      left:0 top:60
    "M4.5,65 L0,69.5 L0,96 L4.5,101 L9,96 L9,69.5 Z",
    // 5 br:      right:0 top:60
    "M57.5,65 L53,69.5 L53,96 L57.5,101 L62,96 L62,69.5 Z",
    // 6 bot:     top:101
    "M14,101 L48,101 L54,105.5 L48,110 L14,110 L8,105.5 Z",
  ];
  return (
    <svg width={w} height={size} viewBox="0 0 62 110" style={{ display: 'block', flexShrink: 0 }}>
      {shapes.map((d, i) => (
        <path key={i} d={d} fill={segs[i] ? on : off} />
      ))}
    </svg>
  );
}

function SvgColon({ visible = true, size = 110, on = '#111' }) {
  const w = 18 * (size / 110);
  return (
    <svg width={w} height={size} viewBox="0 0 18 110" style={{ display: 'block', flexShrink: 0 }}>
      {visible && (
        <>
          <rect x="4.5" y="40" width="9" height="9" fill={on} />
          <rect x="4.5" y="65" width="9" height="9" fill={on} />
        </>
      )}
    </svg>
  );
}

// ─── 1. ANALOG CLOCK ────────────────────────────────────────────────────────
function AnalogClockScreen({ now }) {
  // 4.2" e-paper analog clock, centered face. Plain ticks and hands.
  const cx = 200, cy = 150, r = 120;
  const h = now.getHours() % 12, m = now.getMinutes(), s = now.getSeconds();
  const hourAng  = ((h + m/60) / 12) * Math.PI * 2 - Math.PI/2;
  const minAng   = ((m + s/60) / 60) * Math.PI * 2 - Math.PI/2;
  const secAng   = (s / 60) * Math.PI * 2 - Math.PI/2;
  const numerals = [12,1,2,3,4,5,6,7,8,9,10,11];
  return (
    <Screen>
      <svg width="400" height="300" viewBox="0 0 400 300">
        {/* tick marks */}
        {Array.from({length: 60}).map((_, i) => {
          const a = (i / 60) * Math.PI * 2 - Math.PI/2;
          const isHr = i % 5 === 0;
          const r1 = isHr ? r - 10 : r - 4;
          const r2 = r;
          return (
            <line key={i}
              x1={cx + Math.cos(a)*r1} y1={cy + Math.sin(a)*r1}
              x2={cx + Math.cos(a)*r2} y2={cy + Math.sin(a)*r2}
              stroke="#000" strokeWidth={isHr ? 2 : 1} />
          );
        })}
        {/* numerals */}
        {numerals.map((n, i) => {
          const a = (i / 12) * Math.PI * 2 - Math.PI/2;
          const tr = r - 22;
          return (
            <text key={n}
              x={cx + Math.cos(a)*tr} y={cy + Math.sin(a)*tr + 5}
              textAnchor="middle"
              fontFamily="'Share Tech Mono', monospace"
              fontSize="13" fill="#000">
              {n}
            </text>
          );
        })}
        {/* hour hand */}
        <line x1={cx} y1={cy}
          x2={cx + Math.cos(hourAng) * (r - 50)}
          y2={cy + Math.sin(hourAng) * (r - 50)}
          stroke="#000" strokeWidth="4" strokeLinecap="round" />
        {/* minute hand */}
        <line x1={cx} y1={cy}
          x2={cx + Math.cos(minAng) * (r - 24)}
          y2={cy + Math.sin(minAng) * (r - 24)}
          stroke="#000" strokeWidth="2.5" strokeLinecap="round" />
        {/* second hand */}
        <line x1={cx} y1={cy}
          x2={cx + Math.cos(secAng) * (r - 14)}
          y2={cy + Math.sin(secAng) * (r - 14)}
          stroke="#000" strokeWidth="1" strokeLinecap="round" />
        {/* center pivot */}
        <circle cx={cx} cy={cy} r="3" fill="#000" />
      </svg>
    </Screen>
  );
}

// ─── 1b. DIGITAL CLOCK — hybrid 7-seg + perimeter % of day ring ─────────────
// 60-square perimeter ring (18 cols × 14 rows), evenly inset from screen edge.
// Top-left → clockwise. Each square = 1/60 of the day → 24 minutes.
const DAY_RING = (() => {
  // Geometry: square=10px, x-pitch=22, y-pitch=21 → centered with ~8px pad
  const SQ = 10;
  const PX = 22, PY = 21;
  const COLS = 18, ROWS = 14;
  const W = (COLS - 1) * PX + SQ;        // 384
  const H = (ROWS - 1) * PY + SQ;        // 283
  const x0 = Math.round((400 - W) / 2);  // 8
  const y0 = Math.round((300 - H) / 2);  // 8 (or 9)
  const pos = [];
  // top row L→R
  for (let i = 0; i < COLS; i++) pos.push([x0 + i * PX, y0]);
  // right col T→B (skip top corner already added)
  for (let i = 1; i < ROWS; i++) pos.push([x0 + (COLS - 1) * PX, y0 + i * PY]);
  // bottom row R→L (skip right corner)
  for (let i = COLS - 2; i >= 0; i--) pos.push([x0 + i * PX, y0 + (ROWS - 1) * PY]);
  // left col B→T (skip both corners)
  for (let i = ROWS - 2; i >= 1; i--) pos.push([x0, y0 + i * PY]);
  return pos; // length 60
})();

function DigitalClockScreen({ now, digitH = 116, dateShift = 30, ringMode = 'seconds' }) {
  const hh = String(now.getHours()).padStart(2, '0');
  const mm = String(now.getMinutes()).padStart(2, '0');
  const s  = now.getSeconds();
  const colonOn = true;
  const dateStr = `${WDAYS[now.getDay()]}, ${ORD(now.getDate())} ${MONTHS[now.getMonth()].slice(0,3)}. ${now.getFullYear()}`;

  // Ring fill behavior:
  //   'seconds' — base state is all filled. A clockwise "wave" cycles 60s at
  //               a time, alternating empty-pass and fill-pass. So each second
  //               the next square clockwise toggles state.
  //   'day'     — % of day, filled clockwise.
  let getFill;
  if (ringMode === 'day') {
    const pct = (now.getHours() * 3600 + now.getMinutes() * 60 + s) / 86400;
    const filled = Math.floor(pct * 60);
    getFill = (i) => i < filled;
  } else {
    // Total cycle = 120s. First 60s: emptying. Next 60s: filling.
    const cyclePos = (now.getMinutes() * 60 + s) % 120;
    const emptying = cyclePos < 60;
    const head = emptying ? cyclePos : cyclePos - 60;
    // emptying: indices 0..head are empty, rest filled.
    // filling : indices 0..head are filled, rest empty.
    getFill = (i) => emptying ? i > head : i <= head;
  }

  // 7-seg time. Digit height drives entire scale. Width = h * 62/110.
  const DIGIT_H = digitH;
  const digitW  = 62 * (DIGIT_H / 110);
  const colonW  = 18 * (DIGIT_H / 110);
  const gap = 8;
  const totalW = digitW * 4 + colonW + gap * 4;
  const startX = Math.round(200 - totalW / 2);
  // Center clock vertically, then nudge up a bit so date has room beneath
  const startY = Math.round(150 - DIGIT_H / 2 - 10);

  return (
    <Screen>
      {/* perimeter ring */}
      <svg width="400" height="300" style={{ position: 'absolute', inset: 0 }}>
        {DAY_RING.map(([x, y], i) => (
          <rect key={i} x={x} y={y} width="10" height="10"
            fill={getFill(i) ? '#000' : '#E6E6E6'} />
        ))}
      </svg>

      {/* 7-seg time */}
      <div style={{
        position: 'absolute', left: startX, top: startY,
        display: 'flex', alignItems: 'center', gap,
      }}>
        <SvgDigit ch={hh[0]} size={DIGIT_H} />
        <SvgDigit ch={hh[1]} size={DIGIT_H} />
        <SvgColon visible={colonOn} size={DIGIT_H} />
        <SvgDigit ch={mm[0]} size={DIGIT_H} />
        <SvgDigit ch={mm[1]} size={DIGIT_H} />
      </div>

      {/* date — sits roughly halfway between clock bottom and screen bottom */}
      <div style={{
        position: 'absolute', left: 0, right: 0,
        top: startY + DIGIT_H + dateShift,
        textAlign: 'center', fontSize: 13, letterSpacing: '0.04em', color: '#000',
      }}>{dateStr}</div>
    </Screen>
  );
}

// ─── 2. ALARM ───────────────────────────────────────────────────────────────
function AlarmScreen() {
  const rawAlarms = [
    { time: '06:30', label: 'wake',     on: true,  days: 'M T W T F · ·' },
    { time: '07:45', label: 'leave',    on: true,  days: 'M T W T F · ·' },
    { time: '09:00', label: 'standup',  on: true,  days: 'M T W T F · ·' },
    { time: '13:30', label: 'lunch',    on: false, days: '· · · · · · ·' },
    { time: '17:00', label: 'shutdown', on: true,  days: 'M T W T F · ·' },
    { time: '19:15', label: 'workout',  on: true,  days: '· · W · · · ·' },
    { time: '21:00', label: 'medicine', on: true,  days: 'M T W T F S S' },
    { time: '22:30', label: 'wind down',on: false, days: 'M T W T F S S' },
  ];
  // ascending by time
  const alarms = [...rawAlarms].sort((a, b) => a.time.localeCompare(b.time));
  const focus = 4; // shutdown — demonstrates auto-scroll on long lists

  // 400×300 canvas. Header 28px → list area = 272px. Row height fixed at 36px.
  const HEADER_H = 28;
  const LIST_H = 300 - HEADER_H;
  const ROW_H = 36;

  const listRef = React.useRef(null);
  React.useEffect(() => {
    const el = listRef.current;
    if (!el) return;
    const focusRow = el.querySelector(`[data-row="${focus}"]`);
    if (!focusRow) return;
    const rowTop = focus * ROW_H;
    const rowBottom = rowTop + ROW_H;
    const viewTop = el.scrollTop;
    const viewBottom = viewTop + LIST_H;
    if (rowTop < viewTop) {
      el.scrollTop = rowTop;
    } else if (rowBottom > viewBottom) {
      el.scrollTop = rowBottom - LIST_H;
    }
  }, [focus, alarms.length]);

  return (
    <Screen>
      {/* header */}
      <div style={{
        display: 'flex', justifyContent: 'space-between', alignItems: 'center',
        height: HEADER_H, padding: '0 14px', borderBottom: '1px solid #000',
      }}>
        <span style={{ fontSize: 11, letterSpacing: '0.18em', fontWeight: 700 }}>ALARMS</span>
        <span style={{ fontSize: 9, letterSpacing: '0.18em', opacity: 0.55 }}>
          {alarms.filter(a => a.on).length} / {alarms.length} ACTIVE
        </span>
      </div>

      {/* scrollable list */}
      <div ref={listRef} style={{
        height: LIST_H, overflowY: 'auto', overflowX: 'hidden',
        scrollbarWidth: 'none',
      }}>
        <style>{`.alarm-scroll::-webkit-scrollbar{display:none}`}</style>
        <div className="alarm-scroll">
          {alarms.map((a, i) => {
            const focused = i === focus;
            return (
              <div key={i} data-row={i} style={{
                display: 'grid',
                gridTemplateColumns: '14px 56px 1fr 76px 18px',
                gap: 10, alignItems: 'center',
                height: ROW_H, padding: '0 14px',
                borderBottom: '1px solid #E6E6E6',
                background: focused ? '#000' : 'transparent',
                color: focused ? '#fff' : '#000',
                boxSizing: 'border-box',
              }}>
                <span style={{ fontSize: 11 }}>{focused ? '▶' : ' '}</span>
                <span style={{ fontSize: 17, letterSpacing: '0.04em' }}>{a.time}</span>
                <span style={{
                  fontSize: 10, letterSpacing: '0.16em',
                  opacity: focused ? 0.9 : 0.75,
                  whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis',
                }}>{a.label}</span>
                <span style={{
                  fontSize: 8, letterSpacing: '0.18em',
                  opacity: focused ? 0.7 : 0.5,
                }}>{a.days}</span>
                {/* ON/OFF indicator: solid filled square = ON, outlined empty = OFF */}
                <span style={{
                  width: 12, height: 12, justifySelf: 'end',
                  border: `1.5px solid ${focused ? '#fff' : '#000'}`,
                  background: a.on ? (focused ? '#fff' : '#000') : 'transparent',
                  boxSizing: 'border-box',
                }} />
              </div>
            );
          })}
        </div>
      </div>
    </Screen>
  );
}

// ─── 3. POMODORO ────────────────────────────────────────────────────────────
// Reuses the same 60-square DAY_RING perimeter as the digital clock so the
// "seconds wave" is visually consistent across screens.

function PomodoroScreen({
  now,
  mode = 'focus',          // 'focus' | 'break' | 'long'
  running = true,          // false → paused, shows ▶ instead of ⏸
  session = 1,             // current session index
  totalSessions = 3,       // total sessions in this set (header shows "SESSION 1/3")
  minutesLeft = 24,        // big 2-digit minutes-only readout (00–99)
  digitH = 150,            // tweakable digit height (matches reference proportions)
  titleSize = 11,          // tweakable header text size
}) {
  // Seconds-wave ring: identical logic to DigitalClockScreen seconds mode.
  // 120-second cycle — first 60s empties one square per second clockwise from
  // top-left, next 60s refills. Default visual is "mostly full", matching the
  // reference asset where the ring is densely filled.
  const s = now ? now.getSeconds() : 0;
  const m = now ? now.getMinutes() : 0;
  const cyclePos = (m * 60 + s) % 120;
  const emptying = cyclePos < 60;
  const head = emptying ? cyclePos : cyclePos - 60;
  const getFill = (i) => emptying ? i > head : i <= head;

  // Big minutes-only digit pair, sized to match reference (~150px tall).
  const DIGIT_H = digitH;
  const digitW  = 62 * (DIGIT_H / 110);
  const gap = 12;
  const totalW = digitW * 2 + gap;
  const startX = Math.round(200 - totalW / 2);
  // Vertically center within the inner safe area (header 28 → bottom controls ~50)
  // Inner band roughly y=28..250 → center ~139
  const startY = Math.round(139 - DIGIT_H / 2);

  const mm = String(Math.max(0, Math.min(99, minutesLeft))).padStart(2, '0');
  const sessionLabel = `SESSION ${session}/${totalSessions}`;

  // Icon stroke matches text weight on e-ink. 18×18 viewBox icons.
  const Icon = ({ d, fill = false, size = 22 }) => (
    <svg width={size} height={size} viewBox="0 0 18 18" style={{ display: 'block' }}>
      <path d={d} fill={fill ? '#000' : 'none'} stroke="#000" strokeWidth="1.6"
        strokeLinejoin="round" strokeLinecap="round" />
    </svg>
  );
  const PlayIcon  = () => <Icon d="M4 3 L15 9 L4 15 Z" fill={true} />;
  const PauseIcon = () => (
    <svg width="22" height="22" viewBox="0 0 18 18" style={{ display: 'block' }}>
      <rect x="4" y="3" width="3.4" height="12" fill="#000" />
      <rect x="10.6" y="3" width="3.4" height="12" fill="#000" />
    </svg>
  );
  const StopIcon  = () => (
    <svg width="22" height="22" viewBox="0 0 18 18" style={{ display: 'block' }}>
      <rect x="3.5" y="3.5" width="11" height="11" fill="#000" />
    </svg>
  );

  return (
    <Screen>
      {/* perimeter ring — same geometry as DigitalClockScreen */}
      <svg width="400" height="300" style={{ position: 'absolute', inset: 0 }}>
        {DAY_RING.map(([x, y], i) => (
          <rect key={i} x={x} y={y} width="10" height="10"
            fill={getFill(i) ? '#000' : '#E6E6E6'} />
        ))}
      </svg>

      {/* header — positioned below top ring row, in inner safe area */}
      <div style={{
        position: 'absolute', top: 24, left: 32, right: 32,
        display: 'flex', justifyContent: 'space-between', alignItems: 'center',
        height: titleSize + 4,
      }}>
        <span style={{ fontSize: titleSize, letterSpacing: '0.18em', fontWeight: 700 }}>
          POMODORO
        </span>
        <span style={{ fontSize: Math.max(8, titleSize - 2), letterSpacing: '0.14em', opacity: 0.6, whiteSpace: 'nowrap' }}>
          {sessionLabel}
        </span>
      </div>

      {/* big minutes-only readout */}
      <div style={{
        position: 'absolute', left: startX, top: startY,
        display: 'flex', alignItems: 'center', gap,
      }}>
        <SvgDigit ch={mm[0]} size={DIGIT_H} />
        <SvgDigit ch={mm[1]} size={DIGIT_H} />
      </div>

      {/* bottom controls: play/pause · pills · stop — sits ABOVE bottom ring row */}
      <div style={{
        position: 'absolute', bottom: 30, left: 0, right: 0,
        display: 'grid', gridTemplateColumns: '1fr auto 1fr', alignItems: 'center',
        padding: '0 32px',
      }}>
        {/* play / pause (left) */}
        <div style={{ justifySelf: 'start' }}>
          {running ? <PauseIcon /> : <PlayIcon />}
        </div>
        {/* mode pills (center) */}
        <div style={{ display: 'flex', gap: 6 }}>
          {['FOCUS', 'BREAK', 'LONG'].map(label => {
            const active = label === mode.toUpperCase();
            return (
              <span key={label} style={{
                fontSize: 8, letterSpacing: '0.14em', height: 22,
                borderRadius: 11, padding: '0 12px',
                border: '1px solid #000',
                background: active ? '#000' : '#fff',
                color: active ? '#fff' : '#000',
                display: 'inline-flex', alignItems: 'center',
              }}>{label}</span>
            );
          })}
        </div>
        {/* stop (right) */}
        <div style={{ justifySelf: 'end' }}>
          <StopIcon />
        </div>
      </div>
    </Screen>
  );
}

// ─── 4. CALENDAR ────────────────────────────────────────────────────────────
function CalendarScreen() {
  const year = 2026, month = 4; // May 2026 (May 5 today per system note)
  const today = 5;
  const events = [3, 7, 12, 19, 22, 28];
  const first = new Date(year, month, 1).getDay();
  const days = new Date(year, month + 1, 0).getDate();
  const prev = new Date(year, month, 0).getDate();
  const cells = [];
  for (let i = 0; i < first; i++) cells.push({ n: prev - first + 1 + i, other: true });
  for (let d = 1; d <= days; d++) cells.push({
    n: d, other: false, today: d === today, event: events.includes(d),
  });
  const trail = (7 - ((first + days) % 7)) % 7;
  for (let i = 1; i <= trail; i++) cells.push({ n: i, other: true });
  const rows = Math.ceil(cells.length / 7);
  const cellH = Math.floor(254 / rows);

  return (
    <Screen>
      {/* header */}
      <div style={{
        display:'flex', justifyContent:'space-between', alignItems:'center',
        height: 30, padding: '0 14px', borderBottom: '1px solid #000',
      }}>
        <span style={{ fontSize: 16 }}>‹</span>
        <span style={{ fontSize: 11, letterSpacing: '0.2em', fontWeight: 700 }}>
          {MONTHS[month].toUpperCase()} {year}
        </span>
        <span style={{ fontSize: 16 }}>›</span>
      </div>
      {/* day strip */}
      <div style={{
        display:'grid', gridTemplateColumns: 'repeat(7,1fr)',
        height: 16, borderBottom: '1px solid #000', textAlign: 'center',
      }}>
        {WDAYS3.map(d => (
          <span key={d} style={{
            fontSize: 8, letterSpacing: '0.14em', opacity: 0.6, lineHeight: '16px',
          }}>{d.toUpperCase()}</span>
        ))}
      </div>
      {/* grid */}
      <div style={{
        display:'grid', gridTemplateColumns: 'repeat(7,1fr)',
        gridTemplateRows: `repeat(${rows}, ${cellH}px)`, height: rows * cellH,
      }}>
        {cells.map((c, i) => (
          <div key={i} style={{
            position: 'relative',
            borderRight: (i+1) % 7 === 0 ? 'none' : '1px solid #E6E6E6',
            borderBottom: '1px solid #E6E6E6',
            display: 'flex', alignItems: 'center', justifyContent: 'center',
            fontSize: 12,
            opacity: c.other ? 0.2 : 1,
            background: c.today ? '#000' : 'transparent',
            color: c.today ? '#fff' : '#000',
            fontWeight: c.event || c.today ? 700 : 400,
          }}>
            <span>{c.n}</span>
            {c.event && (
              <span style={{
                position: 'absolute', bottom: 4, left: '50%',
                transform: 'translateX(-50%)',
                width: 8, height: 2,
                background: c.today ? '#fff' : '#000',
              }} />
            )}
          </div>
        ))}
      </div>
    </Screen>
  );
}

// ─── 5. TASKS ───────────────────────────────────────────────────────────────
function TasksScreen() {
  const tasks = [
    { text: 'finalize firmware v0.4 release notes', done: false, tag: 'WORK' },
    { text: 'reply to PCB fab quote',               done: false, tag: 'WORK' },
    { text: 'order replacement screen',             done: true,  tag: 'HW'   },
    { text: 'pick up groceries on the way home',    done: false, tag: 'HOME' },
    { text: 'review obsidian inbox',                done: false, tag: null   },
    { text: 'pay electric bill',                    done: true,  tag: 'BILL' },
    { text: 'read chapter 4',                       done: false, tag: 'READ' },
  ];
  const open = tasks.filter(t => !t.done).length;
  return (
    <Screen>
      <div style={{
        display:'flex', justifyContent:'space-between', alignItems:'center',
        height: 28, padding: '0 14px', borderBottom: '1px solid #000',
      }}>
        <span style={{ fontSize: 11, letterSpacing: '0.18em', fontWeight: 700 }}>TASKS</span>
        <span style={{ fontSize: 9, letterSpacing: '0.18em', opacity: 0.55 }}>
          OBSIDIAN · {open} OPEN
        </span>
      </div>
      <div style={{ padding: '2px 14px' }}>
        {tasks.map((t, i) => (
          <div key={i} style={{
            display:'grid', gridTemplateColumns: 'auto 1fr auto',
            gap: 10, alignItems: 'center', padding: '5px 0',
            borderBottom: i === tasks.length - 1 ? 'none' : '1px solid #E6E6E6',
          }}>
            <div style={{
              width: 11, height: 11,
              border: '1.5px solid #000',
              background: t.done ? '#000' : '#fff',
              position: 'relative',
            }}>
              {t.done && (
                <svg viewBox="0 0 11 11" width="11" height="11" style={{ position:'absolute', inset: -1.5 }}>
                  <polyline points="2,6 5,8 9,3" fill="none" stroke="#fff" strokeWidth="1.5" />
                </svg>
              )}
            </div>
            <div style={{
              fontSize: 11, lineHeight: 1.35,
              textDecoration: t.done ? 'line-through' : 'none',
              opacity: t.done ? 0.35 : 1,
            }}>{t.text}</div>
            {t.tag && (
              <span style={{
                fontSize: 7, letterSpacing: '0.14em',
                border: '1px solid #000',
                padding: '1px 5px', opacity: 0.65,
              }}>{t.tag}</span>
            )}
          </div>
        ))}
      </div>
    </Screen>
  );
}

// ─── 6. SETTINGS ────────────────────────────────────────────────────────────
function SettingsScreen() {
  const focus = 0;
  const rows = [
    { k: 'WIFI',     v: 'home_2.4' },
    { k: 'SIGNAL',   v: '-58 dBm · ▌▌▌▌·' },
    { k: 'IP',       v: '192.168.1.42' },
    { k: 'THEME',    v: 'LIGHT' },
    { k: 'FIRMWARE', v: 'v0.4.2' },
    { k: 'UPDATE',   v: 'v0.4.3 · READY' },
    { k: 'TIME ZONE',v: 'GMT-04:00' },
    { k: 'SLEEP',    v: '5 MIN' },
  ];
  return (
    <Screen>
      <div style={{
        display:'flex', justifyContent:'space-between', alignItems:'center',
        height: 28, padding: '0 14px', borderBottom: '1px solid #000',
      }}>
        <span style={{ fontSize: 11, letterSpacing: '0.18em', fontWeight: 700 }}>SETTINGS</span>
        <span style={{ fontSize: 9, letterSpacing: '0.18em', opacity: 0.55 }}>
          {focus + 1} / {rows.length}
        </span>
      </div>
      <div>
        {rows.map((r, i) => (
          <div key={r.k} style={{
            display: 'grid',
            gridTemplateColumns: '14px 96px 1fr',
            gap: 8, alignItems: 'center',
            padding: '5px 14px',
            borderBottom: '1px solid #E6E6E6',
            background: i === focus ? '#000' : 'transparent',
            color: i === focus ? '#fff' : '#000',
          }}>
            <span style={{ fontSize: 10 }}>{i === focus ? '▶' : ' '}</span>
            <span style={{ fontSize: 10, letterSpacing: '0.16em', fontWeight: 700 }}>{r.k}</span>
            <span style={{
              fontSize: 10, letterSpacing: '0.06em',
              opacity: i === focus ? 0.85 : 0.6,
              textAlign: 'right',
            }}>{r.v}</span>
          </div>
        ))}
      </div>
      <div style={{
        position:'absolute', bottom: 0, left: 0, right: 0,
        borderTop: '1px solid #000', padding: '5px 14px',
        fontSize: 8, letterSpacing: '0.18em', opacity: 0.55,
        display:'flex', justifyContent:'space-between',
      }}>
        <span>BTN1/2 NAV</span>
        <span>BTN3 SELECT</span>
      </div>
    </Screen>
  );
}

// ─── live clock hook ────────────────────────────────────────────────────────
function useNow() {
  const [now, setNow] = useState(() => new Date());
  useEffect(() => {
    const t = setInterval(() => setNow(new Date()), 1000);
    return () => clearInterval(t);
  }, []);
  return now;
}

Object.assign(window, {
  AnalogClockScreen, DigitalClockScreen,
  AlarmScreen, PomodoroScreen,
  CalendarScreen, TasksScreen, SettingsScreen,
  useNow,
});
