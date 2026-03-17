#ifndef THEME_BRUTALIST_H
#define THEME_BRUTALIST_H

#include "../theme_interface.h"

// Brutalist palette — concrete, steel, hazard
#define B_CONCRETE   0xCE59  // warm concrete gray
#define B_DARK       0x3186  // dark concrete
#define B_BLACK      0x0000
#define B_WHITE      0xFFFF
#define B_YELLOW     0xFFE0  // hazard yellow
#define B_RED        0xF800  // danger red
#define B_STEEL      0x7BCF  // steel gray

class BrutalistTheme : public PermitterTheme {
private:
  void hazardBar(int y, int h) {
    M5.Display.fillRect(0, y, 320, h, B_YELLOW);
    // Checkerboard hazard pattern
    for (int x = 0; x < 320; x += h * 2) {
      M5.Display.fillRect(x, y, h, h, B_BLACK);
    }
  }

  void blockButton(int x, int y, int w, int h, uint16_t color, uint16_t textColor, const char* label) {
    M5.Display.fillRect(x, y, w, h, color);
    // Heavy double border — brutalist
    M5.Display.drawRect(x, y, w, h, B_BLACK);
    M5.Display.drawRect(x + 1, y + 1, w - 2, h - 2, B_BLACK);
    M5.Display.drawRect(x + 2, y + 2, w - 4, h - 4, B_BLACK);

    int textW = strlen(label) * 18;
    M5.Display.setTextSize(3);
    M5.Display.setTextColor(textColor, color);
    M5.Display.setCursor(x + (w - textW) / 2, y + (h - 21) / 2);
    M5.Display.print(label);
  }

public:
  const char* name() override { return "brutalist"; }
  const char* displayName() override { return "Brutalist"; }

  void setup() override {}

  void drawBoot(const char* ssid) override {
    M5.Display.fillScreen(B_CONCRETE);
    M5.Display.fillRect(0, 0, 320, 40, B_BLACK);
    M5.Display.setTextSize(3);
    M5.Display.setTextColor(B_WHITE, B_BLACK);
    M5.Display.setCursor(30, 8);
    M5.Display.print("PERMITTER");

    M5.Display.setTextSize(1);
    M5.Display.setTextColor(B_BLACK, B_CONCRETE);
    M5.Display.setCursor(10, 55);
    M5.Display.printf("NETWORK: %s", ssid);
  }

  void drawBootStatus(bool wifiOk, const char* ip) override {
    M5.Display.setTextSize(2);
    if (wifiOk) {
      M5.Display.setTextColor(B_BLACK, B_CONCRETE);
      M5.Display.setCursor(10, 80);
      M5.Display.print("CONNECTED");
      M5.Display.setTextSize(1);
      M5.Display.setCursor(10, 100);
      M5.Display.print(ip);
    } else {
      M5.Display.setTextColor(B_RED, B_CONCRETE);
      M5.Display.setCursor(10, 80);
      M5.Display.print("NO CONNECTION");
    }
  }

  void drawIdle(bool bridgeConnected) override {
    M5.Display.fillScreen(B_CONCRETE);

    // Heavy black header
    M5.Display.fillRect(0, 0, 320, 28, B_BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(B_WHITE, B_BLACK);
    M5.Display.setCursor(8, 6);
    M5.Display.print("PERMITTER");

    // Status block
    uint16_t stColor = bridgeConnected ? B_WHITE : B_YELLOW;
    M5.Display.fillRect(218, 3, 98, 22, stColor);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(B_BLACK, stColor);
    M5.Display.setCursor(228, 10);
    M5.Display.print(bridgeConnected ? "  READY  " : " WAITING ");

    // Idle text
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(B_DARK, B_CONCRETE);
    M5.Display.setCursor(30, 112);
    M5.Display.print("STANDING BY FOR AUTHORIZATION");

    // Hazard bar
    hazardBar(124, 6);

    // Button zone — taller
    M5.Display.fillRect(0, 130, 320, 110, B_DARK);
    blockButton(3, 134, 103, 100, B_WHITE, B_BLACK, "YES");
    blockButton(109, 134, 103, 100, B_YELLOW, B_BLACK, "1x");
    blockButton(215, 134, 102, 100, B_RED, B_WHITE, "NO");
  }

  void drawClock(const char* timeStr, const char* dateStr) override {
    M5.Display.fillRect(0, 32, 320, 76, B_CONCRETE);

    // Heavy time
    M5.Display.setTextSize(4);
    M5.Display.setTextColor(B_BLACK, B_CONCRETE);
    M5.Display.setCursor(48, 36);
    M5.Display.print(timeStr);

    // Date — understated
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(B_DARK, B_CONCRETE);
    M5.Display.setCursor(115, 76);
    M5.Display.print(dateStr);

    // Thick rule
    M5.Display.fillRect(10, 92, 300, 4, B_BLACK);
  }

  void drawPermission(PermissionRequest req) override {
    M5.Display.fillScreen(B_CONCRETE);

    // Risk-colored header
    uint16_t hdrColor = B_WHITE;
    uint16_t hdrText = B_BLACK;
    const char* rl = "LOW";
    if (req.risk == "medium") { hdrColor = B_YELLOW; rl = "MED"; }
    if (req.risk == "high") { hdrColor = B_RED; hdrText = B_WHITE; rl = "HIGH"; }

    M5.Display.fillRect(0, 0, 320, 28, B_BLACK);
    // Risk block
    M5.Display.fillRect(0, 0, 80, 28, hdrColor);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(hdrText, hdrColor);
    M5.Display.setCursor(8, 6);
    M5.Display.print(rl);

    // Tool name
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(B_WHITE, B_BLACK);
    M5.Display.setCursor(90, 6);
    M5.Display.print(req.tool.c_str());

    // Thick color separator
    M5.Display.fillRect(0, 28, 320, 4, hdrColor);

    // Action in white card with heavy border
    M5.Display.fillRect(4, 36, 312, 80, B_WHITE);
    M5.Display.drawRect(4, 36, 312, 80, B_BLACK);
    M5.Display.drawRect(5, 37, 310, 78, B_BLACK);
    M5.Display.drawRect(6, 38, 308, 76, B_BLACK);

    M5.Display.setTextSize(1);
    M5.Display.setTextColor(B_BLACK, B_WHITE);
    String a = req.action;
    int line = 0, pos = 0;
    while (pos < (int)a.length() && line < 5) {
      int end = pos + 48;
      if (end > (int)a.length()) end = a.length();
      M5.Display.setCursor(12, 44 + line * 13);
      M5.Display.print(a.substring(pos, end).c_str());
      pos = end;
      line++;
    }

    // Hazard bar
    hazardBar(124, 6);

    // Taller buttons
    M5.Display.fillRect(0, 130, 320, 110, B_DARK);
    blockButton(3, 134, 103, 100, B_WHITE, B_BLACK, "YES");
    blockButton(109, 134, 103, 100, B_YELLOW, B_BLACK, "1x");
    blockButton(215, 134, 102, 100, B_RED, B_WHITE, "NO");
  }

  void drawConfirm(bool accepted, const char* tool) override {
    uint16_t bg = accepted ? B_WHITE : B_RED;
    uint16_t fg = accepted ? B_BLACK : B_WHITE;

    M5.Display.fillScreen(bg);

    // Giant text
    M5.Display.setTextSize(4);
    M5.Display.setTextColor(fg, bg);
    M5.Display.setCursor(accepted ? 16 : 36, 55);
    M5.Display.print(accepted ? "APPROVED" : "DENIED");

    // Heavy rule
    M5.Display.fillRect(10, 100, 300, 6, fg);

    M5.Display.setTextSize(2);
    M5.Display.setTextColor(fg, bg);
    M5.Display.setCursor(100, 120);
    M5.Display.print(tool);

    if (!accepted) {
      hazardBar(0, 8);
      hazardBar(232, 8);
    }
  }

  void playAlertSound() override {
    M5.Speaker.tone(600, 200);
    delay(250);
    M5.Speaker.tone(600, 200);
    delay(250);
    M5.Speaker.tone(600, 200);
  }

  void playConfirmSound(bool accepted) override {
    if (accepted) {
      M5.Speaker.tone(1000, 50);
    } else {
      M5.Speaker.tone(200, 400);
    }
  }
};

#endif
