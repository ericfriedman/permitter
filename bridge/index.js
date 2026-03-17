const http = require("http");
const { classifyRisk, needsDualApproval } = require("./classifier");

const PORT = process.env.PERMITTER_PORT || 3737;

// Current pending permission request (or null)
let pending = null;
// Callbacks waiting for a response
let waiters = [];
// Tools the user has "trusted" (always allow)
const trustedTools = new Set();
// Last activity for the device to display
let lastActivity = null;

function resolveWaiters(choice) {
  const cbs = waiters;
  waiters = [];
  for (const cb of cbs) cb(choice);
}

const server = http.createServer((req, res) => {
  res.setHeader("Access-Control-Allow-Origin", "*");
  res.setHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  res.setHeader("Access-Control-Allow-Headers", "Content-Type");

  if (req.method === "OPTIONS") {
    res.writeHead(204);
    return res.end();
  }

  const url = new URL(req.url, `http://localhost:${PORT}`);

  // GET /status — heartbeat
  if (req.method === "GET" && url.pathname === "/status") {
    res.writeHead(200, { "Content-Type": "application/json" });
    return res.end(
      JSON.stringify({
        connected: true,
        version: "0.3.0",
        trusted: [...trustedTools],
      })
    );
  }

  // GET /pending — current state for device
  // Returns: permission request, activity flash, or null
  if (req.method === "GET" && url.pathname === "/pending") {
    // Permission requests take priority
    if (pending) {
      res.writeHead(200, { "Content-Type": "application/json" });
      return res.end(JSON.stringify(pending));
    }
    // Activity flash (auto-approved tool)
    if (lastActivity) {
      const activity = lastActivity;
      lastActivity = null; // clear after one read
      res.writeHead(200, { "Content-Type": "application/json" });
      return res.end(JSON.stringify(activity));
    }
    res.writeHead(200, { "Content-Type": "application/json" });
    return res.end("null");
  }

  // POST /request — hook sends a permission request
  if (req.method === "POST" && url.pathname === "/request") {
    let body = "";
    req.on("data", (chunk) => (body += chunk));
    req.on("end", () => {
      try {
        const data = JSON.parse(body);
        const tool = data.tool || "Unknown";
        const action = data.action || "";
        const risk = classifyRisk(tool, action);

        // Trusted tool — auto-approve and notify device
        if (trustedTools.has(tool)) {
          console.log(`[AUTO] ${tool}: ${action} (trusted)`);
          lastActivity = {
            type: "activity",
            tool,
            action,
            risk,
            timestamp: new Date().toISOString(),
          };
          res.writeHead(200, { "Content-Type": "application/json" });
          return res.end(JSON.stringify({ choice: "allow" }));
        }

        // Not trusted — hold for device
        const dual = needsDualApproval(tool);
        pending = {
          type: "permission",
          id: String(Date.now()),
          tool,
          action,
          risk,
          dual: dual ? "1" : "0",
          timestamp: new Date().toISOString(),
        };

        console.log(`[PENDING] ${tool}: ${action} (${risk})`);

        // Timeout after 120s
        const timeout = setTimeout(() => {
          resolveWaiters("deny");
          pending = null;
        }, 120000);

        waiters.push((choice) => {
          clearTimeout(timeout);
          res.writeHead(200, { "Content-Type": "application/json" });
          res.end(JSON.stringify({ choice }));
        });
      } catch (e) {
        res.writeHead(400, { "Content-Type": "application/json" });
        res.end(JSON.stringify({ error: "Invalid JSON" }));
      }
    });
    return;
  }

  // POST /respond — device sends its choice
  if (req.method === "POST" && url.pathname === "/respond") {
    let body = "";
    req.on("data", (chunk) => (body += chunk));
    req.on("end", () => {
      try {
        const data = JSON.parse(body);
        const choice = data.choice; // "allow", "always", or "deny"

        // If "always", add tool to trusted set
        if (choice === "always" && pending) {
          trustedTools.add(pending.tool);
          console.log(`[TRUSTED] ${pending.tool} added to trusted tools`);
        }

        console.log(`[RESPONSE] ${choice}`);
        pending = null;
        resolveWaiters(choice === "always" ? "allow" : choice);
        res.writeHead(200, { "Content-Type": "application/json" });
        res.end(JSON.stringify({ ok: true }));
      } catch (e) {
        res.writeHead(400, { "Content-Type": "application/json" });
        res.end(JSON.stringify({ error: "Invalid JSON" }));
      }
    });
    return;
  }

  res.writeHead(404);
  res.end("Not found");
});

server.listen(PORT, "0.0.0.0", () => {
  console.log(`Permitter bridge listening on http://0.0.0.0:${PORT}`);
});
