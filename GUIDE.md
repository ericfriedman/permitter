# Permitter — Getting Started Guide

This guide walks you through setting up Permitter from scratch: flashing the M5Stack, starting the bridge, hooking up Claude Code, and approving your first tool call.

## What you need

- [M5Stack Core2](https://amzn.to/4snph8w) — the physical device
- A Mac (or Linux) with Node.js 18+ installed
- [arduino-cli](https://arduino.github.io/arduino-cli/) — for flashing firmware
- A 2.4GHz WiFi network (ESP32 doesn't support 5GHz)
- USB-C cable (for flashing only — the device runs on WiFi + battery after that)

## Step 1: Clone the repo

```bash
git clone https://github.com/ericfriedman/permitter.git
cd permitter
```

Add the `permitter` command to your PATH:

```bash
export PATH="$PATH:$(pwd)"
```

(Add this to your `.zshrc` or `.bashrc` to make it permanent.)

## Step 2: Flash the firmware onto the M5Stack

Plug the M5Stack into your computer with a USB-C cable.

```bash
# Install ESP32 board support (one-time)
arduino-cli core install esp32:esp32 \
  --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json

# Install M5Unified library (one-time)
arduino-cli lib install M5Unified
```

Now configure your WiFi and bridge connection:

```bash
cp firmware/permitter/config.example.h firmware/permitter/config.h
```

Edit `firmware/permitter/config.h` with your details:

```cpp
const char* WIFI_SSID    = "Your WiFi Name";     // must be 2.4GHz
const char* WIFI_PASS    = "your_password";
const char* BRIDGE_HOST  = "192.168.1.100";       // your computer's local IP
const int   BRIDGE_PORT  = 3737;
const char* DEFAULT_THEME = "terminal";
```

To find your local IP:

```bash
# macOS
ipconfig getifaddr en0

# Linux
hostname -I
```

Find your M5Stack's serial port:

```bash
# macOS
ls /dev/cu.usb*

# Linux
ls /dev/ttyUSB*
```

Compile and flash:

```bash
arduino-cli compile --fqbn esp32:esp32:m5stack_core2 firmware/permitter
arduino-cli upload --fqbn esp32:esp32:m5stack_core2 --port /dev/cu.usbserial-XXXXX firmware/permitter
```

Replace `/dev/cu.usbserial-XXXXX` with your actual serial port.

The device will reboot and show the boot screen. Once it connects to WiFi, you'll see the idle screen with a clock.

**You can now unplug the USB cable.** The M5Stack has a built-in battery and communicates over WiFi from here on.

## Step 3: Hook up a project

Go to whatever project you want to use Permitter with:

```bash
cd ~/my-project
permitter setup
```

This installs 7 hooks in `.claude/settings.json` and enables solo mode (M5Stack as sole approver) via `.claude/settings.local.json`. All paths are resolved automatically.

You only need to run this once per project.

## Step 4: Start a session

From your project directory:

```bash
permitter start
claude
```

Two commands, one terminal. The bridge runs in the background.

## Step 5: Your first approval

In Claude Code, ask it to do something:

```
> read package.json
```

Here's what happens:

1. Claude decides to use the `Read` tool
2. The hook intercepts the call and sends it to the bridge
3. The M5Stack **beeps** and shows the permission request — you'll see the tool name, what it's trying to do, and a risk level color (green/yellow/red)
4. Tap one of the three buttons:
   - **Left (Trust)** — allow this tool forever for this session. Future calls auto-approve with an activity flash on screen.
   - **Center (Once)** — allow just this one call
   - **Right (Deny)** — block the call
5. The screen flashes your decision and returns to idle
6. Claude Code continues immediately

## Step 6: Solo vs Dual mode

**Solo mode** (default after `permitter setup`) — the M5Stack is the only approver. No keyboard prompts. This works by pre-allowing Bash, WebFetch, and WebSearch in Claude Code's permission system while keeping the PreToolUse hook as the gatekeeper.

**Dual mode** — both the device and Claude Code's keyboard prompt are active. Some tools will require approval in both places.

Toggle between modes:

```bash
permitter solo    # M5Stack only
permitter dual    # device + keyboard
```

Restart Claude Code after switching for changes to take effect.

## Step 7: Live status

While Claude is working, the device shows real-time status:

- **WORKING** — Claude is actively using tools
- **WAITING** — Claude is waiting for your input
- **IDLE** — no active session

Session uptime and active agent count are displayed alongside the state.

## Step 8: Settings

Long-press the idle screen to open settings:

- **Theme** — tap to highlight, tap again to apply (Terminal, Skeuo, or Brutalist)
- **Clock format** — toggle between 12-hour and 24-hour

All settings persist across reboots.

## Step 9: When you're done

Exit Claude Code normally (type `/exit` or Ctrl+C), then stop the bridge:

```bash
permitter stop
```

## Quick reference

| Command | What it does |
|---------|-------------|
| `permitter setup` | Install hooks in current project |
| `permitter start` | Start bridge server |
| `permitter stop` | Stop bridge server |
| `permitter status` | Check bridge connection |
| `permitter solo` | M5Stack only (no keyboard prompts) |
| `permitter dual` | Device + keyboard prompts |
| `permitter off` | Remove all hooks |
| `claude` | Launch Claude Code (hooks activate automatically) |
| Long-press idle screen | Open settings (themes + clock format) |

## Troubleshooting

**M5Stack shows "WAITING" instead of "READY"**
The device can't reach the bridge. Check that the bridge is running (`permitter status`) and that `BRIDGE_HOST` in `config.h` matches your computer's current local IP.

**Claude Code doesn't trigger the device**
Make sure you ran `permitter setup` in your project directory. Check that `.claude/settings.json` exists with the hook paths.

**Bridge not running / forgot to start it**
No problem — when the hook can't reach the bridge, it falls back to Claude Code's normal terminal permission prompt. Nothing breaks.

**Device won't connect to WiFi**
ESP32 only supports 2.4GHz. Make sure your router has a 2.4GHz band enabled and `WIFI_SSID` in `config.h` matches exactly (case-sensitive, spaces matter).

**Changes not taking effect after switching modes**
You need to restart Claude Code (`/exit` then `claude`) for hook/permission changes to take effect.

**"Disk not readable" dialog on macOS after flashing**
Normal — macOS doesn't recognize the ESP32 flash partition. Just click "Ignore".
