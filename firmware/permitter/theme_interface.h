#ifndef THEME_INTERFACE_H
#define THEME_INTERFACE_H

#include <M5Unified.h>

struct PermissionRequest {
  String tool;
  String action;
  String risk;  // "low" | "medium" | "high"
  String id;
  bool dual;    // also needs terminal approval
};

class PermitterTheme {
public:
  virtual ~PermitterTheme() {}
  virtual const char* name() = 0;
  virtual const char* displayName() = 0;

  // Called once when theme is activated
  virtual void setup() = 0;

  // Screen renderers
  virtual void drawBoot(const char* ssid) = 0;
  virtual void drawBootStatus(bool wifiOk, const char* ip) = 0;
  virtual void drawIdle(bool bridgeConnected) = 0;
  virtual void drawClock(const char* timeStr, const char* dateStr) = 0;
  virtual void drawPermission(PermissionRequest req) = 0;
  virtual void drawConfirm(bool accepted, const char* tool) = 0;

  // Activity indicator — trusted tool auto-approved
  virtual void drawActivity(const char* tool, const char* action) = 0;

  // Status bar — Claude state, agents, uptime
  virtual void drawStatusBar(const char* state, int agents, int uptime) = 0;

  // Audio + haptics
  virtual void playAlertSound() = 0;
  virtual void playConfirmSound(bool accepted) = 0;
};

#endif
