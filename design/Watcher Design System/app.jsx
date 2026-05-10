/* global React, ReactDOM, EpdCanvas, useTweaks, TweaksPanel, TweakSection, TweakToggle, TweakSlider, WatcherAPI */
// Bootstrap from firmware-injected NVS state (or empty in design preview)
const BOOT = (typeof WatcherAPI !== 'undefined' && WatcherAPI.bootstrap) || {};
const INITIAL_CFG = Object.assign({
  theme: 'light', apMode: false,
  wifiSsid: 'WATCHER-NET-5G', wifiPass: '••••••••••',
  deviceName: 'watcher', timezone: 'Asia/Kolkata', ntpServer: 'pool.ntp.org',
  buzzer: true, timeFormat: '24h', dateFormat: 'long',
  focusMins: 25, breakMins: 5, longMins: 15, sessionsBeforeLong: 4,
}, BOOT.cfg || {});
/* removed-duplicate */
const { useEffect, useMemo, useRef, useState, useCallback } = React;

const TWEAK_DEFAULTS = /*EDITMODE-BEGIN*/{
  "showButtonHints": true,
  "showScreenPaths": true,
  "epdScale": 2.2
}/*EDITMODE-END*/;

const SCREENS = [
  { id: 'clock',    label: 'CLOCK',     group: 'time', path: '/screens/clock.html' },
  { id: 'alarm',    label: 'ALARM',     group: 'time', path: '/screens/alarm.html' },
  { id: 'pomo',     label: 'POMODORO',  group: 'work', path: '/screens/pomodoro.html' },
  { id: 'cal',      label: 'CALENDAR',  group: 'work', path: '/screens/calendar.html' },
  { id: 'tasks',    label: 'TASKS',     group: 'work', path: '/screens/tasks.html' },
  { id: 'settings', label: 'SETTINGS',  group: 'sys',  path: '/screens/settings.html' },
];

const INITIAL_TASKS = BOOT.tasks || [
  { text: 'Review PR #42',            done: true,  tag: 'WORK' },
  { text: 'Weekly review · Obsidian', done: false, tag: 'NOTE' },
  { text: 'Update vault index',       done: false, tag: 'WORK' },
  { text: 'Watcher v7.2 plan',        done: false, tag: 'IDEA' },
  { text: 'ESP32 deep sleep notes',   done: false, tag: 'NOTE' },
];

const INITIAL_ALARMS = BOOT.alarms || [
  { time: '07:00', label: 'WAKE',      on: true  },
  { time: '09:30', label: 'STANDUP',   on: true  },
  { time: '12:30', label: 'LUNCH',     on: false },
  { time: '17:45', label: 'WIND DOWN', on: true  },
];

const INITIAL_EVENTS = BOOT.events || [
  { day: 27, month: 3, type: 'BIRTHDAY', label: "Aarav's birthday" },
  { day: 30, month: 3, type: 'EVENT',    label: 'Sprint review' },
  { day: 5,  month: 4, type: 'EVENT',    label: 'Conference talk' },
  { day: 14, month: 4, type: 'BIRTHDAY', label: "Mum's birthday" },
];

function useNow(running) {
  const [now, setNow] = useState(() => new Date());
  useEffect(() => {
    if (!running) return;
    const id = setInterval(() => setNow(new Date()), 1000);
    return () => clearInterval(id);
  }, [running]);
  return now;
}

function App() {
  // global config (per-screen + system)
  const [cfg, setCfg] = useState(INITIAL_CFG);
  const updateCfg = (k, v) => {
    setCfg(c => ({ ...c, [k]: v }));
    if (typeof WatcherAPI !== 'undefined') WatcherAPI.patchCfg({ [k]: v });
  };
  // ui theme controls the WEB CONSOLE chrome; cfg.theme controls the DEVICE EPD only
  const [uiTheme, setUiTheme] = useState('dark');
  const uiDark = uiTheme === 'dark';
  const deviceDark = cfg.theme === 'dark';
  const dark = uiDark; // shell
  const [t, setTweak] = useTweaks(TWEAK_DEFAULTS);

  const [screen, setScreen] = useState('clock');
  const [conn, setConn] = useState('connected'); // connected | reconnecting | offline
  const [calYear, setCalYear] = useState(2026);
  const [calMonth, setCalMonth] = useState(3);
  const [tasks, setTasks_] = useState(INITIAL_TASKS);
  const [alarms, setAlarms_] = useState(INITIAL_ALARMS);
  const [events, setEvents_] = useState(INITIAL_EVENTS);
  const setTasks  = (u) => setTasks_( prev => { const n = typeof u==='function'?u(prev):u; if (typeof WatcherAPI!=='undefined') WatcherAPI.putTasks(n);  return n; });
  const setAlarms = (u) => setAlarms_(prev => { const n = typeof u==='function'?u(prev):u; if (typeof WatcherAPI!=='undefined') WatcherAPI.putAlarms(n); return n; });
  const setEvents = (u) => setEvents_(prev => { const n = typeof u==='function'?u(prev):u; if (typeof WatcherAPI!=='undefined') WatcherAPI.putEvents(n); return n; });
  const [refreshing, setRefreshing] = useState(false);
  const [alarmFocus, setAlarmFocus] = useState(0);
  const [settingsFocus, setSettingsFocus] = useState(0);

  // pomodoro
  const [pomoMode, setPomoMode] = useState('focus');
  const pomoTotal = (pomoMode==='focus' ? cfg.focusMins : pomoMode==='break' ? cfg.breakMins : cfg.longMins) * 60;
  const [pomoSec, setPomoSec] = useState(pomoTotal);
  const [pomoRunning, setPomoRunning] = useState(false);
  const [pomoSessionCount, setPomoSessionCount] = useState(1);
  const [pomoLog, setPomoLog] = useState([
    { ts: '09:00', mode: 'FOCUS', dur: '25:00', state: 'COMPLETED' },
    { ts: '09:25', mode: 'BREAK', dur: '05:00', state: 'COMPLETED' },
    { ts: '09:30', mode: 'FOCUS', dur: '25:00', state: 'COMPLETED' },
  ]);
  const [alarmLog, setAlarmLog] = useState([
    { ts: '07:00', label: 'WAKE',     state: 'FIRED' },
    { ts: '09:30', label: 'STANDUP',  state: 'FIRED' },
  ]);

  useEffect(() => { setPomoSec(pomoTotal); setPomoRunning(false); }, [pomoTotal]);
  useEffect(() => {
    if (!pomoRunning) return;
    const id = setInterval(() => setPomoSec(s => s > 0 ? s - 1 : 0), 1000);
    return () => clearInterval(id);
  }, [pomoRunning]);

  // OTA simulation
  const [ota, setOta] = useState({ available: true, next: '7.2.0', installing: false, progress: 0 });
  useEffect(() => {
    if (!ota.installing) return;
    const id = setInterval(() => {
      setOta(o => o.progress >= 100 ? { ...o, installing: false, available: false, progress: 100 } : { ...o, progress: o.progress + 4 });
    }, 250);
    return () => clearInterval(id);
  }, [ota.installing]);

  const now = useNow(true);
  const colonOn = now.getSeconds() % 2 === 0;

  useEffect(() => {
    setRefreshing(true);
    const t = setTimeout(() => setRefreshing(false), 380);
    return () => clearTimeout(t);
  }, [screen]);

  // hardware buttons — context-aware per-screen
  const handleBtn = (btn, longPress = false) => {
    if (screen === 'clock') {
      if (btn === 1) setScreen('alarm');                                // BTN_1: open alarms
      else if (btn === 2) setScreen('pomo');                            // BTN_2: next screen
      else if (btn === 3) {/* toggle date format on clock */
        const fmts = ['long','short','iso','numeric'];
        const idx = fmts.indexOf(cfg.dateFormat);
        updateCfg('dateFormat', fmts[(idx+1)%fmts.length]);
      }
    } else if (screen === 'alarm') {
      if (btn === 1)      setAlarmFocus(i => (i-1+alarms.length)%alarms.length);
      else if (btn === 2) setAlarmFocus(i => (i+1)%alarms.length);
      else if (btn === 3) setAlarms(as => as.map((a,i) => i===alarmFocus ? {...a, on: !a.on} : a));
    } else if (screen === 'pomo') {
      if (btn === 3) {
        if (longPress) { setPomoRunning(false); setPomoSec(pomoTotal); }
        else setPomoRunning(r => !r);
      } else if (btn === 1) setScreen('clock');
      else if (btn === 2) setScreen('cal');
    } else if (screen === 'cal') {
      if (btn === 1) setCalMonth(m => m===0 ? (setCalYear(y=>y-1), 11) : m-1);
      else if (btn === 2) setCalMonth(m => m===11 ? (setCalYear(y=>y+1), 0) : m+1);
      else if (btn === 3) setScreen('tasks');
    } else if (screen === 'tasks') {
      if (btn === 1) setScreen('cal');
      else if (btn === 2) setScreen('settings');
    } else if (screen === 'settings') {
      if (btn === 1) setSettingsFocus(i => Math.max(0, i-1));
      else if (btn === 2) setSettingsFocus(i => Math.min(5, i+1));
      else if (btn === 3) {
        if (settingsFocus === 5 && ota.available) setOta(o => ({...o, installing: true, progress: 0}));
        else if (settingsFocus === 3) updateCfg('theme', cfg.theme==='light'?'dark':'light');
        else if (settingsFocus === 0) updateCfg('apMode', !cfg.apMode);
      }
    }
  };

  const epdState = {
    now, colonOn, cfg,
    pomoSec, pomoTotal, pomoMode, pomoRunning, pomoSessionCount,
    calYear, calMonth, events,
    tasks, alarms, alarmFocus,
    ota, fwVer: '7.1.0', settingsFocus,
    wifi: { ssid: cfg.wifiSsid, rssi: -54, bars: '▮▮▮▯' },
  };

  return (
    <div className={`shell ${dark?'shell-dark':''} ${t.showButtonHints?'':'no-hints'} ${t.showScreenPaths?'':'no-paths'}`}>
      <header className="top-bar">
        <div className="brand">
          <span className="brand-mark">▮</span>
          <span className="brand-name">WATCHER</span>
          <span className="brand-sub">v7.1.0 · WEB CONSOLE · {cfg.apMode?'AP MODE':'STA'}</span>
        </div>
        <div className="meta">
          <span className="meta-item"><span className={`dot dot-${conn}`}>●</span> {conn.toUpperCase()}</span>
          <span className="meta-sep">/</span>
          <span className="meta-item">{cfg.deviceName}.local</span>
          <span className="meta-sep">/</span>
          <span className="meta-item">{cfg.apMode ? '192.168.4.1' : '192.168.1.42'}</span>
          <span className="meta-sep">/</span>
          <span className="meta-item">UPTIME 3d 14h</span>
          <span className="meta-sep">/</span>
          <span className="meta-item">{now.toTimeString().slice(0,8)}</span>
        </div>
      </header>

      <main className="grid">
        {/* LEFT: screens + config */}
        <aside className="pane pane-left">
          <SectionHead label="SCREENS" right={`${SCREENS.length} REGISTERED`} />
          <ScreenList screens={SCREENS} active={screen} onSelect={setScreen} />

          <SectionHead label={`CONFIG · ${SCREENS.find(s=>s.id===screen)?.label || ''}`} right="JSON · LIVE" />
          <ConfigPane
            screen={screen} cfg={cfg} updateCfg={updateCfg}
            alarms={alarms} setAlarms={setAlarms}
            events={events} setEvents={setEvents}
            tasks={tasks} setTasks={setTasks}
            ota={ota} setOta={setOta}
            conn={conn} setConn={setConn}
            pomoLog={pomoLog} alarmLog={alarmLog}
          />

        </aside>

        {/* CENTER */}
        <section className="pane pane-center">
          <div className="epd-scaler" style={{ '--epd-scale': t.epdScale }}>
            <EpdCanvas screen={screen} state={epdState} dark={deviceDark} refreshing={refreshing} />
          </div>
        </section>

        {/* RIGHT: telemetry + log */}
        <aside className="pane pane-right">
          <SectionHead label="DEVICE" right="ESP32-S3-WROOM-1 · N8R8" />
          <KvList items={[
            ['UPTIME', '3d 14h 22m'],
            ['HEAP FREE', '218 KB'],
            ['PSRAM', '7.4 / 8.0 MB'],
            ['CPU', '240 MHz · 8%'],
            ['EPD CTRL', 'SSD1683'],
            ['REFRESH', 'PARTIAL 412 ms'],
          ]} />

          <SectionHead label="NETWORK" right={cfg.apMode ? 'AP MODE' : conn.toUpperCase()} />
          <KvList items={[
            ['MODE', cfg.apMode ? 'AP · WATCHER-SETUP' : `STA · ${cfg.wifiSsid}`],
            ['IP', cfg.apMode ? '192.168.4.1' : '192.168.1.42'],
            ['RSSI', cfg.apMode ? '—' : `${epdState.wifi.rssi} dBm`],
            ['MDNS', `${cfg.deviceName}.local`],
            ['NTP', cfg.apMode ? 'OFFLINE' : 'SYNCED'],
            ['WS', conn==='connected' ? 'OPEN /ws' : 'CLOSED'],
          ]} />

          <SectionHead label="SYSTEM" right="REMOTE CTRL" />
          <div className="sys-btns">
            <button className="sys-btn" onClick={()=>{
              setRefreshing(true);
              setTimeout(()=>setRefreshing(false), 380);
            }}>
              <span className="sys-btn-l">REFRESH</span>
              <span className="sys-btn-h">EPD · FULL</span>
            </button>
            <button className={`sys-btn ${cfg.apMode?'on':''}`} onClick={()=>updateCfg('apMode', !cfg.apMode)}>
              <span className="sys-btn-l">OTA MODE</span>
              <span className="sys-btn-h">{cfg.apMode?'ACTIVE · 192.168.4.1':'AP · WATCHER-SETUP'}</span>
            </button>
            <button className="sys-btn" onClick={()=>{
              setConn('reconnecting');
              setTimeout(()=>setConn('offline'), 200);
              setTimeout(()=>setConn('reconnecting'), 1400);
              setTimeout(()=>setConn('connected'), 2800);
            }}>
              <span className="sys-btn-l">REBOOT</span>
              <span className="sys-btn-h">SOFT · ESP_RESTART()</span>
            </button>
          </div>

          <SectionHead label="HARDWARE BUTTONS" right="GPIO 39 · 40 · 41" />
          <div className="btn-sim">
            <BtnRow label="BTN_1" hint="PREV / BACK" short={()=>handleBtn(1)} long={()=>handleBtn(1,true)} />
            <BtnRow label="BTN_2" hint="NEXT" short={()=>handleBtn(2)} long={()=>handleBtn(2,true)} />
            <BtnRow label="BTN_3" hint="SELECT / TOGGLE" short={()=>handleBtn(3)} long={()=>handleBtn(3,true)} longLabel="LONG" />
          </div>

          <SectionHead label="EVENT LOG" right="LIVE" />
          <EventLog screen={screen} pomoRunning={pomoRunning} conn={conn} apMode={cfg.apMode} theme={cfg.theme} />
        </aside>
      </main>

      <footer className="bottom-bar">
        <div className="status-cell">
          <span className="cell-k">SCREEN</span>
          <span className="cell-v">{screen.toUpperCase()}</span>
        </div>
        <div className="status-cell">
          <span className="cell-k">POMO</span>
          <span className="cell-v">{pomoMode.toUpperCase()} · {pomoRunning?'RUN':'IDLE'} · {String(Math.floor(pomoSec/60)).padStart(2,'0')}:{String(pomoSec%60).padStart(2,'0')}</span>
        </div>
        <div className="status-cell">
          <span className="cell-k">REST API</span>
          <span className="cell-v">9 ROUTES · /ws</span>
        </div>
        <div className="status-cell">
          <span className="cell-k">FW</span>
          <span className="cell-v">v7.1.0 · {ota.available ? `→ v${ota.next}` : 'LATEST'}</span>
        </div>
        <div className="status-cell pulse">
          <span className="cell-k">▮▮▮▮</span>
          <span className="cell-v">ALIVE</span>
        </div>
      </footer>

      <button className="ui-theme-toggle" onClick={()=>setUiTheme(t => t==='dark'?'light':'dark')}
        title={`UI THEME · ${uiTheme.toUpperCase()} · click to toggle`}>
        <span className="ui-theme-toggle-icon">{uiDark ? '◐' : '◑'}</span>
        <span className="ui-theme-toggle-l">UI · {uiDark ? 'DARK' : 'LIGHT'}</span>
      </button>

      <TweaksPanel>
        <TweakSection label="Labels" />
        <TweakToggle label="Show button hints" value={t.showButtonHints}
          onChange={(v)=>setTweak('showButtonHints', v)} />
        <TweakToggle label="Show screen paths" value={t.showScreenPaths}
          onChange={(v)=>setTweak('showScreenPaths', v)} />
        <TweakSection label="Canvas" />
        <TweakSlider label="EPD scale" value={t.epdScale} min={1} max={3.5} step={0.1} unit="×"
          onChange={(v)=>setTweak('epdScale', v)} />
      </TweaksPanel>
    </div>
  );
}

// ── small pieces ────────────────────────────────────────────────────────────
function SectionHead({ label, right }) {
  return (
    <div className="sec-head">
      <span className="sec-head-l">{label}</span>
      <span className="sec-head-r">{right}</span>
    </div>
  );
}

function ScreenList({ screens, active, onSelect }) {
  const groups = { time: 'TIME', work: 'WORK', sys: 'SYSTEM' };
  const byGroup = {};
  screens.forEach(s => { (byGroup[s.group] = byGroup[s.group] || []).push(s); });
  return (
    <div className="screen-list">
      {Object.entries(byGroup).map(([g, list]) => (
        <div key={g} className="screen-group">
          <div className="screen-group-l">{groups[g]}</div>
          {list.map(s => (
            <button key={s.id} className={`screen-row ${active===s.id?'active':''}`} onClick={()=>onSelect(s.id)}>
              <span className="screen-row-mark">{active===s.id?'▶':'·'}</span>
              <span className="screen-row-label">{s.label}</span>
              <span className="screen-row-path">{s.path}</span>
            </button>
          ))}
        </div>
      ))}
    </div>
  );
}

function BtnRow({ label, hint, short, long, longLabel = 'LONG' }) {
  return (
    <div className="btn-row">
      <div className="btn-row-meta">
        <span className="btn-row-label">{label}</span>
        <span className="btn-row-hint">{hint}</span>
      </div>
      <button className="hw-btn" onClick={short}>SHORT</button>
      <button className="hw-btn" onClick={long}>{longLabel}</button>
    </div>
  );
}

function KvList({ items }) {
  return (
    <div className="kv">
      {items.map(([k,v]) => (
        <div key={k} className="kv-row">
          <span className="kv-k">{k}</span>
          <span className="kv-dot" />
          <span className="kv-v">{v}</span>
        </div>
      ))}
    </div>
  );
}

function EventLog({ screen, pomoRunning, conn, apMode, theme }) {
  const [lines, setLines] = useState([
    { t: '00:00:00', tag: 'BOOT', msg: 'esp_idf 5.2 · firmware v7.1.0' },
    { t: '00:00:01', tag: 'WIFI', msg: 'connected · 192.168.1.42' },
    { t: '00:00:02', tag: 'NTP',  msg: 'sync ok · drift 12 ms' },
    { t: '00:00:02', tag: 'EPD',  msg: 'init ok · ssd1683 · partial' },
    { t: '00:00:03', tag: 'HTTP', msg: 'listening :80 · /ws ready' },
  ]);
  const lastRef = useRef({ screen, pomoRunning, conn, apMode, theme });
  useEffect(() => {
    const last = lastRef.current;
    const ts = new Date().toTimeString().slice(0,8);
    const next = [];
    if (screen !== last.screen)             next.push({ t: ts, tag: 'NAV',  msg: `screen → ${screen}` });
    if (pomoRunning !== last.pomoRunning)   next.push({ t: ts, tag: 'POMO', msg: pomoRunning ? 'start tick 1Hz' : 'pause' });
    if (conn !== last.conn)                 next.push({ t: ts, tag: 'WS',   msg: conn });
    if (apMode !== last.apMode)             next.push({ t: ts, tag: 'WIFI', msg: apMode ? 'AP broadcast · WATCHER-SETUP' : 'STA reconnect' });
    if (theme !== last.theme)               next.push({ t: ts, tag: 'EPD',  msg: `theme → ${theme}` });
    if (next.length) setLines(l => [...l, ...next].slice(-14));
    lastRef.current = { screen, pomoRunning, conn, apMode, theme };
  }, [screen, pomoRunning, conn, apMode, theme]);
  return (
    <div className="log">
      {lines.map((l,i) => (
        <div key={i} className="log-row">
          <span className="log-t">{l.t}</span>
          <span className={`log-tag log-tag-${l.tag.toLowerCase()}`}>{l.tag}</span>
          <span className="log-msg">{l.msg}</span>
        </div>
      ))}
    </div>
  );
}

// ── CONFIG PANE — per-screen ───────────────────────────────────────────────
function ConfigPane({ screen, cfg, updateCfg, alarms, setAlarms, events, setEvents, tasks, setTasks, ota, setOta, conn, setConn, pomoLog, alarmLog }) {
  if (screen === 'clock') return <ClockConfig cfg={cfg} updateCfg={updateCfg} />;
  if (screen === 'alarm') return <AlarmConfig alarms={alarms} setAlarms={setAlarms} log={alarmLog} />;
  if (screen === 'pomo')  return <PomoConfig cfg={cfg} updateCfg={updateCfg} log={pomoLog} />;
  if (screen === 'cal')   return <CalConfig events={events} setEvents={setEvents} />;
  if (screen === 'tasks') return <TasksConfig tasks={tasks} setTasks={setTasks} />;
  if (screen === 'settings') return <SettingsConfig cfg={cfg} updateCfg={updateCfg} ota={ota} setOta={setOta} conn={conn} setConn={setConn} />;
  return null;
}

function CfgRow({ k, children }) {
  return (
    <div className="cfg-row">
      <span className="cfg-k">{k}</span>
      <div className="cfg-v">{children}</div>
    </div>
  );
}

function CfgSegment({ value, options, onChange }) {
  return (
    <div className="cfg-segment">
      {options.map(o => (
        <button key={o.value} className={`cfg-seg ${value===o.value?'active':''}`} onClick={()=>onChange(o.value)}>{o.label}</button>
      ))}
    </div>
  );
}

function CfgInput({ value, onChange, type='text', placeholder, suffix }) {
  return (
    <div className="cfg-input-wrap">
      <input className="cfg-input" type={type} value={value} placeholder={placeholder}
        onChange={e => onChange(type==='number' ? Number(e.target.value) : e.target.value)} />
      {suffix && <span className="cfg-suffix">{suffix}</span>}
    </div>
  );
}

function ClockConfig({ cfg, updateCfg }) {
  return (
    <div className="cfg">
      <CfgRow k="TIME">
        <CfgSegment value={cfg.timeFormat} onChange={v=>updateCfg('timeFormat', v)}
          options={[{value:'24h',label:'24H'},{value:'12h',label:'12H'}]} />
      </CfgRow>
      <CfgRow k="DATE">
        <CfgSegment value={cfg.dateFormat} onChange={v=>updateCfg('dateFormat', v)}
          options={[{value:'long',label:'LONG'},{value:'short',label:'SHORT'},{value:'iso',label:'ISO'},{value:'numeric',label:'D/M/Y'}]} />
      </CfgRow>
      <CfgRow k="PREVIEW">
        <span className="cfg-preview">
          {cfg.timeFormat==='12h'?'2:34 PM':'14:34'} · {{
            long:'Saturday, 25th Apr. 2026',
            short:'Sat 25 Apr',
            iso:'2026-04-25',
            numeric:'25/04/2026'
          }[cfg.dateFormat]}
        </span>
      </CfgRow>
    </div>
  );
}

function AlarmConfig({ alarms, setAlarms, log }) {
  const update = (i, k, v) => setAlarms(as => as.map((a,j)=>j===i?{...a,[k]:v}:a));
  const add = () => setAlarms(as => [...as, {time:'08:00', label:'NEW', on:true}]);
  const del = (i) => setAlarms(as => as.filter((_,j)=>j!==i));
  return (
    <div className="cfg">
      <div className="cfg-list">
        {alarms.map((a,i)=>(
          <div key={i} className="cfg-item">
            <input className="cfg-input cfg-input-time" type="time" value={a.time} onChange={e=>update(i,'time',e.target.value)} />
            <input className="cfg-input" value={a.label} onChange={e=>update(i,'label',e.target.value.toUpperCase())} />
            <button className={`cfg-toggle ${a.on?'on':''}`} onClick={()=>update(i,'on',!a.on)}>{a.on?'ON':'OFF'}</button>
            <button className="cfg-x" onClick={()=>del(i)}>×</button>
          </div>
        ))}
      </div>
      <button className="cfg-add" onClick={add}>+ ADD ALARM</button>
      <ImportExportRow kind="ALARMS" />
      <div className="cfg-sub">RECENT FIRES</div>
      <div className="cfg-mini-log">
        {log.map((l,i)=>(
          <div key={i} className="mini-log-row">
            <span className="ml-t">{l.ts}</span>
            <span className="ml-l">{l.label}</span>
            <span className="ml-s">{l.state}</span>
          </div>
        ))}
      </div>
    </div>
  );
}

function PomoConfig({ cfg, updateCfg, log }) {
  return (
    <div className="cfg">
      <CfgRow k="FOCUS"><CfgInput type="number" value={cfg.focusMins} onChange={v=>updateCfg('focusMins', v)} suffix="MIN" /></CfgRow>
      <CfgRow k="BREAK"><CfgInput type="number" value={cfg.breakMins} onChange={v=>updateCfg('breakMins', v)} suffix="MIN" /></CfgRow>
      <CfgRow k="LONG"><CfgInput type="number" value={cfg.longMins} onChange={v=>updateCfg('longMins', v)} suffix="MIN" /></CfgRow>
      <CfgRow k="CYCLES"><CfgInput type="number" value={cfg.sessionsBeforeLong} onChange={v=>updateCfg('sessionsBeforeLong', v)} suffix="↻ LONG" /></CfgRow>
      <div className="cfg-sub">SESSION LOG · TODAY</div>
      <div className="cfg-mini-log">
        {log.map((l,i)=>(
          <div key={i} className="mini-log-row">
            <span className="ml-t">{l.ts}</span>
            <span className={`ml-tag ml-tag-${l.mode.toLowerCase()}`}>{l.mode}</span>
            <span className="ml-l">{l.dur}</span>
            <span className="ml-s">{l.state}</span>
          </div>
        ))}
      </div>
    </div>
  );
}

function CalConfig({ events, setEvents }) {
  const MS = ['JAN','FEB','MAR','APR','MAY','JUN','JUL','AUG','SEP','OCT','NOV','DEC'];
  const update = (i,k,v) => setEvents(es => es.map((e,j)=>j===i?{...e,[k]:v}:e));
  const add = () => setEvents(es => [...es, {day:1, month:3, type:'EVENT', label:'NEW EVENT'}]);
  const del = (i) => setEvents(es => es.filter((_,j)=>j!==i));
  return (
    <div className="cfg">
      <div className="cfg-list">
        {events.map((e,i)=>(
          <div key={i} className="cfg-item cfg-item-event">
            <input className="cfg-input cfg-input-day" type="number" min="1" max="31" value={e.day} onChange={ev=>update(i,'day',Number(ev.target.value))} />
            <select className="cfg-input cfg-input-mo" value={e.month} onChange={ev=>update(i,'month',Number(ev.target.value))}>
              {MS.map((m,j)=><option key={j} value={j}>{m}</option>)}
            </select>
            <select className="cfg-input cfg-input-type" value={e.type} onChange={ev=>update(i,'type',ev.target.value)}>
              <option value="EVENT">EVENT</option>
              <option value="BIRTHDAY">BIRTHDAY</option>
              <option value="HOLIDAY">HOLIDAY</option>
            </select>
            <input className="cfg-input" value={e.label} onChange={ev=>update(i,'label',ev.target.value)} />
            <button className="cfg-x" onClick={()=>del(i)}>×</button>
          </div>
        ))}
      </div>
      <button className="cfg-add" onClick={add}>+ ADD EVENT / BIRTHDAY</button>
      <ImportExportRow kind="CALENDAR" />
    </div>
  );
}

function TasksConfig({ tasks, setTasks }) {
  const update = (i,k,v) => setTasks(ts => ts.map((t,j)=>j===i?{...t,[k]:v}:t));
  const del = (i) => setTasks(ts => ts.filter((_,j)=>j!==i));
  return (
    <div className="cfg">
      <div className="cfg-info">SOURCED FROM OBSIDIAN VAULT · /WATCHER/TODAY.MD</div>
      <div className="cfg-list">
        {tasks.map((t,i)=>(
          <div key={i} className="cfg-item">
            <button className={`cfg-toggle ${t.done?'on':''}`} onClick={()=>update(i,'done',!t.done)}>{t.done?'✓':' '}</button>
            <input className="cfg-input" value={t.text} onChange={e=>update(i,'text',e.target.value)} />
            <select className="cfg-input cfg-input-tag" value={t.tag||''} onChange={e=>update(i,'tag',e.target.value)}>
              <option value="WORK">WORK</option>
              <option value="NOTE">NOTE</option>
              <option value="IDEA">IDEA</option>
            </select>
            <button className="cfg-x" onClick={()=>del(i)}>×</button>
          </div>
        ))}
      </div>
      <button className="cfg-add" onClick={()=>setTasks(ts => [...ts, {text:'NEW TASK', done:false, tag:'WORK'}])}>+ ADD TASK</button>
      <ImportExportRow kind="TASKS" />
    </div>
  );
}

function ImportExportRow({ kind }) {
  return (
    <div className="io-row">
      <button className="io-btn">
        <span className="io-btn-icon">↑</span>
        <span className="io-btn-l">IMPORT</span>
        <span className="io-btn-h">{kind}.JSON</span>
      </button>
      <button className="io-btn">
        <span className="io-btn-icon">↓</span>
        <span className="io-btn-l">EXPORT</span>
        <span className="io-btn-h">{kind}.JSON</span>
      </button>
    </div>
  );
}

function SettingsConfig({ cfg, updateCfg, ota, setOta, conn, setConn }) {
  return (
    <div className="cfg">
      <div className="cfg-sub">THEME</div>
      <CfgRow k="DEVICE THEME">
        <CfgSegment value={cfg.theme} onChange={v=>updateCfg('theme', v)}
          options={[{value:'light',label:'LIGHT'},{value:'dark',label:'DARK'}]} />
      </CfgRow>

      <div className="cfg-sub">WIFI</div>
      <CfgRow k="MODE">
        <CfgSegment value={cfg.apMode?'ap':'sta'} onChange={v=>updateCfg('apMode', v==='ap')}
          options={[{value:'sta',label:'STATION'},{value:'ap',label:'AP · SETUP'}]} />
      </CfgRow>
      <CfgRow k="SSID"><CfgInput value={cfg.wifiSsid} onChange={v=>updateCfg('wifiSsid', v)} /></CfgRow>
      <CfgRow k="PASS"><CfgInput type="password" value={cfg.wifiPass} onChange={v=>updateCfg('wifiPass', v)} /></CfgRow>
      <CfgRow k="HOST"><CfgInput value={cfg.deviceName} onChange={v=>updateCfg('deviceName', v)} suffix=".LOCAL" /></CfgRow>
      <button className="cfg-add cfg-warn" onClick={()=>{ updateCfg('apMode', !cfg.apMode); }}>
        {cfg.apMode ? '↻ EXIT AP & RECONNECT WIFI' : '↻ START AP · BROADCAST WATCHER-SETUP'}
      </button>

      <div className="cfg-sub">UPDATE</div>
      <CfgRow k="CURRENT"><span className="cfg-static">v7.1.0</span></CfgRow>
      <CfgRow k="AVAILABLE"><span className="cfg-static">{ota.available ? `v${ota.next}` : 'LATEST'}</span></CfgRow>
      {ota.available && !ota.installing && (
        <button className="cfg-add" onClick={()=>setOta(o=>({...o, installing:true, progress:0}))}>↧ INSTALL v{ota.next}</button>
      )}
      {ota.installing && (
        <div className="cfg-progress">
          <div className="cfg-progress-bar" style={{width: `${ota.progress}%`}} />
          <span className="cfg-progress-label">DOWNLOADING · {ota.progress}%</span>
        </div>
      )}

      <div className="cfg-sub">SYSTEM</div>
      <CfgRow k="BUZZER">
        <CfgSegment value={cfg.buzzer?'on':'off'} onChange={v=>updateCfg('buzzer', v==='on')}
          options={[{value:'on',label:'ON'},{value:'off',label:'OFF'}]} />
      </CfgRow>
      <CfgRow k="TIMEZONE"><CfgInput value={cfg.timezone} onChange={v=>updateCfg('timezone', v)} /></CfgRow>
      <CfgRow k="NTP"><CfgInput value={cfg.ntpServer} onChange={v=>updateCfg('ntpServer', v)} /></CfgRow>
      <CfgRow k="WS LINK">
        <CfgSegment value={conn} onChange={setConn}
          options={[{value:'connected',label:'OK'},{value:'reconnecting',label:'RETRY'},{value:'offline',label:'OFF'}]} />
      </CfgRow>
    </div>
  );
}

ReactDOM.createRoot(document.getElementById('root')).render(<App />);
