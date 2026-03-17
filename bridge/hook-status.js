#!/usr/bin/env node
// Permitter status hook — fires on PostToolUse, SubagentStart, SubagentStop, Stop, Notification
// Sends events to the bridge so the M5Stack can show live Claude status.

const http = require("http");

const BRIDGE_PORT = process.env.PERMITTER_PORT || 3737;
const BRIDGE_HOST = "localhost";

let input = "";
process.stdin.on("data", (chunk) => (input += chunk));
process.stdin.on("end", () => {
  let data;
  try {
    data = JSON.parse(input);
  } catch {
    process.exit(0);
    return;
  }

  const event = data.hook_event_name || "";
  const payload = { event };

  if (event === "PostToolUse" || event === "PostToolUseFailure") {
    payload.tool = data.tool_name || "";
    payload.success = event === "PostToolUse";
  } else if (event === "SubagentStart") {
    payload.agentId = data.agent_id || "";
    payload.agentType = data.agent_type || "";
  } else if (event === "SubagentStop") {
    payload.agentId = data.agent_id || "";
    payload.agentType = data.agent_type || "";
  } else if (event === "Stop") {
    payload.message = (data.last_assistant_message || "").slice(0, 200);
  } else if (event === "Notification") {
    payload.message = data.message || "";
    payload.type = data.notification_type || "";
  }

  const body = JSON.stringify(payload);

  // Fire-and-forget POST to bridge
  const req = http.request(
    {
      hostname: BRIDGE_HOST,
      port: BRIDGE_PORT,
      path: "/event",
      method: "POST",
      headers: { "Content-Type": "application/json" },
      timeout: 2000,
    },
    () => {}
  );
  req.on("error", () => {});
  req.on("timeout", () => req.destroy());
  req.write(body);
  req.end();

  // Don't block Claude — exit immediately
  console.log(JSON.stringify({}));
});
