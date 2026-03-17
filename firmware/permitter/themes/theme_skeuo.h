#ifndef THEME_SKEUO_H
#define THEME_SKEUO_H

#include "../theme_interface.h"

// iOS 6 / Skeuomorphic palette
#define S_LINEN      0xEF5B  // warm linen background
#define S_FELT       0x4228  // dark felt/leather
#define S_CHROME     0xC618  // brushed aluminum
#define S_CHROME_HI  0xDEFB  // chrome highlight
#define S_BLUE       0x2B5F  // iOS blue button
#define S_BLUE_HI    0x43BF  // blue highlight
#define S_GREEN_BTN  0x2EC8  // green gel button
#define S_GREEN_HI   0x4F0A  // green highlight
#define S_RED_BTN    0xC082  // red gel button
#define S_RED_HI     0xE8C4  // red highlight
#define S_AMBER_BTN  0xDD00  // amber button
#define S_AMBER_HI   0xFE40  // amber highlight
#define S_TEXT_DARK  0x2104  // dark text
#define S_TEXT_LIGHT 0xFFFF  // white text
#define S_SHADOW     0x4208  // drop shadow

class SkeuoTheme : public PermitterTheme {
private:
  // Draw a beveled gel button
  void gelButton(int x, int y, int w, int h, uint16_t base, uint16_t hi, const char* label) {
    // Shadow
    M5.Display.fillRoundRect(x + 2, y + 2, w, h, 6, S_SHADOW);
    // Base
    M5.Display.fillRoundRect(x, y, w, h, 6, base);
    // Highlight (top half)
    M5.Display.fillRoundRect(x + 2, y + 2, w - 4, h / 2 - 2, 4, hi);
    // Label
    int textW = strlen(label) * 12;
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(S_TEXT_LIGHT, base);
    M5.Display.setCursor(x + (w - textW) / 2, y + h / 2 + 2);
    M5.Display.print(label);
  }

  void chromeBar(int y, int h) {
    M5.Display.fillRect(0, y, 320, h, S_CHROME);
    M5.Display.drawFastHLine(0, y, 320, S_CHROME_HI);
    M5.Display.drawFastHLine(0, y + h - 1, 320, S_SHADOW);
  }

public:
  const char* name() override { return "skeuo"; }
  const char* displayName() override { return "Skeuo"; }

  void setup() override {}

  void drawBoot(const char* ssid) override {
    M5.Display.fillScreen(S_LINEN);
    chromeBar(0, 28);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(S_TEXT_DARK, S_CHROME);
    M5.Display.setCursor(60, 6);
    M5.Display.print("Permitter");
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(S_TEXT_DARK, S_LINEN);
    M5.Display.setCursor(20, 50);
    M5.Display.printf("Connecting to %s...", ssid);
  }

  void drawBootStatus(bool wifiOk, const char* ip) override {
    M5.Display.setTextSize(1);
    if (wifiOk) {
      M5.Display.setTextColor(0x0400, S_LINEN);
      M5.Display.setCursor(20, 70);
      M5.Display.printf("Connected: %s", ip);
    } else {
      M5.Display.setTextColor(S_RED_BTN, S_LINEN);
      M5.Display.setCursor(20, 70);
      M5.Display.print("Connection failed");
    }
  }

  void drawIdle(bool bridgeConnected) override {
    M5.Display.fillScreen(S_LINEN);

    // Chrome header
    chromeBar(0, 28);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(S_TEXT_DARK, S_CHROME);
    M5.Display.setCursor(60, 6);
    M5.Display.print("Permitter");

    // Status pill
    uint16_t pillColor = bridgeConnected ? S_GREEN_BTN : S_AMBER_BTN;
    M5.Display.fillRoundRect(255, 7, 55, 14, 7, pillColor);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(S_TEXT_LIGHT, pillColor);
    M5.Display.setCursor(260, 10);
    M5.Display.print(bridgeConnected ? "Ready" : "Wait");

    // Subtle label
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(S_SHADOW, S_LINEN);
    M5.Display.setCursor(80, 118);
    M5.Display.print("Waiting for requests...");

    // Felt texture zone area — taller
    M5.Display.fillRect(0, 130, 320, 110, S_FELT);
    M5.Display.drawFastHLine(0, 130, 320, S_SHADOW);

    // Bigger gel buttons
    gelButton(5, 138, 100, 94, S_GREEN_BTN, S_GREEN_HI, "Trust");
    gelButton(110, 138, 100, 94, S_AMBER_BTN, S_AMBER_HI, "Once");
    gelButton(215, 138, 100, 94, S_RED_BTN, S_RED_HI, "Deny");
  }

  void drawClock(const char* timeStr, const char* dateStr) override {
    M5.Display.fillRect(0, 30, 320, 85, S_LINEN);

    // Time - smaller, elegant
    M5.Display.setTextSize(3);
    M5.Display.setTextColor(S_TEXT_DARK, S_LINEN);
    M5.Display.setCursor(65, 40);
    M5.Display.print(timeStr);

    // Date
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(S_SHADOW, S_LINEN);
    M5.Display.setCursor(110, 72);
    M5.Display.print(dateStr);
  }

  void drawPermission(PermissionRequest req) override {
    M5.Display.fillScreen(S_LINEN);

    // Risk-colored header
    uint16_t hdrColor = S_GREEN_BTN;
    const char* rl = "LOW RISK";
    if (req.risk == "medium") { hdrColor = S_AMBER_BTN; rl = "MEDIUM RISK"; }
    if (req.risk == "high") { hdrColor = S_RED_BTN; rl = "HIGH RISK"; }

    chromeBar(0, 28);
    // Risk pill on header
    M5.Display.fillRoundRect(4, 5, 90, 18, 9, hdrColor);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(S_TEXT_LIGHT, hdrColor);
    M5.Display.setCursor(12, 10);
    M5.Display.print(rl);

    // Tool name
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(S_TEXT_DARK, S_CHROME);
    M5.Display.setCursor(100, 6);
    M5.Display.print(req.tool.c_str());

    // Card for action — shorter to make room for bigger buttons
    M5.Display.fillRoundRect(8, 34, 304, 88, 8, S_TEXT_LIGHT);
    M5.Display.drawRoundRect(8, 34, 304, 88, 8, S_CHROME);

    // Action text
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(S_TEXT_DARK, S_TEXT_LIGHT);
    String a = req.action;
    int line = 0, pos = 0;
    while (pos < (int)a.length() && line < 6) {
      int end = pos + 46;
      if (end > (int)a.length()) end = a.length();
      M5.Display.setCursor(16, 42 + line * 12);
      M5.Display.print(a.substring(pos, end).c_str());
      pos = end;
      line++;
    }

    // Felt zone + bigger gel buttons
    M5.Display.fillRect(0, 130, 320, 110, S_FELT);
    M5.Display.drawFastHLine(0, 130, 320, S_SHADOW);
    gelButton(5, 138, 100, 94, S_GREEN_BTN, S_GREEN_HI, "Trust");
    gelButton(110, 138, 100, 94, S_AMBER_BTN, S_AMBER_HI, "Once");
    gelButton(215, 138, 100, 94, S_RED_BTN, S_RED_HI, "Deny");
  }

  void drawConfirm(bool accepted, const char* tool) override {
    uint16_t bg = accepted ? S_GREEN_BTN : S_RED_BTN;
    M5.Display.fillScreen(S_LINEN);

    // Big centered card
    M5.Display.fillRoundRect(20, 40, 280, 120, 12, bg);
    M5.Display.drawRoundRect(20, 40, 280, 120, 12, S_SHADOW);

    // Highlight
    M5.Display.fillRoundRect(24, 44, 272, 50, 8, accepted ? S_GREEN_HI : S_RED_HI);

    M5.Display.setTextSize(3);
    M5.Display.setTextColor(S_TEXT_LIGHT, accepted ? S_GREEN_HI : S_RED_HI);
    M5.Display.setCursor(accepted ? 50 : 62, 55);
    M5.Display.print(accepted ? "ALLOWED" : "DENIED");

    M5.Display.setTextSize(2);
    M5.Display.setTextColor(S_TEXT_LIGHT, bg);
    M5.Display.setCursor(100, 120);
    M5.Display.print(tool);
  }

  void playAlertSound() override {
    M5.Speaker.tone(880, 150);
    delay(170);
    M5.Speaker.tone(880, 150);
  }

  void playConfirmSound(bool accepted) override {
    if (accepted) {
      M5.Speaker.tone(523, 80);
      delay(100);
      M5.Speaker.tone(659, 80);
      delay(100);
      M5.Speaker.tone(784, 100);
    } else {
      M5.Speaker.tone(400, 200);
      delay(220);
      M5.Speaker.tone(300, 300);
    }
  }
};

#endif
