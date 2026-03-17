#include <M5Unified.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include "config.h"
#include "theme_interface.h"
#include "theme_registry.h"

#define SCREEN_W 320
#define SCREEN_H 240
#define ZONE_Y_TOP 158
#define ZONE_1_X_MAX 106
#define ZONE_2_X_MAX 213
#define LONG_PRESS_MS 1500

enum TouchZone { ZONE_NONE = 0, ZONE_ALWAYS = 1, ZONE_ONCE = 2, ZONE_DENY = 3 };
enum State { STATE_IDLE, STATE_PERMISSION, STATE_CONFIRM, STATE_THEME_PICKER, STATE_DISCONNECTED };

// Theme
Preferences prefs;
PermitterTheme* theme = nullptr;
int currentThemeIndex = 0;
int pickerIndex = 0;

// State
State state = STATE_IDLE;
bool wifiConnected = false;
bool bridgeConnected = false;
unsigned long lastTimeUpdate = 0;
unsigned long lastPoll = 0;
unsigned long lastStatusCheck = 0;
unsigned long confirmUntil = 0;
bool wasTouching = false;
bool lastChoiceAccepted = false;

// Long press detection
unsigned long touchStartTime = 0;
bool longPressTriggered = false;

// Current pending request
String pendingId = "";
String pendingTool = "";
String pendingAction = "";
String pendingRisk = "";

// --- Network ---

void checkBridgeStatus() {
  HTTPClient http;
  String url = String("http://") + BRIDGE_HOST + ":" + BRIDGE_PORT + "/status";
  http.begin(url);
  http.setTimeout(2000);
  int code = http.GET();
  bridgeConnected = (code == 200);
  http.end();
}

bool pollPending() {
  HTTPClient http;
  String url = String("http://") + BRIDGE_HOST + ":" + BRIDGE_PORT + "/pending";
  http.begin(url);
  http.setTimeout(2000);
  int code = http.GET();
  if (code == 200) {
    String body = http.getString();
    http.end();
    if (body == "null" || body.length() < 5) return false;
    int idx;
    idx = body.indexOf("\"id\":\"");
    if (idx >= 0) pendingId = body.substring(idx + 6, body.indexOf("\"", idx + 6));
    idx = body.indexOf("\"tool\":\"");
    if (idx >= 0) pendingTool = body.substring(idx + 8, body.indexOf("\"", idx + 8));
    idx = body.indexOf("\"action\":\"");
    if (idx >= 0) pendingAction = body.substring(idx + 10, body.indexOf("\"", idx + 10));
    idx = body.indexOf("\"risk\":\"");
    if (idx >= 0) pendingRisk = body.substring(idx + 8, body.indexOf("\"", idx + 8));
    return pendingId.length() > 0;
  }
  http.end();
  return false;
}

void sendResponse(const char* choice) {
  HTTPClient http;
  String url = String("http://") + BRIDGE_HOST + ":" + BRIDGE_PORT + "/respond";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);
  String body = String("{\"choice\":\"") + choice + "\"}";
  http.POST(body);
  http.end();
}

// --- Touch ---

TouchZone getTouchZone(int x, int y) {
  if (y < ZONE_Y_TOP) return ZONE_NONE;
  if (x <= ZONE_1_X_MAX) return ZONE_ALWAYS;
  if (x <= ZONE_2_X_MAX) return ZONE_ONCE;
  return ZONE_DENY;
}

// --- Theme picker ---

void drawThemePicker() {
  M5.Display.fillScreen(0x0000);

  M5.Display.setTextSize(2);
  M5.Display.setTextColor(0xFFFF, 0x0000);
  M5.Display.setCursor(60, 10);
  M5.Display.print("SELECT THEME");

  M5.Display.drawFastHLine(20, 32, 280, 0x7BEF);

  for (int i = 0; i < THEME_COUNT; i++) {
    int y = 45 + i * 45;
    bool selected = (i == pickerIndex);

    if (selected) {
      M5.Display.fillRect(10, y, 300, 38, 0x2104);
      M5.Display.drawRect(10, y, 300, 38, 0xFFFF);
    } else {
      M5.Display.drawRect(10, y, 300, 38, 0x4208);
    }

    M5.Display.setTextSize(2);
    M5.Display.setTextColor(selected ? 0xFFFF : 0x7BEF, selected ? 0x2104 : 0x0000);
    M5.Display.setCursor(20, y + 10);
    M5.Display.print(THEME_LABELS[i]);

    if (selected) {
      M5.Display.setTextSize(1);
      M5.Display.setCursor(260, y + 14);
      M5.Display.print("[SET]");
    }
  }

  // Instructions
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(0x7BEF, 0x0000);
  M5.Display.setCursor(30, 195);
  M5.Display.print("Tap theme to select. Top area = back.");
  M5.Display.setCursor(60, 215);
  M5.Display.print("Long-press idle = picker");
}

void switchTheme(int index) {
  if (theme) delete theme;
  currentThemeIndex = index;
  theme = createTheme(index);
  theme->setup();

  // Save to flash
  prefs.begin("permitter", false);
  prefs.putInt("theme", index);
  prefs.end();
}

// --- Setup ---

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200);

  // Load saved theme
  prefs.begin("permitter", true);
  currentThemeIndex = prefs.getInt("theme", 0);
  prefs.end();
  theme = createTheme(currentThemeIndex);
  theme->setup();

  M5.Display.setRotation(1);

  // Boot screen
  theme->drawBoot(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    M5.Display.print(".");
    attempts++;
  }
  wifiConnected = (WiFi.status() == WL_CONNECTED);

  if (wifiConnected) {
    configTime(-4 * 3600, 0, "pool.ntp.org");
  }

  theme->drawBootStatus(wifiConnected, wifiConnected ? WiFi.localIP().toString().c_str() : "");
  delay(1000);

  state = STATE_IDLE;
  theme->drawIdle(bridgeConnected);
}

// --- Main loop ---

void loop() {
  M5.update();

  auto t = M5.Touch.getDetail();
  bool isTouching = t.isPressed();

  switch (state) {
    case STATE_IDLE: {
      // Clock
      if (millis() - lastTimeUpdate > 1000) {
        lastTimeUpdate = millis();
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
          char ts[9], ds[12];
          strftime(ts, sizeof(ts), "%H:%M:%S", &timeinfo);
          strftime(ds, sizeof(ds), "%Y-%m-%d", &timeinfo);
          theme->drawClock(ts, ds);
        }
      }

      // Bridge status
      if (wifiConnected && millis() - lastStatusCheck > 5000) {
        lastStatusCheck = millis();
        checkBridgeStatus();
      }

      // Poll pending
      if (bridgeConnected && millis() - lastPoll > 500) {
        lastPoll = millis();
        if (pollPending()) {
          state = STATE_PERMISSION;
          PermissionRequest req = { pendingTool, pendingAction, pendingRisk, pendingId };
          theme->playAlertSound();
          theme->drawPermission(req);
          wasTouching = true;
          break;
        }
      }

      // Long press detection for theme picker
      if (isTouching && !wasTouching) {
        touchStartTime = millis();
        longPressTriggered = false;
      }
      if (isTouching && !longPressTriggered && (millis() - touchStartTime > LONG_PRESS_MS)) {
        longPressTriggered = true;
        M5.Speaker.tone(500, 50);
        state = STATE_THEME_PICKER;
        pickerIndex = currentThemeIndex;
        drawThemePicker();
      }
      break;
    }

    case STATE_PERMISSION: {
      if (isTouching && !wasTouching) {
        TouchZone zone = getTouchZone(t.x, t.y);
        if (zone != ZONE_NONE) {
          const char* choice;
          switch (zone) {
            case ZONE_ALWAYS: choice = "always"; lastChoiceAccepted = true; break;
            case ZONE_ONCE:   choice = "allow";  lastChoiceAccepted = true; break;
            case ZONE_DENY:   choice = "deny";   lastChoiceAccepted = false; break;
            default:          choice = "deny";   lastChoiceAccepted = false; break;
          }
          sendResponse(choice);
          state = STATE_CONFIRM;
          confirmUntil = millis() + 1500;
          theme->playConfirmSound(lastChoiceAccepted);
          theme->drawConfirm(lastChoiceAccepted, pendingTool.c_str());
        }
      }
      break;
    }

    case STATE_CONFIRM: {
      if (millis() > confirmUntil) {
        state = STATE_IDLE;
        pendingId = "";
        theme->drawIdle(bridgeConnected);
      }
      break;
    }

    case STATE_THEME_PICKER: {
      if (isTouching && !wasTouching) {
        // Check which theme was tapped
        for (int i = 0; i < THEME_COUNT; i++) {
          int y = 45 + i * 45;
          if (t.y >= y && t.y < y + 38 && t.x >= 10 && t.x <= 310) {
            if (i == pickerIndex) {
              // Confirm selection
              switchTheme(i);
              state = STATE_IDLE;
              theme->drawIdle(bridgeConnected);
            } else {
              pickerIndex = i;
              drawThemePicker();
            }
            break;
          }
        }
        // Tap above themes = back
        if (t.y < 40) {
          state = STATE_IDLE;
          theme->drawIdle(bridgeConnected);
        }
      }
      break;
    }

    case STATE_DISCONNECTED: {
      if (millis() - lastStatusCheck > 5000) {
        lastStatusCheck = millis();
        checkBridgeStatus();
        if (bridgeConnected) {
          state = STATE_IDLE;
          theme->drawIdle(bridgeConnected);
        }
      }
      break;
    }
  }

  wasTouching = isTouching;
  delay(20);
}
