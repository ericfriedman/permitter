#ifndef THEME_REGISTRY_H
#define THEME_REGISTRY_H

#include "themes/theme_terminal.h"
#include "themes/theme_skeuo.h"
#include "themes/theme_brutalist.h"

const int THEME_COUNT = 3;
const char* THEME_NAMES[] = { "terminal", "skeuo", "brutalist" };
const char* THEME_LABELS[] = { "Terminal", "Skeuo", "Brutalist" };

PermitterTheme* createTheme(int index) {
  switch (index) {
    case 0: return new TerminalTheme();
    case 1: return new SkeuoTheme();
    case 2: return new BrutalistTheme();
    default: return new TerminalTheme();
  }
}

PermitterTheme* createThemeByName(const char* name) {
  for (int i = 0; i < THEME_COUNT; i++) {
    if (strcmp(name, THEME_NAMES[i]) == 0) return createTheme(i);
  }
  return createTheme(0);
}

int getThemeIndex(const char* name) {
  for (int i = 0; i < THEME_COUNT; i++) {
    if (strcmp(name, THEME_NAMES[i]) == 0) return i;
  }
  return 0;
}

#endif
