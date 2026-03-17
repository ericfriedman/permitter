const http = require("http");
const { classifyRisk } = require("./classifier");

const PORT = process.env.PERMITTER_PORT || 3737;

// Current pending permission request (or null)
let pending = null;
// Callbacks waiting for a response to the pending request
let waiters = [];

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
    return res.end(JSON.stringify({ connected: true, version: "0.1.0" }));
  }

  // GET /pending — current permission request
  if (req.method === "GET" && url.pathname === "/pending") {
    res.writeHead(200, { "Content-Type": "application/json" });
    return res.end(JSON.stringify(pending));
  }

  // POST /request — hook sends a new permission request, waits for response
  if (req.method === "POST" && url.pathname === "/request") {
    let body = "";
    req.on("data", (chunk) => (body += chunk));
    req.on("end", () => {
      try {
        const data = JSON.parse(body);
        const tool = data.tool || "Unknown";
        const action = data.action || "";
        const risk = classifyRisk(tool, action);

        pending = {
          id: String(Date.now()),
          tool,
          action,
          risk,
          context: data.context || "",
          timestamp: new Date().toISOString(),
        };

        console.log(`[PENDING] ${tool}: ${action} (${risk})`);

        // Wait for device response (timeout after 120s)
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
        console.log(`[RESPONSE] ${choice}`);
        pending = null;
        resolveWaiters(choice);
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
