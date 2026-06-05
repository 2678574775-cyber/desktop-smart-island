/**
 * button_events.cpp — 按键事件管理 (per README.md)
 *
 * D(GPIO1): 单击=切模式  双击=配对切换(CLOCK↔Weather, TODO↔Countdown)  长按=进设置
 * A/B/C:   分发到各模式 handle
 */

#include "button_events.h"
#include "button_hal.h"
#include "buzzer_hal.h"
#include "../modes/mode_chat_robot.h"
#include "../modes/mode_clock.h"
#include "../modes/mode_countdown.h"
#include "../modes/mode_todo.h"
#include "../modes/mode_weather.h"
#include "../modes/mode_settings.h"

// D 键双击/长按时忽略松手的短按触发
static bool s_dSuppressRelease = false;

// D 双击 → 配对切换
static void toggleModePair() {
  switch (currentMode) {
    case MODE_CLOCK:     enterMode(MODE_WEATHER);   break;
    case MODE_WEATHER:   enterMode(MODE_CLOCK);     break;
    case MODE_TODO:      enterMode(MODE_COUNTDOWN); break;
    case MODE_COUNTDOWN: enterMode(MODE_TODO);      break;
    default:             enterMode(MODE_CLOCK);     break;
  }
}

static void dispatchGlobalButtons() {
  Button& bD = buttons[3];

  if (bD.doubleTap) { s_dSuppressRelease = true; toggleModePair(); feedbackClick(); return; }
  if (bD.longPress) { s_dSuppressRelease = true;
    if (currentMode != MODE_SETTINGS) { beforeSettings = currentMode; enterMode(MODE_SETTINGS); }
    feedbackClick(); return; }

  if (bD.released) {
    bool suppress = s_dSuppressRelease; s_dSuppressRelease = false;
    if (suppress) return;
    // 普通短按 → 循环模式
    if (currentMode == MODE_SETTINGS) enterMode(MODE_CHAT_ROBOT);
    else cycleMode();
    feedbackClick();
  }
}

static void dispatchModeButtons() {
  switch (currentMode) {
    case MODE_CHAT_ROBOT: handleChatRobotButtons(); break;
    case MODE_CLOCK:      handleClockButtons();      break;
    case MODE_COUNTDOWN:  handleCountdownButtons();  break;
    case MODE_TODO:       handleTodoButtons();       break;
    case MODE_WEATHER:    handleWeatherButtons();    break;
    case MODE_SETTINGS:   handleSettingsButtons();   break;
    default: break;
  }
}

bool processButtonEvents() {
  readButtons();
#if BTN_DEBUG
  debugPrintButtons();
#endif
  dispatchGlobalButtons();
  dispatchModeButtons();
  for (int i = 0; i < BTN_COUNT; i++)
    if (buttons[i].pressed || buttons[i].released || buttons[i].longPress || buttons[i].doubleTap) return true;
  return false;
}
