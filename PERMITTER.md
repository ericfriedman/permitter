# Permitter
### Physical permission panel for Claude Code — M5Stack Core2 V1.1

> A physical device that intercepts Claude Code permission prompts and lets you approve or deny them by tapping a touchscreen. No keyboard. No context switch. Fully open source with a runtime-switchable theme system.

---

## Project Overview

When Claude Code hits a permission prompt it normally pauses and waits for you to type `y` or `n` in the terminal. Permitter intercepts that moment, pushes the request to an M5Stack Core2 over WiFi, and lets you tap one of three zones on the device screen:

- **Class 1 — Trust Always** — equivalent to "yes, and remember this"
- **Class 2 — This Time** — equivalent to "yes, just this once"
- **Class 3 — Deny** — equivalent to "no"

The device vibrates and plays a sound when a request arrives. When idle it runs a clock. The entire visual language is controlled by a theme — three ship with the project, and contributors can add their own.

**Website:** permitter.app *(coming)*
**License:** MIT

---

## Hardware

| Component | Spec |
|---|---|
| Board | M5Stack Core2 V1.1 |
| Chip | ESP32-D0WDQ6-V3, dual-core Xtensa LX6 @ 240MHz |
| Screen | 2.0" IPS capacitive touchscreen, 320×240px |
| Memory | 16MB Flash, 8MB PSRAM, 390mAh LiPo |
| Power | USB-C charge and flash, same port |
| Wireless | WiFi 802.11 b/g/n + Bluetooth |
| Extras | 6-axis IMU, PDM microphone, vibration motor, speaker |
| Interface | Full touchscreen — no physical buttons |

**Buy:** [M5Stack Core2 V1.1 on Amazon](https://amzn.to/4snph8w)

---

## Architecture

```
┌─────────────────────────────────────┐
│  Mac                                │
│                                     │
│  claude (CLI process)               │
│      │ PreToolUse hook              │
│      ▼                              │
│  hook.js → POST /request            │
│      │ blocks until response        │
│      │                              │
│  Permitter Bridge (Node.js :3737)   │
│      │ holds pending request        │
│      │ serves /pending to device    │
│      │ receives /respond from device│
│      │ returns choice to hook       │
└──────┼──────────────────────────────┘
       │ WiFi (local network)
┌──────┼──────────────────────────────┐
│  M5Stack Core2                      │
│      │ polls /pending every 500ms   │
│      │ renders Permission screen    │
│      │ user taps zone               │
│      ▼                              │
│  POST /respond { choice }           │
└─────────────────────────────────────┘
```

**Key principles:**
- Uses Claude Code's hooks system — no stdout parsing, no process wrapping.
- Hook script blocks until device responds, then returns allow/deny to Claude Code.
- All Claude Code config lives on the Mac. The device is display + input only.
- WiFi required for companion mode. Device falls back to clock/idle without it.

---

## Screen States

### 1. Idle
- Clock display (time + date)
- Connection status dot (green = bridge reachable, gray = offline)
- Subtle animation defined by theme
- Long-press anywhere → Theme Picker

### 2. Activity Flash
- Shows when a trusted tool auto-approves
- Brief display of tool name and action in the idle screen area
- No user input needed — confirms that trusted tools are running

### 3. Permission Request
- Triggered by bridge when Claude Code hits a permission prompt
- Shows: tool name, full command string, risk level badge
- Three full-width tap zones across bottom 82px of screen:
  - **Left third** — Class 1 / Trust Always
  - **Center third** — Class 2 / This Time
  - **Right third** — Class 3 / Deny
- Risk level (low / medium / high) passed from bridge, affects badge color
- Dual approval indicator — shows "ALSO NEEDS TERMINAL APPROVAL" for tools that also trigger Claude Code's built-in prompt (Bash, Fetch, WebSearch)
- Device vibrates on arrival, plays theme alert sound

### 4. Accepted / Denied
- Confirmation flash (1.5s) then returns to Idle
- Theme defines visual and sound for this state

### 5. Settings
- Triggered by long-press on Idle screen (1.5s hold)
- Theme selection — tap to highlight, tap again to confirm
- Clock format toggle — 12-hour or 24-hour display
- All settings saved to flash (persist across reboots)
- No reflash required — fully runtime

### 6. Disconnected
- Shown when bridge unreachable for >5s
- Clock still runs
- Reconnect attempts every 5s with visual indicator

---

## Touch Zones

```
┌─────────────────────────────────────┐ ← 320px wide
│                                     │
│         [content area]              │ ← 158px tall
│                                     │
├───────────────┬───────────┬─────────┤ ← y=158
│               │           │         │
│  CLASS 1      │  CLASS 2  │ CLASS 3 │ ← 82px tall
│  TRUST        │  THIS     │         │
│  ALWAYS       │  TIME     │  DENY   │
│               │           │         │
└───────────────┴───────────┴─────────┘
  x: 0–106        107–213     214–319
```

Touch detection uses raw `M5.Touch.getPressPoint()` — no virtual button abstraction. Large zones are intentional: tappable without looking.

---

## Risk Classification

Computed by the bridge from tool name + command string. Rules applied in order:

**High risk** — any of:
- Tool is `Bash` AND command contains: `rm`, `--force`, `sudo`, `chmod`, `chown`, `curl | bash`, `wget`, `dd`, `mkfs`, `DROP`, `DELETE`, `truncate`
- Tool is `Bash` AND pipes to `sh` or `bash`

**Medium risk** — any of:
- Tool is `Bash` AND command writes files (`>`, `>>`, `tee`, `mv`, `cp`)
- Tool is `Write` (any file write)
- Tool is `Bash` AND command contains `git push`, `git commit`, `npm publish`, `deploy`

**Low risk** — everything else:
- Tool is `Read`
- Tool is `Bash` AND command is read-only (`ls`, `cat`, `grep`, `find`, `echo`)

Risk level is included in the `/pending` response payload. Themes use it to set colors and sounds.

---

## Bridge Server

**Stack:** Node.js, no framework
**File:** `bridge/index.js`
**Port:** 3737 (configurable via `PERMITTER_PORT` env var)

### Starting the bridge

```bash
# Instead of running claude directly:
node bridge/index.js

# Or wrap it:
permitter                     # if installed globally via npm link
permitter -- --model claude-sonnet-4-5   # pass args to claude
```

### HTTP Endpoints

| Method | Path | Description |
|---|---|---|
| `GET` | `/status` | Heartbeat. Returns `{ connected, version, trusted[] }` |
| `GET` | `/pending` | Current permission request, activity flash, or `null` |
| `POST` | `/request` | Hook sends permission request, blocks until device responds |
| `POST` | `/respond` | Device sends choice: `{ choice: "always" \| "allow" \| "deny" }` |

### `/pending` Response Shape

```json
// Permission request (needs device approval)
{
  "type": "permission",
  "id": "1718123456789",
  "tool": "Bash",
  "action": "git push origin main --force",
  "risk": "high",
  "dual": "1",
  "timestamp": "2025-03-16T14:22:01Z"
}

// Activity flash (trusted tool, auto-approved)
{
  "type": "activity",
  "tool": "Read",
  "action": "/path/to/file",
  "risk": "low",
  "timestamp": "2025-03-16T14:22:02Z"
}

// Nothing pending
null
```

### Hook Integration

Uses Claude Code's `PreToolUse` hook system — no stdout parsing or process wrapping.

The hook (`bridge/hook.js`) receives tool info on stdin as JSON, POSTs to the bridge's `/request` endpoint, and blocks until the device responds. Returns `{"decision": "allow"}` or `{"decision": "deny"}` to Claude Code.

### Trusted Tool Memory

When the user taps "Trust" on the device, the bridge adds that tool to an in-memory trusted set. Future calls to trusted tools are auto-approved instantly — the hook returns immediately and the device shows a brief activity flash.

The trusted set resets when the bridge restarts (per-session).

---

## Firmware

**Language:** Arduino / C++
**Main file:** `firmware/permitter/permitter.ino`
**Libraries required:**
- `M5Core2` (M5Stack official)
- `M5Unified` (unified HAL — preferred)
- `ArduinoJson` (v6+)
- `WiFi` (built-in ESP32)
- `HTTPClient` (built-in ESP32)

### Config

```cpp
// firmware/permitter/config.h  ← gitignored
// Copy from config.example.h

const char* WIFI_SSID    = "your_network";
const char* WIFI_PASS    = "your_password";
const char* BRIDGE_HOST  = "192.168.1.x";  // Mac local IP
const int   BRIDGE_PORT  = 3737;
const char* DEFAULT_THEME = "terminal";     // fallback if flash empty
```

### Main Loop

```
setup():
  init M5
  connect WiFi
  load saved theme from flash (Preferences library)
  init theme (call theme.setup())
  set state = IDLE

loop():
  M5.update()
  handle long-press → theme picker
  
  switch (state):
    IDLE:
      theme.drawIdle()
      poll /pending every 500ms
      if pending → vibrate, play alert sound, state = PERMISSION

    THINKING:
      theme.drawThinking()
      poll /pending every 500ms
      if pending → vibrate, play alert sound, state = PERMISSION

    PERMISSION:
      theme.drawPermission(pending)
      if touch:
        zone = getTouchZone()    // 1, 2, or deny
        POST /respond { choice: zone }
        state = CONFIRM

    CONFIRM:
      theme.drawConfirm(lastChoice)
      play confirm sound
      vibrate short
      wait 1500ms
      state = IDLE

    THEME_PICKER:
      drawThemePicker()
      handle scroll + select
      on confirm: save to flash, reinit theme

    DISCONNECTED:
      theme.drawDisconnected()
      retry /status every 5s
      if connected → state = IDLE
```

### Touch Zone Helper

```cpp
enum Zone { ZONE_ALWAYS = 1, ZONE_ONCE = 2, ZONE_DENY = 3, ZONE_NONE = 0 };

Zone getTouchZone() {
  if (!M5.Touch.ispressed()) return ZONE_NONE;
  TouchPoint p = M5.Touch.getPressPoint();
  if (p.y < 158) return ZONE_NONE;  // above button area
  if (p.x < 107) return ZONE_ALWAYS;
  if (p.x < 214) return ZONE_ONCE;
  return ZONE_DENY;
}
```

---

## Theme System

Themes are the heart of the OSS contribution model. Every theme is a single `.h` file that implements the `PermitterTheme` interface.

### The Interface

```cpp
// firmware/permitter/theme_interface.h

struct PermissionRequest {
  String tool;
  String action;
  String risk;   // "low" | "medium" | "high"
  String id;
  bool dual;     // also needs terminal approval
};

class PermitterTheme {
public:
  virtual const char* name() = 0;         // "terminal"
  virtual const char* displayName() = 0;  // "Terminal"

  // Called once when theme is activated
  virtual void setup() = 0;

  // Screen renderers
  virtual void drawBoot(const char* ssid) = 0;
  virtual void drawBootStatus(bool wifiOk, const char* ip) = 0;
  virtual void drawIdle(bool bridgeConnected) = 0;
  virtual void drawClock(const char* timeStr, const char* dateStr) = 0;
  virtual void drawPermission(PermissionRequest req) = 0;
  virtual void drawConfirm(bool accepted, const char* tool) = 0;
  virtual void drawActivity(const char* tool, const char* action) = 0;

  // Audio
  virtual void playAlertSound() = 0;
  virtual void playConfirmSound(bool accepted) = 0;
};
```

### Bundled Themes

| ID | Name | Aesthetic |
|---|---|---|
| `terminal` | Terminal | CRT phosphor green, scanlines, ASCII art, blinking cursor |
| `skeuo` | Skeuo | iOS 6 leather/felt, beveled gel buttons, brushed aluminum header |
| `brutalist` | Brutalist | Concrete texture, Bauhaus heavy type, hazard stripes, no border-radius |

### Theme Registry

```cpp
// firmware/permitter/theme_registry.h

#include "themes/theme_terminal.h"
#include "themes/theme_skeuo.h"
#include "themes/theme_brutalist.h"

// Add new themes here ↓
// #include "themes/theme_yourtheme.h"

PermitterTheme* getTheme(const char* name) {
  if (strcmp(name, "terminal") == 0)  return new TerminalTheme();
  if (strcmp(name, "skeuo") == 0)     return new SkeuoTheme();
  if (strcmp(name, "brutalist") == 0) return new BrutalistTheme();
  // Add new themes here ↓
  return new TerminalTheme(); // fallback
}

const char* THEME_LIST[] = { "terminal", "skeuo", "brutalist" };
const int THEME_COUNT = 3;
```

### Building Your Own Theme

1. Copy `firmware/permitter/themes/theme_terminal.h` to `theme_yourname.h`
2. Rename the class to `YourNameTheme`
3. Implement all methods in the interface
4. Add your include and entry to `theme_registry.h`
5. Add your theme ID to `THEME_LIST[]`
6. Flash and test
7. Open a PR — include a screenshot or video of each screen state

See `themes/README.md` for detailed guidance, M5Stack drawing API reference, and audio/vibration examples.

---

## File Structure

```
permitter/
├── README.md
├── LICENSE                        ← MIT
├── CONTRIBUTING.md
│
├── bridge/                        ← Node.js bridge server
│   ├── package.json
│   ├── index.js                   ← main entry, spawns claude, HTTP server
│   ├── detector.js                ← stdout pattern matcher
│   ├── classifier.js              ← risk classification logic
│   └── README.md
│
├── firmware/
│   └── permitter/
│       ├── permitter.ino          ← main sketch
│       ├── theme_interface.h      ← PermitterTheme base class
│       ├── theme_registry.h       ← theme list + factory
│       ├── config.example.h       ← copy to config.h and fill in
│       ├── config.h               ← gitignored
│       │
│       └── themes/
│           ├── README.md          ← how to build a theme
│           ├── theme_terminal.h
│           ├── theme_skeuo.h
│           └── theme_brutalist.h
│
├── designer/                      ← React UI design tool
│   ├── src/
│   │   ├── themes/
│   │   │   ├── terminal.jsx
│   │   │   ├── skeuo.jsx
│   │   │   └── brutalist.jsx
│   │   └── App.jsx
│   └── package.json
│
└── docs/
    ├── setup.md                   ← getting started guide
    ├── themes.md                  ← theme contributor guide
    └── bridge.md                  ← bridge server deep-dive
```

---

## Getting Started (End User)

### 1. Flash the firmware

```bash
# Install Arduino IDE + M5Stack board package
# Add board manager URL:
# https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json

# Install libraries via Arduino Library Manager:
# - M5Unified
# - ArduinoJson

# Copy config
cp firmware/permitter/config.example.h firmware/permitter/config.h
# Edit config.h — add your WiFi credentials and Mac's local IP

# Open firmware/permitter/permitter.ino in Arduino IDE
# Select board: M5Stack Core2
# Upload
```

### 2. Run the bridge

```bash
cd bridge
npm install
npm start                  # starts bridge, then starts claude
# or
node index.js              # same thing
```

### 3. Use it

- Permitter bridge starts Claude Code automatically
- When Claude Code hits a permission prompt, your Core2 vibrates and shows the request
- Tap left (Trust Always), center (This Time), or right (Deny)
- Long-press the idle screen to switch themes

---

## Roadmap

- [x] UI design — 3 themes designed in React simulator
- [x] Architecture defined
- [x] Theme interface designed
- [x] Risk classification logic defined
- [x] ~~Capture live Claude Code stdout~~ — replaced with hooks-based approach
- [x] Bridge server implementation — Node.js HTTP server on port 3737
- [x] Firmware implementation — core loop (WiFi, touch, bridge polling, permission flow)
- [x] Claude Code PreToolUse hook integration — end-to-end working
- [x] End-to-end test on physical device — M5Stack Core2 V1.1 confirmed working
- [x] Theme implementations — Terminal, Skeuo, Brutalist (all three complete)
- [x] Runtime theme picker / settings screen
- [x] 12/24-hour clock toggle
- [x] Trusted tool memory — tap Trust once, tool auto-approves with activity flash
- [x] Dual approval detection — warns when terminal approval also needed
- [x] README + setup docs + GUIDE.md
- [x] setup.sh — one-command project hookup
- [ ] npm package for bridge (`npx permitter`)
- [ ] permitter.app microsite
- [ ] v1.0 release

---

## Open Questions

- [x] ~~**Claude Code stdout format**~~ — solved via PreToolUse hooks. No stdout parsing needed.
- [ ] **WebSocket vs polling** — currently HTTP polling every 500ms. Ship polling for v1, migrate if latency is noticeable.
- [ ] **Multi-prompt queue** — if Claude Code fires two permission prompts in quick succession, bridge should queue them and device works through them in order.
- [ ] **Sleep / brightness** — screen dims after 60s idle, off after 5min. Wake on new permission or touch.
- [ ] **Standalone apps** — Tamagotchi, reaction timer, Simon Says for when bridge is offline. Post-v1 milestone.

---

## Contributing

PRs welcome, especially:
- New themes (most wanted)
- Bridge improvements
- Arduino library alternatives
- Standalone app modes

See `CONTRIBUTING.md` for code style, PR process, and the theme contributor checklist.

---

## Design Assets

React UI simulator lives in `designer/`. Run it to preview all themes across all screen states at actual 320×240 resolution before flashing.

```bash
cd designer
npm install
npm run dev
```

Theme JSX files in `designer/src/themes/` mirror the firmware themes and are the recommended starting point for designing a new theme visually before implementing in C++.
