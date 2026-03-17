#!/bin/bash
# Permitter setup — run this in any project to enable the physical permission panel
# Usage: bash /path/to/permitter/setup.sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
HOOK_JS="$SCRIPT_DIR/bridge/hook.js"

if [ ! -f "$HOOK_JS" ]; then
  echo "Error: Can't find hook.js at $HOOK_JS"
  exit 1
fi

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
    ]
  }
}
INNEREOF

echo "Permitter hook installed in $(pwd)/.claude/settings.json"
echo ""
echo "To start a session:"
echo "  node $SCRIPT_DIR/bridge/index.js &"
echo "  claude"
