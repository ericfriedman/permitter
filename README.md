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
- **Trusted tool memory** — tap Trust once and that tool auto-approves for the rest of the session, with a brief activity flash on screen so you can see what's running
- **Dual approval detection** — some tools (Bash, Fetch, WebSearch) also trigger Claude Code's built-in terminal prompt. The device warns you with "ALSO NEEDS TERMINAL APPROVAL" so you know to check your keyboard too
- **Risk classification** — requests are color-coded by risk level (red/yellow/green)
- **Three themes** — Terminal (CRT green), Skeuo (iOS 6), Brutalist (concrete/hazard). Switchable at runtime, persists across reboots
- **12/24-hour clock** — toggle in settings, persists across reboots
- **Graceful fallback** — if the bridge is down, Claude Code falls back to its normal terminal prompt

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

### 2. Start the bridge server

```bash
cd bridge
node index.js
```

The bridge listens on port 3737 by default. Set `PERMITTER_PORT` env var to change it.

### 3. Hook up your project

From your project directory, run the setup script:

```bash
cd ~/my-project
bash /path/to/permitter/setup.sh
```

This creates `.claude/settings.json` with the hook pointing at `hook.js`. The path is resolved automatically.

Or to enable for **all** projects, put the generated config in `~/.claude/settings.json`.

### 4. Use it

```bash
# Start the bridge in the background
node /path/to/permitter/bridge/index.js &

# Launch Claude Code
claude
```

The first time each tool fires, the M5Stack beeps and shows a permission request. Tap **Trust** to always-allow that tool — future calls will auto-approve with a brief activity flash on screen. Tap **Once** to allow just this call, or **Deny** to block it.

For tools that also require Claude Code's built-in approval (Bash commands, web fetches), the device shows "ALSO NEEDS TERMINAL APPROVAL" so you know to confirm on the keyboard too.

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

## Risk classification

The bridge classifies each request:

| Risk | Color | Examples |
|------|-------|---------|
| **High** | Red | `rm`, `sudo`, `curl \| bash`, `DROP TABLE` |
| **Medium** | Yellow | File writes, `git push`, `Edit`/`Write` tools |
| **Low** | Green | Read operations, searches, `Glob`, `Grep` |

## Dual approval

Some tool calls trigger both the Permitter device **and** Claude Code's built-in terminal prompt:

| Tool | Permitter only | Also needs terminal |
|------|:-:|:-:|
| Read, Grep, Glob | Yes | |
| Write, Edit | Yes | |
| Bash (safe) | Yes | |
| Bash (dangerous) | Yes | Yes |
| WebFetch, WebSearch | Yes | Yes |

When dual approval is needed, the device shows a warning so you know to check the terminal after tapping.

## Project structure

```
permitter/
  bridge/
    index.js          # HTTP bridge server
    hook.js           # Claude Code PreToolUse hook
    classifier.js     # Risk classification logic
  firmware/
    permitter/
      permitter.ino   # Main firmware sketch
      config.example.h
      theme_interface.h
      theme_registry.h
      themes/
        theme_terminal.h
        theme_skeuo.h
        theme_brutalist.h
  setup.sh            # One-command project hookup
  GUIDE.md            # Step-by-step getting started guide
```

## License

MIT
