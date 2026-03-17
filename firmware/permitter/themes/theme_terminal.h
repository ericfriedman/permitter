#ifndef THEME_TERMINAL_H
#define THEME_TERMINAL_H

#include "../theme_interface.h"

// CRT phosphor green palette
#define T_BG       0x0000
#define T_GREEN    0x07E0
#define T_DIM      0x0320
#define T_DARK     0x0120
#define T_AMBER    0xFE00
#define T_RED      0xF800

class TerminalTheme : public PermitterTheme {
private:
  void scanlines() {
    for (int y = 0; y < 240; y += 8)
      M5.Display.drawFastHLine(0, y, 320, T_DARK);
  }

  void scanlines(int y0, int y1) {
    for (int y = y0; y < y1; y += 8)
      M5.Display.drawFastHLine(0, y, 320, T_DARK);
  }

  void box(int x, int y, int w, int h, uint16_t c) {
    M5.Display.drawRect(x, y, w, h, c);
  }

  // Draw a zone with double-line border for that terminal feel
  void termZone(int x, int y, int w, int h, uint16_t bg, uint16_t fg, uint16_t dim,
                const char* label, const char* sublabel) {
    M5.Display.fillRect(x, y, w, h, bg);
    M5.Display.drawRect(x, y, w, h, dim);
    // Label centered horizontally and vertically
    int labelW = strlen(label) * 12;
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(fg, bg);
    M5.Display.setCursor(x + (w - labelW) / 2, y + h / 2 - 16);
    M5.Display.print(label);
    // Sublabel centered below label
    int subW = strlen(sublabel) * 6;
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(dim, bg);
    M5.Display.setCursor(x + (w - subW) / 2, y + h / 2 + 8);
  }

public:
  const char* name() override { return "terminal"; }
  const char* displayName() override { return "Terminal"; }

  void setup() override {}

  void drawBoot(const char* ssid) override {
    M5.Display.fillScreen(T_BG);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(T_GREEN, T_BG);
    M5.Display.setCursor(8, 8);
    M5.Display.println("PERMITTER BOOT SEQUENCE");
    M5.Display.println();
    M5.Display.print("  Connecting to ");
    M5.Display.print(ssid);
    M5.Display.print("...");
  }

  void drawBootStatus(bool wifiOk, const char* ip) override {
    if (wifiOk) {
      M5.Display.setTextColor(T_GREEN, T_BG);
      M5.Display.println(" OK");
      M5.Display.printf("  IP: %s\n", ip);
    } else {
      M5.Display.setTextColor(T_RED, T_BG);
      M5.Display.println(" FAIL");
    }
    M5.Display.println();
    M5.Display.setTextColor(T_DIM, T_BG);
    M5.Display.println("  Starting bridge monitor...");
  }

  void drawIdle(bool bridgeConnected) override {
    M5.Display.fillScreen(T_BG);

    // Header bar
    M5.Display.fillRect(0, 0, 320, 18, T_DIM);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(T_BG, T_DIM);
    M5.Display.setCursor(4, 5);
    M5.Display.print(" PERMITTER v0.1 ");
    const char* st = bridgeConnected ? "[BRIDGE OK]" : "[NO BRIDGE]";
    M5.Display.setCursor(248, 5);
    M5.Display.print(st);

    // Prompt — larger
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(T_DIM, T_BG);
    M5.Display.setCursor(16, 108);
    M5.Display.print("> standing by_");

    // Taller bottom zones starting at y=130
    M5.Display.drawFastHLine(0, 128, 320, T_DIM);
    termZone(0, 130, 106, 110, T_DARK, T_GREEN, T_DIM, "TRUST", "[1] always");
    termZone(107, 130, 106, 110, 0x2100, T_AMBER, T_DIM, "ONCE", "[2] this time");
    termZone(214, 130, 106, 110, 0x4000, T_RED, T_DIM, "DENY", "[3] reject");

    scanlines();
  }

  void drawClock(const char* timeStr, const char* dateStr) override {
    M5.Display.fillRect(0, 22, 320, 72, T_BG);

    // Big time
    M5.Display.setTextSize(4);
    M5.Display.setTextColor(T_GREEN, T_BG);
    M5.Display.setCursor(48, 30);
    M5.Display.print(timeStr);

    // Date
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(T_DIM, T_BG);
    M5.Display.setCursor(110, 70);
    M5.Display.print(dateStr);

    // Separator line
    M5.Display.drawFastHLine(8, 85, 304, T_DIM);

    scanlines(22, 110);
  }

  void drawPermission(PermissionRequest req) override {
    M5.Display.fillScreen(T_BG);

    uint16_t rc = T_GREEN;
    const char* rl = "LOW";
    if (req.risk == "medium") { rc = T_AMBER; rl = "MED"; }
    if (req.risk == "high") { rc = T_RED; rl = "HIGH"; }

    // Header
    M5.Display.fillRect(0, 0, 320, 18, rc);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(T_BG, rc);
    M5.Display.setCursor(4, 5);
    M5.Display.printf(" PERMISSION REQUEST [%s RISK] ", rl);

    // Tool
    M5.Display.setTextSize(3);
    M5.Display.setTextColor(rc, T_BG);
    M5.Display.setCursor(8, 24);
    M5.Display.print(req.tool.c_str());

    M5.Display.drawFastHLine(8, 50, 304, T_DIM);

    // Action — larger text
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(T_GREEN, T_BG);
    String a = req.action;
    int line = 0, pos = 0;
    while (pos < (int)a.length() && line < 3) {
      int end = pos + 24;
      if (end > (int)a.length()) end = a.length();
      M5.Display.setCursor(8, 54 + line * 22);
      M5.Display.print(a.substring(pos, end).c_str());
      pos = end;
      line++;
    }

    // Dual approval notice
    if (req.dual) {
      M5.Display.setTextSize(2);
      M5.Display.setTextColor(T_AMBER, T_BG);
      M5.Display.setCursor(10, 116);
      M5.Display.print("+ TERMINAL APPROVE");
    }

    // Taller zones
    M5.Display.drawFastHLine(0, 128, 320, T_DIM);
    termZone(0, 130, 106, 110, T_DARK, T_GREEN, T_DIM, "TRUST", "[1] always");
    termZone(107, 130, 106, 110, 0x2100, T_AMBER, T_DIM, "ONCE", "[2] this time");
    termZone(214, 130, 106, 110, 0x4000, T_RED, T_DIM, "DENY", "[3] reject");

    scanlines();
  }

  void drawConfirm(bool accepted, const char* tool) override {
    uint16_t c = accepted ? T_GREEN : T_RED;
    M5.Display.fillScreen(T_BG);

    // Box around confirmation
    box(20, 40, 280, 100, c);
    box(22, 42, 276, 96, c);

    M5.Display.setTextSize(4);
    M5.Display.setTextColor(c, T_BG);
    M5.Display.setCursor(accepted ? 30 : 48, 60);
    M5.Display.print(accepted ? "ALLOWED" : "DENIED");

    M5.Display.setTextSize(1);
    M5.Display.setTextColor(T_DIM, T_BG);
    M5.Display.setCursor(120, 105);
    M5.Display.printf("> %s", tool);

    scanlines();
  }

  void drawStatusBar(const char* state, int agents, int uptime) override {
    M5.Display.fillRect(0, 96, 320, 30, T_BG);

    // State indicator — larger
    uint16_t sc = T_DIM;
    if (strcmp(state, "working") == 0) sc = T_GREEN;
    if (strcmp(state, "waiting") == 0) sc = T_AMBER;
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(sc, T_BG);
    M5.Display.setCursor(4, 98);
    if (strcmp(state, "working") == 0) M5.Display.print(">WORKING");
    else if (strcmp(state, "waiting") == 0) M5.Display.print(">WAITING");
    else M5.Display.print(">IDLE");

    // Agents + uptime
    M5.Display.setTextColor(T_DIM, T_BG);
    int m = uptime / 60;
    int s = uptime % 60;
    if (agents > 0) {
      M5.Display.setCursor(140, 98);
      M5.Display.printf("%d agt %dm%02ds", agents, m, s);
    } else {
      M5.Display.setCursor(180, 98);
      M5.Display.printf("%dm%02ds", m, s);
    }
    // No scanlines here — readability over aesthetics
  }

  void drawActivity(const char* tool, const char* action) override {
    // Brief flash in the prompt area
    M5.Display.fillRect(0, 96, 320, 30, T_BG);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(T_DIM, T_BG);
    M5.Display.setCursor(4, 100);
    String label = String("> ") + String(tool) + " " + String(action);
    if (label.length() > 26) label = label.substring(0, 26);
    M5.Display.print(label.c_str());
    // No scanlines here — readability over aesthetics
  }

  void playAlertSound() override {
    M5.Speaker.tone(1500, 100);
    delay(120);
    M5.Speaker.tone(2000, 80);
  }

  void playConfirmSound(bool accepted) override {
    M5.Speaker.tone(accepted ? 800 : 300, 100);
  }
};

#endif
