#!/usr/bin/env node
// Permitter PreToolUse hook
// Sends request to bridge, blocks until device responds or bridge auto-approves.
// Falls back to normal Claude Code prompt if bridge is unreachable.

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
    console.log(JSON.stringify({}));
    return;
  }

  const tool = data.tool_name || "Unknown";
  const toolInput = data.tool_input || {};

  // Extract a readable action string
  let action = "";
  if (typeof toolInput === "string") {
    action = toolInput.slice(0, 500);
  } else if (toolInput.command) {
    action = toolInput.command.slice(0, 500);
  } else if (toolInput.file_path) {
    action = toolInput.file_path;
  } else if (toolInput.content) {
    action = toolInput.content.slice(0, 200);
  } else if (toolInput.pattern) {
    action = toolInput.pattern;
  } else {
    action = JSON.stringify(toolInput).slice(0, 300);
  }

  const payload = JSON.stringify({ tool, action });

  const req = http.request(
    {
      hostname: BRIDGE_HOST,
      port: BRIDGE_PORT,
      path: "/request",
      method: "POST",
      headers: { "Content-Type": "application/json" },
      timeout: 120000,
    },
    (res) => {
      let body = "";
      res.on("data", (chunk) => (body += chunk));
      res.on("end", () => {
        try {
          const result = JSON.parse(body);
          const choice = result.choice || "deny";
          if (choice === "always" || choice === "allow") {
            console.log(JSON.stringify({ decision: "allow" }));
          } else {
            console.log(JSON.stringify({ decision: "deny" }));
          }
        } catch {
          console.log(JSON.stringify({ decision: "deny" }));
        }
      });
    }
  );

  req.on("error", () => {
    // Bridge unreachable — fall back to normal terminal prompt
    console.log(JSON.stringify({}));
  });

  req.on("timeout", () => {
    req.destroy();
    console.log(JSON.stringify({ decision: "deny" }));
  });

  req.write(payload);
  req.end();
});
