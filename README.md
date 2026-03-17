# Permitter

A physical permission panel for [Claude Code](https://claude.ai/claude-code). Intercepts tool-use permission prompts over WiFi and lets you approve or deny them by tapping a touchscreen.

Built on the **M5Stack Core2** (ESP32, 320x240 IPS capacitive touch).

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

### 3. Add the hook to your project

Create or edit `.claude/settings.json` in your project:

```json
{
  "hooks": {
    "PreToolUse": [
      {
        "matcher": "",
        "hooks": [
          {
            "type": "command",
            "command": "node /absolute/path/to/permitter/bridge/hook.js"
          }
        ]
      }
    ]
  }
}
```

Replace the path with the absolute path to `hook.js` on your machine.

To enable for **all** projects, put the same config in `~/.claude/settings.json`.

### 4. Quickstart walkthrough

This walks you through your first Permitter-approved Claude Code session, start to finish.

**Terminal 1 — start the bridge:**

```bash
cd permitter/bridge
node index.js
# => Permitter bridge listening on http://0.0.0.0:3737
```

Leave this running.

**Terminal 2 — set up your project:**

```bash
cd ~/my-project

# Create the hook config
mkdir -p .claude
cat > .claude/settings.json << 'EOF'
{
  "hooks": {
    "PreToolUse": [
      {
        "matcher": "",
        "hooks": [
          {
            "type": "command",
            "command": "node /absolute/path/to/permitter/bridge/hook.js"
          }
        ]
      }
    ]
  }
}
EOF
```

Replace `/absolute/path/to/permitter` with the actual path to your permitter clone.

**Launch Claude Code:**

```bash
claude
```

**Make sure the M5Stack is powered on** — it should show the idle screen with a clock and "READY" in the header (or "WAITING" if the bridge isn't reachable yet).

**Ask Claude to do something:**

```
> read my package.json and tell me what dependencies I have
```

**What happens next:**

1. Claude decides to use the `Read` tool
2. Your M5Stack **beeps** and the screen switches to a permission request showing the tool name, action, and risk level
3. Tap one of the three buttons:
   - **Trust** (left) — allow this tool forever, won't ask again
   - **Once** (center) — allow just this one call
   - **Deny** (right) — block the tool call
4. The screen flashes "APPROVED" or "DENIED" for a moment, then returns to idle
5. Claude Code continues immediately with your decision

That's it — every tool call Claude makes now routes through your physical button.

## Themes

Three built-in themes, switchable at runtime (long-press the idle screen):

- **Terminal** — CRT phosphor green, scanlines
- **Skeuo** — iOS 6 linen, chrome bars, gel buttons
- **Brutalist** — Concrete, hazard stripes, heavy borders

Your theme choice persists across reboots.

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
```

## License

MIT
