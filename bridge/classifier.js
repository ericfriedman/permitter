// Risk classification logic
// Returns "low", "medium", or "high"

const HIGH_PATTERNS = [
  /\brm\b/, /--force/, /\bsudo\b/, /\bchmod\b/, /\bchown\b/,
  /curl\s.*\|\s*(ba)?sh/, /wget/, /\bdd\b/, /\bmkfs\b/,
  /\bDROP\b/, /\bDELETE\b/, /\btruncate\b/,
  /\|\s*(ba)?sh/, /\|\s*bash/
];

const MEDIUM_PATTERNS = [
  /[>]{1,2}/, /\btee\b/, /\bmv\b/, /\bcp\b/,
  /git\s+push/, /git\s+commit/, /npm\s+publish/, /\bdeploy\b/
];

function classifyRisk(tool, action) {
  if (tool === "Bash" || tool === "bash") {
    for (const p of HIGH_PATTERNS) {
      if (p.test(action)) return "high";
    }
    for (const p of MEDIUM_PATTERNS) {
      if (p.test(action)) return "medium";
    }
  }

  if (tool === "Write" || tool === "Edit") return "medium";

  return "low";
}

// Tools that also trigger Claude Code's built-in permission prompt
const DUAL_APPROVAL_TOOLS = new Set([
  "Bash", "bash",
  "WebFetch", "Fetch",
  "WebSearch",
]);

function needsDualApproval(tool) {
  return DUAL_APPROVAL_TOOLS.has(tool);
}

module.exports = { classifyRisk, needsDualApproval };
