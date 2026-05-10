/* global React, ReactDOM */
const { useState } = React;
const {
  AnalogClockScreen, DigitalClockScreen,
  AlarmScreen, PomodoroScreen,
  CalendarScreen, TasksScreen, SettingsScreen,
  useNow,
  DesignCanvas, DCSection, DCArtboard,
  TweaksPanel, useTweaks, TweakSection, TweakSlider, TweakRadio, TweakToggle,
} = window;

// Defaults persisted by the host. Keep this block valid JSON between markers.
const TWEAK_DEFAULTS = /*EDITMODE-BEGIN*/{
  "digitH": 116,
  "dateShift": 30,
  "ringMode": "seconds",
  "pomoDigitH": 150,
  "pomoMinutes": 24,
  "pomoMode": "focus",
  "pomoSession": 1,
  "pomoTotal": 3,
  "pomoRunning": true,
  "pomoTitleSize": 11
}/*EDITMODE-END*/;

function App() {
  const now = useNow();
  const [tweaks, setTweak] = useTweaks(TWEAK_DEFAULTS);

  return (
    <>
      <DesignCanvas
        title="Watcher · Original Web Console Screens"
        subtitle="400×300 e-paper · 4.2″ B/W · one prototype per screen"
        docId="watcher-original-screens-v2"
      >
        <DCSection id="clock" title="Clock">
          <DCArtboard id="clock-digital" label="Clock · Digital + ring" width={400} height={300}>
            <DigitalClockScreen
              now={now}
              digitH={tweaks.digitH}
              dateShift={tweaks.dateShift}
              ringMode={tweaks.ringMode}
            />
          </DCArtboard>
        </DCSection>

        <DCSection id="alarm" title="Alarm">
          <DCArtboard id="alarm-list" label="Alarm · List" width={400} height={300}>
            <AlarmScreen />
          </DCArtboard>
        </DCSection>

        <DCSection id="pomo" title="Pomodoro">
          <DCArtboard id="pomo-running" label="Pomodoro · Focus running" width={400} height={300}>
            <PomodoroScreen
              now={now}
              digitH={tweaks.pomoDigitH}
              minutesLeft={tweaks.pomoMinutes}
              mode={tweaks.pomoMode}
              session={tweaks.pomoSession}
              totalSessions={tweaks.pomoTotal}
              running={tweaks.pomoRunning}
              titleSize={tweaks.pomoTitleSize}
            />
          </DCArtboard>
        </DCSection>

        <DCSection id="cal" title="Calendar">
          <DCArtboard id="cal-month" label="Calendar · Month view" width={400} height={300}>
            <CalendarScreen />
          </DCArtboard>
        </DCSection>

        <DCSection id="tasks" title="Tasks">
          <DCArtboard id="tasks-list" label="Tasks · Obsidian list" width={400} height={300}>
            <TasksScreen />
          </DCArtboard>
        </DCSection>

        <DCSection id="settings" title="Settings">
          <DCArtboard id="settings-main" label="Settings · Main" width={400} height={300}>
            <SettingsScreen />
          </DCArtboard>
        </DCSection>
      </DesignCanvas>

      <TweaksPanel title="Tweaks">
        <TweakSection label="Clock — layout">
          <TweakSlider label="Digit height" min={90} max={140} step={2}
            value={tweaks.digitH} onChange={v => setTweak('digitH', v)} unit="px" />
          <TweakSlider label="Date offset" min={6} max={56} step={2}
            value={tweaks.dateShift} onChange={v => setTweak('dateShift', v)} unit="px" />
        </TweakSection>
        <TweakSection label="Clock — perimeter ring">
          <TweakRadio label="Mode"
            value={tweaks.ringMode}
            options={[
              { value: 'seconds', label: 'Seconds' },
              { value: 'day',     label: '% of day' },
            ]}
            onChange={v => setTweak('ringMode', v)} />
        </TweakSection>
        <TweakSection label="Pomodoro">
          <TweakSlider label="Title size" min={8} max={20} step={1}
            value={tweaks.pomoTitleSize} onChange={v => setTweak('pomoTitleSize', v)} unit="px" />
          <TweakSlider label="Digit height" min={110} max={180} step={2}
            value={tweaks.pomoDigitH} onChange={v => setTweak('pomoDigitH', v)} unit="px" />
          <TweakSlider label="Minutes left" min={0} max={99} step={1}
            value={tweaks.pomoMinutes} onChange={v => setTweak('pomoMinutes', v)} unit="m" />
          <TweakRadio label="Mode"
            value={tweaks.pomoMode}
            options={[
              { value: 'focus', label: 'Focus' },
              { value: 'break', label: 'Break' },
              { value: 'long',  label: 'Long'  },
            ]}
            onChange={v => setTweak('pomoMode', v)} />
          <TweakSlider label="Session #" min={1} max={tweaks.pomoTotal} step={1}
            value={tweaks.pomoSession} onChange={v => setTweak('pomoSession', v)} />
          <TweakSlider label="Total sessions" min={1} max={12} step={1}
            value={tweaks.pomoTotal} onChange={v => setTweak('pomoTotal', v)} />
          <TweakToggle label="Running"
            value={tweaks.pomoRunning}
            onChange={v => setTweak('pomoRunning', v)} />
        </TweakSection>
      </TweaksPanel>
    </>
  );
}

ReactDOM.createRoot(document.getElementById('root')).render(<App />);
