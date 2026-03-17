#!/bin/bash
# Permitter setup — run this in any project to enable the physical permission panel
#
# Usage:
#   bash /path/to/permitter/setup.sh           Install hooks + solo mode (M5Stack = sole approver)
#   bash /path/to/permitter/setup.sh --solo     Switch to solo mode (no keyboard prompts)
#   bash /path/to/permitter/setup.sh --dual     Switch to dual mode (device + keyboard prompts)
#   bash /path/to/permitter/setup.sh --off      Remove all Permitter hooks and permissions

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
HOOK_JS="$SCRIPT_DIR/bridge/hook.js"
HOOK_PERM="$SCRIPT_DIR/bridge/hook-permission.js"
HOOK_STATUS="$SCRIPT_DIR/bridge/hook-status.js"
MODE="${1:---solo}"

if [ ! -f "$HOOK_JS" ]; then
  echo "Error: Can't find hook.js at $HOOK_JS"
  exit 1
fi

# --off: remove everything
if [ "$MODE" = "--off" ]; then
  rm -f .claude/settings.json .claude/settings.local.json
  echo "Permitter disabled. Hooks and permissions removed."
  echo "Restart Claude Code for changes to take effect."
  exit 0
fi

# --dual: just remove the broad allows
if [ "$MODE" = "--dual" ]; then
  rm -f .claude/settings.local.json
  echo "Switched to dual mode — device + keyboard approval."
  echo "Restart Claude Code for changes to take effect."
  exit 0
fi

# --solo (or first install): write hooks + broad allows
mkdir -p .claude

cat > .claude/settings.json << INNEREOF
{
  "hooks": {
    "PreToolUse": [
      {
        "matcher": "",
        "hooks": [
          {
            "type": "command",
            "command": "node $HOOK_JS"
          }
        ]
      }
    ],
    "PermissionRequest": [
      {
        "matcher": "",
        "hooks": [
          {
            "type": "command",
            "command": "node $HOOK_PERM"
          }
        ]
      }
    ],
    "PostToolUse": [
      {
        "matcher": "",
        "hooks": [
          {
            "type": "command",
            "command": "node $HOOK_STATUS"
          }
        ]
      }
    ],
    "SubagentStart": [
      {
        "matcher": "",
        "hooks": [
          {
            "type": "command",
            "command": "node $HOOK_STATUS"
          }
        ]
      }
    ],
    "SubagentStop": [
      {
        "matcher": "",
        "hooks": [
          {
            "type": "command",
            "command": "node $HOOK_STATUS"
          }
        ]
      }
    ],
    "Stop": [
      {
        "hooks": [
          {
            "type": "command",
            "command": "node $HOOK_STATUS"
          }
        ]
      }
    ],
    "Notification": [
      {
        "matcher": "",
        "hooks": [
          {
            "type": "command",
            "command": "node $HOOK_STATUS"
          }
        ]
      }
    ]
  }
}
INNEREOF

if [ "$MODE" = "--solo" ] || [ "$MODE" = "" ]; then
  cat > .claude/settings.local.json << INNEREOF
{
  "permissions": {
    "allow": [
      "Bash(*)",
      "WebFetch(*)",
      "WebSearch(*)"
    ]
  }
}
INNEREOF
  echo "Solo mode enabled — M5Stack is the sole approver (no keyboard prompts)."
else
  echo "Hooks installed (dual mode — device + keyboard approval)."
fi

echo ""
echo "Hooks installed in $(pwd)/.claude/settings.json"
echo ""
echo "Toggle modes:"
echo "  bash $0 --solo    M5Stack only (no keyboard prompts)"
echo "  bash $0 --dual    Device + keyboard prompts"
echo "  bash $0 --off     Remove all Permitter hooks"
echo ""
echo "To start a session:"
echo "  node $SCRIPT_DIR/bridge/index.js &"
echo "  claude"
