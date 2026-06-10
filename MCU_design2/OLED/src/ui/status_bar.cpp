/**
 * status_bar.cpp — 状态栏渲染实现
 */

#include "status_bar.h"
#include "icons.h"
#include "../hardware/display_hal.h"

// ---- 绘制状态栏 ----
void drawStatusBar() {
  display.fillRect(0, 0, SCREEN_W, STATUSBAR_H, SSD1306_BLACK);

  // 模式图标 (12x8)
  const uint8_t* modeBmp = (const uint8_t*)pgm_read_ptr(&mode_icon_table[currentModeIcon]);
  drawIcon(MODE_ICON_X, ICON_Y, modeBmp, 12, 8);

  // 时间文字 "HH:MM"
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  if (statusBarTag[0] != '\0') {
    display.setCursor(14, 1);
    display.print(statusBarTag);
    display.setCursor(40, 1);
  } else {
    display.setCursor(36, 1);
  }
  display.print(timeStr);

  // WiFi 图标
  const uint8_t* wifiBmp = (const uint8_t*)pgm_read_ptr(&wifi_icon_table[currentWifi]);
  drawIcon(WIFI_ICON_X, ICON_Y, wifiBmp, 12, 8);

  // 电池图标
  const uint8_t* battBmp = (const uint8_t*)pgm_read_ptr(&batt_icon_table[currentBatt]);
  drawIcon(BATT_ICON_X, ICON_Y, battBmp, 12, 8);

  // 分隔线
  display.drawFastHLine(0, STATUSBAR_H - 1, SCREEN_W, SSD1306_WHITE);
}

// ---- 更新状态栏标签 ----
void updateStatusBarTag() {
  statusBarTag[0] = '\0';
  if (currentMode == MODE_COUNTDOWN) {
    switch (cdState) {
      case CD_RUNNING: strcpy(statusBarTag, cdIsRest ? "RST" : "RUN"); break;
      case CD_PAUSED:  strcpy(statusBarTag, cdIsRest ? "RPA" : "PAU"); break;
      case CD_DONE:    strcpy(statusBarTag, "DON"); break;
      default: break;
    }
  }
}

// ---- 更新模式图标 ----
void updateModeIcon() {
  switch (currentMode) {
    case MODE_CLOCK:      currentModeIcon = ICON_MODE_CLOCK;      break;
    case MODE_COUNTDOWN:  currentModeIcon = ICON_MODE_COUNTDOWN;  break;
    case MODE_CHAT_ROBOT: currentModeIcon = ICON_MODE_CHAT_ROBOT; break;
    case MODE_TODO:       currentModeIcon = ICON_MODE_TODO;       break;
    case MODE_WEATHER:    currentModeIcon = ICON_MODE_WEATHER;    break;
    case MODE_SETTINGS:   currentModeIcon = ICON_MODE_SETTINGS;   break;
  }
}
