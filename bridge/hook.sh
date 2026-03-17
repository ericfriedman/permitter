#!/bin/bash
# Permitter PreToolUse hook
# Reads tool info from stdin, POSTs to bridge, waits for device response

BRIDGE="http://localhost:${PERMITTER_PORT:-3737}"

# Read the hook input from stdin
INPUT=$(cat)

# Extract tool name and input
TOOL=$(echo "$INPUT" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('tool_name','Unknown'))" 2>/dev/null)
ACTION=$(echo "$INPUT" | python3 -c "
import sys,json
d=json.load(sys.stdin)
inp = d.get('tool_input',{})
# For Bash tool, show the command; for others show a summary
if isinstance(inp, dict):
    print(inp.get('command', inp.get('content', inp.get('query', json.dumps(inp)[:200]))))
else:
    print(str(inp)[:200])
" 2>/dev/null)

# Send to bridge and wait for response (up to 120s)
RESPONSE=$(curl -s --max-time 120 \
  -X POST "$BRIDGE/request" \
  -H "Content-Type: application/json" \
  -d "$(python3 -c "
import json
print(json.dumps({'tool': '$TOOL', 'action': '''$ACTION'''}))
" 2>/dev/null)" 2>/dev/null)

# Parse the choice
CHOICE=$(echo "$RESPONSE" | python3 -c "import sys,json; print(json.load(sys.stdin).get('choice','deny'))" 2>/dev/null)

# Return decision to Claude Code
case "$CHOICE" in
  allow)
    echo '{"decision": "allow"}'
    ;;
  always)
    echo '{"decision": "allow", "remember": true}'
    ;;
  *)
    echo '{"decision": "deny"}'
    ;;
esac
