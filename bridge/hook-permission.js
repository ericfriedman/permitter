#!/usr/bin/env node
// Permitter PermissionRequest hook
// Fires when Claude Code's built-in permission dialog would show.
// If the device already approved this tool via PreToolUse, auto-approve here too.
// This eliminates the "double prompt" for Bash, Fetch, etc.

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

  // Check if this tool was already approved by the device (in the trusted set)
  const req = http.request(
    {
      hostname: BRIDGE_HOST,
      port: BRIDGE_PORT,
      path: "/status",
      method: "GET",
      timeout: 2000,
    },
    (res) => {
      let body = "";
      res.on("data", (chunk) => (body += chunk));
      res.on("end", () => {
        try {
          const status = JSON.parse(body);
          const trusted = status.trusted || [];
          if (trusted.includes(tool)) {
            // Device already trusted this tool — auto-approve the built-in prompt
            console.log(JSON.stringify({
              hookSpecificOutput: {
                hookEventName: "PermissionRequest",
                decision: { behavior: "allow" }
              }
            }));
          } else {
            // Not trusted — let the normal prompt show
            console.log(JSON.stringify({}));
          }
        } catch {
          console.log(JSON.stringify({}));
        }
      });
    }
  );

  req.on("error", () => {
    console.log(JSON.stringify({}));
  });

  req.on("timeout", () => {
    req.destroy();
    console.log(JSON.stringify({}));
  });

  req.end();
});
