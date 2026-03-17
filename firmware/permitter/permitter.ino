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

// Theme & settings
Preferences prefs;
PermitterTheme* theme = nullptr;
int currentThemeIndex = 0;
int pickerIndex = 0;
bool use24Hour = true;

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
bool pendingDual = false;

// Claude status from bridge
int activeAgents = 0;
String claudeState = "idle";  // "idle", "working", "waiting"
String lastCompletedTool = "";
int sessionUptime = 0;

// --- Network ---

void checkBridgeStatus() {
  HTTPClient http;
  String url = String("http://") + BRIDGE_HOST + ":" + BRIDGE_PORT + "/status";
  http.begin(url);
  http.setTimeout(2000);
  int code = http.GET();
  bridgeConnected = (code == 200);
  if (code == 200) {
    String body = http.getString();
    int idx;
    idx = body.indexOf("\"agents\":");
    if (idx >= 0) {
      int end = body.indexOf(",", idx + 9);
      if (end > idx) activeAgents = body.substring(idx + 9, end).toInt();
    }
    idx = body.indexOf("\"state\":\"");
    if (idx >= 0) claudeState = body.substring(idx + 9, body.indexOf("\"", idx + 9));
    idx = body.indexOf("\"uptime\":");
    if (idx >= 0) {
      int end = body.indexOf("}", idx + 9);
      int comma = body.indexOf(",", idx + 9);
      if (comma > 0 && comma < end) end = comma;
      if (end > idx) sessionUptime = body.substring(idx + 9, end).toInt();
    }
  }
  http.end();
}

// Returns: 0 = nothing, 1 = permission needed, 2 = activity flash
int pollPending() {
  HTTPClient http;
  String url = String("http://") + BRIDGE_HOST + ":" + BRIDGE_PORT + "/pending";
  http.begin(url);
  http.setTimeout(2000);
  int code = http.GET();
  if (code == 200) {
    String body = http.getString();
    http.end();
    if (body == "null" || body.length() < 5) return 0;

    // Parse type
    int typeIdx = body.indexOf("\"type\":\"");
    String type = "";
    if (typeIdx >= 0) type = body.substring(typeIdx + 8, body.indexOf("\"", typeIdx + 8));

    // Parse common fields
    int idx;
    idx = body.indexOf("\"tool\":\"");
    if (idx >= 0) pendingTool = body.substring(idx + 8, body.indexOf("\"", idx + 8));
    idx = body.indexOf("\"action\":\"");
    if (idx >= 0) pendingAction = body.substring(idx + 10, body.indexOf("\"", idx + 10));
    idx = body.indexOf("\"risk\":\"");
    if (idx >= 0) pendingRisk = body.substring(idx + 8, body.indexOf("\"", idx + 8));

    if (type == "activity") return 2;

    idx = body.indexOf("\"id\":\"");
    if (idx >= 0) pendingId = body.substring(idx + 6, body.indexOf("\"", idx + 6));
    idx = body.indexOf("\"dual\":\"");
    pendingDual = (idx >= 0 && body.substring(idx + 8, idx + 9) == "1");
    return pendingId.length() > 0 ? 1 : 0;
  }
  http.end();
  return 0;
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
  M5.Display.setCursor(80, 10);
  M5.Display.print("SETTINGS");

  M5.Display.drawFastHLine(20, 32, 280, 0x7BEF);

  // Theme rows
  for (int i = 0; i < THEME_COUNT; i++) {
    int y = 40 + i * 36;
    bool selected = (i == pickerIndex);

    if (selected) {
      M5.Display.fillRect(10, y, 300, 32, 0x2104);
      M5.Display.drawRect(10, y, 300, 32, 0xFFFF);
    } else {
      M5.Display.drawRect(10, y, 300, 32, 0x4208);
    }

    M5.Display.setTextSize(2);
    M5.Display.setTextColor(selected ? 0xFFFF : 0x7BEF, selected ? 0x2104 : 0x0000);
    M5.Display.setCursor(20, y + 8);
    M5.Display.print(THEME_LABELS[i]);

    if (selected) {
      M5.Display.setTextSize(1);
      M5.Display.setCursor(260, y + 12);
      M5.Display.print("[SET]");
    }
  }

  // Divider
  int divY = 40 + THEME_COUNT * 36 + 4;
  M5.Display.drawFastHLine(20, divY, 280, 0x7BEF);

  // Clock format toggle
  int clockY = divY + 8;
  M5.Display.fillRect(10, clockY, 300, 32, 0x0000);
  M5.Display.drawRect(10, clockY, 300, 32, 0x4208);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(0x7BEF, 0x0000);
  M5.Display.setCursor(20, clockY + 8);
  M5.Display.printf("Clock: %s", use24Hour ? "24H" : "12H");
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(0x4208, 0x0000);
  M5.Display.setCursor(230, clockY + 12);
  M5.Display.print("[TOGGLE]");

  // Instructions
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(0x7BEF, 0x0000);
  M5.Display.setCursor(50, 215);
  M5.Display.print("Tap to select. Top area = back.");
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

  // Load saved settings
  prefs.begin("permitter", true);
  currentThemeIndex = prefs.getInt("theme", 0);
  use24Hour = prefs.getBool("24hour", true);
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
          char ts[12], ds[12];
          strftime(ts, sizeof(ts), use24Hour ? "%H:%M:%S" : "%I:%M %p", &timeinfo);
          strftime(ds, sizeof(ds), "%Y-%m-%d", &timeinfo);
          theme->drawClock(ts, ds);
        }
      }

      // Bridge status
      if (wifiConnected && millis() - lastStatusCheck > 5000) {
        lastStatusCheck = millis();
        checkBridgeStatus();
        theme->drawStatusBar(claudeState.c_str(), activeAgents, sessionUptime);
      }

      // Poll pending
      if (bridgeConnected && millis() - lastPoll > 500) {
        lastPoll = millis();
        int result = pollPending();
        if (result == 1) {
          // Permission needed
          state = STATE_PERMISSION;
          PermissionRequest req = { pendingTool, pendingAction, pendingRisk, pendingId, pendingDual };
          theme->playAlertSound();
          theme->drawPermission(req);
          wasTouching = true;
          break;
        } else if (result == 2) {
          // Activity flash — trusted tool auto-approved
          theme->drawActivity(pendingTool.c_str(), pendingAction.c_str());
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
          int y = 40 + i * 36;
          if (t.y >= y && t.y < y + 32 && t.x >= 10 && t.x <= 310) {
            if (i == pickerIndex) {
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
        // Clock format toggle
        int clockY = 40 + THEME_COUNT * 36 + 12;
        if (t.y >= clockY && t.y < clockY + 32 && t.x >= 10 && t.x <= 310) {
          use24Hour = !use24Hour;
          prefs.begin("permitter", false);
          prefs.putBool("24hour", use24Hour);
          prefs.end();
          M5.Speaker.tone(500, 50);
          drawThemePicker();
        }
        // Tap above items = back
        if (t.y < 35) {
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
