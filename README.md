# Permitter

A physical permission panel for [Claude Code](https://claude.ai/claude-code). Intercepts tool-use permission prompts over WiFi and lets you approve or deny them by tapping a touchscreen.

Built on the [M5Stack Core2](https://amzn.to/4snph8w) (ESP32, 320x240 IPS capacitive touch).

## How it works

> **Claude Code** &rarr; `PreToolUse` hook &rarr; **Bridge Server** &rarr; HTTP &rarr; **M5Stack Core2**
>
> **M5Stack Core2** &rarr; tap Trust / Once / Deny &rarr; **Bridge Server** &rarr; **Claude Code**

1. Claude Code fires a `PreToolUse` hook before each tool call
2. `hook.js` sends the request to the bridge server on your local network
3. The bridge holds the request until the M5Stack device polls it over WiFi
4. You tap **Trust** (always allow), **Once** (allow this time), or **Deny**
5. The response flows back through the chain and Claude Code continues

If the bridge is unreachable, the hook falls back to Claude Code's normal terminal prompt — no disruption.

## Features

- **Three response options** — Trust (always allow this tool), Once (allow this call), Deny (block it)
- **Trusted tool memory** — tap Trust once and that tool auto-approves for the rest of the session, with a brief activity flash on screen
- **Solo mode** — M5Stack becomes the sole approver. No keyboard prompts. Toggle between solo and dual mode with one command
- **Live status display** — device shows Claude's current state (Working/Idle/Waiting), session uptime, and active agent count
- **Risk classification** — requests are color-coded by risk level (red/yellow/green)
- **Three themes** — Terminal (CRT green), Skeuo (iOS 6), Brutalist (concrete/hazard). Switchable at runtime, persists across reboots
- **12/24-hour clock** — toggle in settings, persists across reboots
- **7 hooks** — PreToolUse, PermissionRequest, PostToolUse, SubagentStart/Stop, Stop, Notification
- **Graceful fallback** — if the bridge is down, Claude Code falls back to its normal terminal prompt

## Quick start

```bash
git clone https://github.com/ericfriedman/permitter.git
export PATH="$PATH:$(pwd)/permitter"

# Flash the M5Stack (see Setup below), then:
cd your-project
permitter setup
permitter start
claude
```

## The `permitter` command

Run from inside any project directory:

| Command | What it does |
|---------|-------------|
| `permitter setup` | Install hooks + solo mode (M5Stack is sole approver) |
| `permitter solo` | Switch to solo mode (no keyboard prompts) |
| `permitter dual` | Switch to dual mode (device + keyboard prompts) |
| `permitter start` | Start the bridge server |
| `permitter stop` | Stop the bridge server |
| `permitter status` | Show bridge connection status |
| `permitter off` | Remove all hooks and permissions |

### Solo vs Dual mode

**Solo mode** (default) — the M5Stack is the only approver. Bash, WebFetch, and WebSearch are pre-allowed in Claude Code's permission system, so its built-in keyboard prompt is skipped. The PreToolUse hook still fires first, so the device remains the gatekeeper.

**Dual mode** — both the M5Stack and Claude Code's keyboard prompt are active. Some tools (Bash, WebFetch, WebSearch) will require approval on both the device and the terminal.

## Setup

### Prerequisites

- [M5Stack Core2](https://amzn.to/4snph8w) (v1.1 tested)
- [arduino-cli](https://arduino.github.io/arduino-cli/)
- Node.js 18+
- A 2.4GHz WiFi network (ESP32 doesn't support 5GHz)

### 1. Flash the firmware

```bash
# Install ESP32 board support
arduino-cli core install esp32:esp32 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json

# Install M5Unified library
arduino-cli lib install M5Unified

# Configure WiFi and bridge IP
cp firmware/permitter/config.example.h firmware/permitter/config.h
# Edit config.h with your WiFi credentials and your computer's local IP

# Compile and flash (replace port with your device's serial port)
arduino-cli compile --fqbn esp32:esp32:m5stack_core2 firmware/permitter
arduino-cli upload --fqbn esp32:esp32:m5stack_core2 --port /dev/cu.usbserial-XXXXX firmware/permitter
```

Find your serial port with `ls /dev/cu.usb*` (macOS) or `ls /dev/ttyUSB*` (Linux).

Find your local IP with `ipconfig getifaddr en0` (macOS) or `hostname -I` (Linux).

### 2. Hook up your project

```bash
cd ~/my-project
permitter setup
```

This creates `.claude/settings.json` with all 7 hooks and `.claude/settings.local.json` with solo mode permissions. Paths are resolved automatically.

### 3. Use it

```bash
permitter start
claude
```

The first time each tool fires, the M5Stack beeps and shows a permission request. Tap **Trust** to always-allow that tool — future calls auto-approve with an activity flash. Tap **Once** to allow just this call, or **Deny** to block it.

## Themes

Three built-in themes, switchable at runtime (long-press the idle screen):

- **Terminal** — CRT phosphor green, scanlines
- **Skeuo** — iOS 6 linen, chrome bars, gel buttons
- **Brutalist** — Concrete, hazard stripes, heavy borders

Your theme choice persists across reboots.

## Settings

Long-press the idle screen to open settings:

- **Theme selection** — tap to highlight, tap again to apply
- **Clock format** — toggle between 12-hour and 24-hour display

All settings persist across reboots.

## Live status

The device shows Claude's current state in real-time:

| State | Meaning |
|-------|---------|
| **WORKING** | Claude is actively using tools |
| **WAITING** | Claude is waiting for your input |
| **IDLE** | No active session or between tasks |

Also displays session uptime and active agent count when subagents are running.

## Risk classification

The bridge classifies each request:

| Risk | Color | Examples |
|------|-------|---------|
| **High** | Red | `rm`, `sudo`, `curl \| bash`, `DROP TABLE` |
| **Medium** | Yellow | File writes, `git push`, `Edit`/`Write` tools |
| **Low** | Green | Read operations, searches, `Glob`, `Grep` |

## Project structure

```
permitter/
  permitter            # CLI — setup, start, stop, solo/dual toggle
  bridge/
    index.js           # HTTP bridge server (v0.4.0)
    hook.js            # PreToolUse hook — device approval
    hook-permission.js # PermissionRequest hook — auto-approve trusted tools
    hook-status.js     # Status hooks — PostToolUse, SubagentStart/Stop, Stop, Notification
    classifier.js      # Risk classification logic
  firmware/
    permitter/
      permitter.ino    # Main firmware sketch
      config.example.h
      theme_interface.h
      theme_registry.h
      themes/
        theme_terminal.h
        theme_skeuo.h
        theme_brutalist.h
  GUIDE.md             # Step-by-step getting started guide
```

## License

MIT
