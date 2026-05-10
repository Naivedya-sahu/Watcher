/* global React */
const { useEffect, useMemo, useRef, useState } = React;

// ── shared helpers ──────────────────────────────────────────────────────────
const MONTHS = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December'];
const WDAYS = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'];
const WDAYS3 = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
const ORD = (n) => n + (n % 10 === 1 && n !== 11 ? 'st' : n % 10 === 2 && n !== 12 ? 'nd' : n % 10 === 3 && n !== 13 ? 'rd' : 'th');

// 58-dot ring positions
const RING_POS = (() => {
  const p = [];
  for (let i = 0; i < 18; i++) p.push([59 + i * 16, 49]);
  for (let i = 0; i < 11; i++) p.push([331, 65 + i * 16]);
  for (let i = 17; i >= 0; i--) p.push([59 + i * 16, 241]);
  for (let i = 10; i >= 0; i--) p.push([59, 65 + i * 16]);
  return p;
})();

// 7-seg digit map
const SEG_MAP = {
  '0': [1, 1, 1, 0, 1, 1, 1], '1': [0, 0, 1, 0, 0, 1, 0], '2': [1, 0, 1, 1, 1, 0, 1],
  '3': [1, 0, 1, 1, 0, 1, 1], '4': [0, 1, 1, 1, 0, 1, 0], '5': [1, 1, 0, 1, 0, 1, 1],
  '6': [1, 1, 0, 1, 1, 1, 1], '7': [1, 0, 1, 0, 0, 1, 0], '8': [1, 1, 1, 1, 1, 1, 1], '9': [1, 1, 1, 1, 0, 1, 1]
};
const SEG_NAMES = [['seg-top', 'sh'], ['seg-tl', 'sv'], ['seg-tr', 'sv'], ['seg-mid', 'sh'], ['seg-bl', 'sv'], ['seg-br', 'sv'], ['seg-bot', 'sh']];

function Digit({ ch, dark, scale = 1 }) {
  const segs = SEG_MAP[ch] || [0, 0, 0, 0, 0, 0, 0];
  return (
    <div className="digit" style={{ transform: scale !== 1 ? `scale(${scale})` : undefined, transformOrigin: 'top left' }}>
      {segs.map((on, i) =>
      <div key={i} className={`seg ${SEG_NAMES[i][1]} ${SEG_NAMES[i][0]} ${on ? dark ? 'on-d' : 'on' : dark ? 'off-d' : 'off'}`} />
      )}
    </div>);

}

function Colon({ visible = true, dark, scale = 1 }) {
  return (
    <div className="colon-wrap" style={{ transform: scale !== 1 ? `scale(${scale})` : undefined, transformOrigin: 'top left' }}>
      <div className="colon-dot" style={{ background: visible ? dark ? '#fff' : '#111' : 'transparent' }} />
      <div className="colon-dot" style={{ background: visible ? dark ? '#fff' : '#111' : 'transparent' }} />
    </div>);

}

function DotRing({ filled, dark }) {
  return (
    <svg width="400" height="300" style={{ position: 'absolute', top: 0, left: 0 }}>
      {RING_POS.map(([x, y], i) =>
      <rect key={i} x={x} y={y} width="10" height="10"
      fill={i < filled ? dark ? '#fff' : '#000' : dark ? '#222' : '#E6E6E6'} />
      )}
    </svg>);

}

// ── helpers for time/date formats ──────────────────────────────────────────
function fmtTime(now, fmt, colonOn) {
  let h = now.getHours();
  const m = now.getMinutes();
  const ampm = h >= 12 ? 'PM' : 'AM';
  if (fmt === '12h') {h = h % 12 || 12;}
  const hh = String(h).padStart(2, '0');
  const mm = String(m).padStart(2, '0');
  const sep = colonOn ? ':' : ' ';
  return { hh, mm, sep, ampm: fmt === '12h' ? ampm : null };
}

function fmtDate(now, fmt) {
  const wd = WDAYS[now.getDay()];
  const wd3 = WDAYS3[now.getDay()];
  const d = now.getDate();
  const mo = MONTHS[now.getMonth()];
  const mo3 = MONTHS[now.getMonth()].slice(0, 3);
  const y = now.getFullYear();
  switch (fmt) {
    case 'long':return `${wd}, ${ORD(d)} ${mo3}. ${y}`;
    case 'iso':return `${y}-${String(now.getMonth() + 1).padStart(2, '0')}-${String(d).padStart(2, '0')}`;
    case 'short':return `${wd3} ${d} ${mo3}`;
    case 'numeric':return `${String(d).padStart(2, '0')}/${String(now.getMonth() + 1).padStart(2, '0')}/${y}`;
    default:return `${wd}, ${ORD(d)} ${mo3}. ${y}`;
  }
}

// ── DAY CLOCK (merged with Digital — 7-seg + dot ring + date) ─────────────
function DayClockScreen({ now, dark, colonOn, timeFmt, dateFmt }) {
  const h = now.getHours(),m = now.getMinutes(),s = now.getSeconds();
  const pct = (h * 3600 + m * 60 + s) / (24 * 3600);
  const filled = Math.round(pct * 58);
  const t = fmtTime(now, timeFmt, colonOn);
  const dateStr = fmtDate(now, dateFmt);

  // Render compact 7-seg time inside dot ring (282x202 area at 59,49)
  // digit 62w·110h scaled 0.6 -> 37×66 ; 4 digits + colon ≈ 37*4 + 11 + gaps
  const sc = 0.62;
  const digitW = 62 * sc;
  const digitH = 110 * sc;
  const colonW = 18 * sc;
  const gap = 2;
  const totalW = digitW * 4 + colonW + gap * 4;
  const startX = 200 - totalW / 2;
  const startY = 150 - digitH / 2 - 14;

  return (
    <div className={`scr ${dark ? 'scr-dark' : ''}`}>
      <DotRing filled={filled} dark={dark} />
      {/* 7-seg time */}
      <div style={{ position: 'absolute', left: startX, top: startY, display: 'flex', gap }}>
        <Digit ch={t.hh[0]} dark={dark} scale={sc} />
        <div style={{ width: digitW }}><Digit ch={t.hh[1]} dark={dark} scale={sc} /></div>
        <div style={{ width: colonW }}><Colon visible={colonOn} dark={dark} scale={sc} /></div>
        <div style={{ width: digitW }}><Digit ch={t.mm[0]} dark={dark} scale={sc} /></div>
        <div style={{ width: digitW }}><Digit ch={t.mm[1]} dark={dark} scale={sc} /></div>
      </div>
      {/* AM/PM if 12h */}
      {t.ampm &&
      <div style={{ position: 'absolute', left: startX + totalW + 6, top: startY + 4,
        fontSize: 10, letterSpacing: '0.18em', opacity: 0.65 }}>{t.ampm}</div>
      }
      {/* date */}
      <div style={{ position: 'absolute', left: 0, right: 0, top: 200, textAlign: 'center',
        fontSize: 11, letterSpacing: '0.06em', opacity: 0.7 }}>{dateStr}</div>
      {/* % of day bar */}
      <div style={{ position: 'absolute', left: 0, right: 0, top: 220, display: 'flex',
        justifyContent: 'center', gap: 2, opacity: "100" }}>
        {Array.from({ length: 24 }).map((_, i) =>
        <div key={i} style={{ width: 6, height: 6,
          background: i < Math.round(pct * 24) ? dark ? '#fff' : '#111' : dark ? '#222' : '#E6E6E6' }} />
        )}
      </div>
      <div style={{ position: 'absolute', left: 0, right: 0, top: 232, textAlign: 'center',
        fontSize: 8, letterSpacing: '0.22em', opacity: 0.55 }}>
        {"\n"}
      </div>
    </div>);

}

// ── ALARM (opens from Clock button press) ──────────────────────────────────
function AlarmScreen({ alarms, dark, focusIdx }) {
  return (
    <div className={`scr ${dark ? 'scr-dark' : ''}`} style={{ padding: '14px 18px' }}>
      <div className="hdr-line">ALARMS · {alarms.filter((a) => a.on).length} ACTIVE</div>
      <div style={{ marginTop: 6 }}>
        {alarms.map((a, i) =>
        <div key={i} className={`alarm-row ${i === focusIdx ? 'focused' : ''}`}>
            <span className="alarm-mark">{i === focusIdx ? '▶' : ' '}</span>
            <span className="alarm-time">{a.time}</span>
            <span className="alarm-label">{a.label}</span>
            <span className="alarm-toggle">{a.on ? '[ ON ]' : '[OFF]'}</span>
          </div>
        )}
      </div>
      <div className="alarm-foot">BTN_1 PREV · BTN_2 NEXT · BTN_3 TOGGLE</div>
    </div>);

}

// ── POMODORO ───────────────────────────────────────────────────────────────
function PomoScreen({ pomoSec, pomoTotal, mode, running, dark, sessionCount }) {
  const filled = Math.round(pomoSec / pomoTotal * 58);
  const mm = String(Math.floor(pomoSec / 60)).padStart(2, '0');
  const ss = String(pomoSec % 60).padStart(2, '0');
  const subLabel = mode === 'focus' ? 'FOCUS' : mode === 'break' ? 'SHORT BREAK' : 'LONG BREAK';
  return (
    <div className={`scr ${dark ? 'scr-dark' : ''}`}>
      <DotRing filled={filled} dark={dark} />
      <div className="pomo-inner">
        <div className="pomo-time">{mm}:{ss}</div>
        <div className="pomo-sub">{subLabel}</div>
        <div className="pomo-meta">SESSION {sessionCount} · {running ? 'RUNNING' : 'PAUSED'}</div>
      </div>
      <div className="pomo-modes-row">
        <span className={`pmode ${mode === 'focus' ? 'active' : ''} ${dark ? 'dk' : ''}`}>FOCUS</span>
        <span className={`pmode ${mode === 'break' ? 'active' : ''} ${dark ? 'dk' : ''}`}>BREAK</span>
        <span className={`pmode ${mode === 'long' ? 'active' : ''} ${dark ? 'dk' : ''}`}>LONG</span>
      </div>
    </div>);

}

// ── CALENDAR — full-bleed all-day grid ─────────────────────────────────────
function CalendarScreen({ year, month, today, dark, events }) {
  const first = new Date(year, month, 1).getDay();
  const days = new Date(year, month + 1, 0).getDate();
  const prev = new Date(year, month, 0).getDate();
  const cells = [];
  for (let i = 0; i < first; i++) cells.push({ n: prev - first + 1 + i, other: true, key: `p${i}` });
  for (let d = 1; d <= days; d++) cells.push({ n: d, other: false, key: `d${d}`,
    today: d === today.getDate() && month === today.getMonth() && year === today.getFullYear(),
    event: events.find((e) => e.month === month && e.day === d)
  });
  const trail = (7 - (first + days) % 7) % 7;
  for (let i = 1; i <= trail; i++) cells.push({ n: i, other: true, key: `t${i}` });
  const rows = Math.ceil(cells.length / 7);
  // 400×300 full bleed: header 30px, day-strip 14px, grid 256px
  const cellH = Math.floor(256 / rows);
  return (
    <div className={`scr ${dark ? 'scr-dark' : ''}`} style={{ padding: 0 }}>
      <div className="cal-hdr-fb">
        <span className="cal-nav">‹</span>
        <span className="cal-title-fb">{MONTHS[month].toUpperCase()} {year}</span>
        <span className="cal-nav">›</span>
      </div>
      <div className="cal-days-fb">
        {WDAYS3.map((d) => <span key={d}>{d.toUpperCase()}</span>)}
      </div>
      <div className="cal-grid-fb" style={{ gridTemplateRows: `repeat(${rows}, ${cellH}px)` }}>
        {cells.map((c) =>
        <div key={c.key} className={`cal-cell-fb ${c.other ? 'other' : ''} ${c.today ? 'today' : ''} ${c.event ? 'has-event' : ''} ${dark ? 'dk' : ''}`}>
            <span className="cell-num">{c.n}</span>
            {c.event && <span className="cell-dot" />}
          </div>
        )}
      </div>
    </div>);

}

// ── TASKS / OBSIDIAN ──────────────────────────────────────────────────────
function TasksScreen({ tasks, dark }) {
  const visible = tasks.slice(0, 7);
  return (
    <div className={`scr ${dark ? 'scr-dark' : ''}`} style={{ padding: '14px 18px' }}>
      <div className="hdr-line">OBSIDIAN · {tasks.filter((t) => !t.done).length} OPEN</div>
      <div style={{ marginTop: 6 }}>
        {visible.map((t, i) =>
        <div key={i} className={`task-item ${dark ? 'dk' : ''}`}>
            <div className={`t-cb ${t.done ? 'checked' : ''} ${dark ? 'dk' : ''}`} />
            <div className={`t-txt ${t.done ? 'done' : ''}`}>{t.text}</div>
            {t.tag && <span className={`t-tag ${dark ? 'dk' : ''}`}>{t.tag}</span>}
          </div>
        )}
      </div>
    </div>);

}

// ── SETTINGS — combined: WiFi · Update · Theme · System ────────────────────
function SettingsScreen({ wifi, ota, theme, dark, fwVer, focusIdx, apMode }) {
  const rows = [
  { k: 'WIFI', v: apMode ? 'AP MODE · WATCHER-SETUP' : wifi.ssid },
  { k: 'SIGNAL', v: apMode ? '—' : `${wifi.rssi} dBm · ${wifi.bars}` },
  { k: 'IP', v: apMode ? '192.168.4.1' : '192.168.1.42' },
  { k: 'THEME', v: theme.toUpperCase() },
  { k: 'FIRMWARE', v: fwVer },
  { k: 'UPDATE', v: ota.available ? `v${ota.next} · READY` : 'UP TO DATE' }];

  return (
    <div className={`scr ${dark ? 'scr-dark' : ''}`} style={{ padding: '14px 18px' }}>
      <div className="hdr-line">SETTINGS</div>
      <div className="settings-list" style={{ marginTop: 6 }}>
        {rows.map((r, i) =>
        <div key={r.k} className={`set-row ${i === focusIdx ? 'focused' : ''}`}>
            <span className="set-mark">{i === focusIdx ? '▶' : ' '}</span>
            <span className="set-k">{r.k}</span>
            <span className="muted">{r.v}</span>
          </div>
        )}
      </div>
      {ota.installing &&
      <div style={{ marginTop: 8 }}>
          <div className="bar-label">DOWNLOADING · {ota.progress}%</div>
          <div className="bar-row">
            {Array.from({ length: 24 }).map((_, i) =>
          <div key={i} className="bar-sq" style={{ background: i < Math.round(ota.progress / 100 * 24) ? dark ? '#fff' : '#000' : dark ? '#222' : '#E6E6E6' }} />
          )}
          </div>
        </div>
      }
      <div className="alarm-foot">BTN_1/2 NAV · BTN_3 SELECT</div>
    </div>);

}

// ── MAIN EPD CANVAS ────────────────────────────────────────────────────────
function EpdCanvas({ screen, state, dark, refreshing }) {
  return (
    <div className={`epd-canvas ${dark ? 'epd-dark' : ''}`}>
      {refreshing && <div className="epd-refresh-flash" />}
      {screen === 'clock' && <DayClockScreen
        now={state.now} dark={dark} colonOn={state.colonOn}
        timeFmt={state.cfg.timeFormat} dateFmt={state.cfg.dateFormat} />}
      {screen === 'alarm' && <AlarmScreen
        alarms={state.alarms} dark={dark} focusIdx={state.alarmFocus} />}
      {screen === 'pomo' && <PomoScreen
        pomoSec={state.pomoSec} pomoTotal={state.pomoTotal}
        mode={state.pomoMode} running={state.pomoRunning} dark={dark}
        sessionCount={state.pomoSessionCount} />}
      {screen === 'cal' && <CalendarScreen
        year={state.calYear} month={state.calMonth} today={state.now}
        dark={dark} events={state.events} />}
      {screen === 'tasks' && <TasksScreen tasks={state.tasks} dark={dark} />}
      {screen === 'settings' && <SettingsScreen
        wifi={state.wifi} ota={state.ota} theme={dark ? 'dark' : 'light'}
        dark={dark} fwVer={state.fwVer} focusIdx={state.settingsFocus}
        apMode={state.cfg.apMode} />}
    </div>);

}

Object.assign(window, { EpdCanvas });