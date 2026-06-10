/**
 * Settings 模式 (per README.md)
 *   A: 下一个   B: 上一个   C: 选择
 */

#include "mode_settings.h"
#include "../hardware/display_hal.h"
#include "../hardware/buzzer_hal.h"
#include "../services/wifi_service.h"
#include <Preferences.h>

void renderSettingsMode() {
  display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  const char* sl[] = {"15s","30s","60s","120s","Off"};
  const char* al[] = {"Cloud(DS)","Local(qwen)","Auto"};

  if (settingsSub == SETTINGS_MAIN) {
    display.setCursor(2, CONTENT_Y); display.print(F("Settings"));
    display.drawFastHLine(2, CONTENT_Y + 8, SCREEN_W - 4, SSD1306_WHITE);
    const char* items[] = {"WiFi Setup","Brightness","Volume","Sound/LED","Sleep Timeout","AI Mode","Device Info"};
    int vs = (settingsCursor >= 5) ? settingsCursor - 4 : 0;
    for (int i = vs; i < SETTINGS_ITEMS && i < vs + 5; i++)
      printLine(CONTENT_Y + 10 + (i - vs) * 8, items[i], i == settingsCursor);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(2, CONTENT_Y + 50); display.print(F("A:Next  B:Prev  C:Enter"));
  }
  else if (settingsSub == SETTINGS_BRIGHTNESS) {
    display.setCursor(2, CONTENT_Y + 1); display.print(F("Brightness"));
    display.drawFastHLine(2, CONTENT_Y + 9, SCREEN_W - 4, SSD1306_WHITE);
    char b[20]; snprintf(b, sizeof(b), "%d%%", settingsBrightness);
    display.setTextSize(2); printCentred(b, CONTENT_Y + 15); display.setTextSize(1);
    int bw = map(settingsBrightness, 0, 100, 0, 116);
    display.drawRect(4, CONTENT_Y + 33, 120, 6, SSD1306_WHITE);
    if (bw > 0) display.fillRect(5, CONTENT_Y + 34, bw, 4, SSD1306_WHITE);
    display.setCursor(2, CONTENT_Y + 46); display.print(F("A:Next  B:Prev  C:Save"));
  }
  else if (settingsSub == SETTINGS_SLEEP) {
    display.setCursor(2, CONTENT_Y + 1); display.print(F("Sleep Timeout"));
    display.drawFastHLine(2, CONTENT_Y + 9, SCREEN_W - 4, SSD1306_WHITE);
    for (int i = 0; i < 5; i++) {
      char r[20]; snprintf(r, sizeof(r), "%s %s", i == settingsSleepIdx ? ">" : " ", sl[i]);
      printLine(CONTENT_Y + 12 + i * 8, r, i == settingsSleepIdx);
    }
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(2, CONTENT_Y + 50); display.print(F("A:Next  B:Prev  C:Save"));
  }
  else if (settingsSub == SETTINGS_AI) {
    display.setCursor(2, CONTENT_Y + 1); display.print(F("AI Mode"));
    display.drawFastHLine(2, CONTENT_Y + 9, SCREEN_W - 4, SSD1306_WHITE);
    for (int i = 0; i < 3; i++) {
      char r[22]; snprintf(r, sizeof(r), "%s %s", i == settingsAiIdx ? ">" : " ", al[i]);
      printLine(CONTENT_Y + 12 + i * 10, r, i == settingsAiIdx);
    }
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(2, CONTENT_Y + 50); display.print(F("A:Next  B:Prev  C:Save"));
  }
}

void handleSettingsButtons() {
  Button& bA = buttons[0];
  Button& bB = buttons[1];
  Button& bC = buttons[2];

  if (settingsSub == SETTINGS_MAIN) {
    // A: 下一个
    if (bA.pressed) { settingsCursor = (settingsCursor + 1) % SETTINGS_ITEMS;
      lastUserAction = millis(); feedbackClick(); }
    // B: 上一个
    if (bB.pressed) { settingsCursor = (settingsCursor + SETTINGS_ITEMS - 1) % SETTINGS_ITEMS;
      lastUserAction = millis(); feedbackClick(); }
    // C: 选择/进入
    if (bC.pressed) {
      switch (settingsCursor) {
        case 0: wifiSetup(); break;
        case 1: settingsSub = SETTINGS_BRIGHTNESS; break;
        case 4: settingsSub = SETTINGS_SLEEP; break;
        case 5: settingsSub = SETTINGS_AI; break;
        default: break;
      }
      lastUserAction = millis(); feedbackClick();
    }
  }
  else if (settingsSub == SETTINGS_BRIGHTNESS) {
    if (bA.pressed) { settingsBrightness += 10; if (settingsBrightness > 100) settingsBrightness = 0;
      lastUserAction = millis(); feedbackClick(); }
    if (bB.pressed) { settingsBrightness -= 10; if (settingsBrightness < 0) settingsBrightness = 100;
      lastUserAction = millis(); feedbackClick(); }
    if (bC.pressed) { display.ssd1306_command(0x81);
      display.ssd1306_command((uint8_t)map(settingsBrightness, 0, 100, 0, 255));
      settingsSub = SETTINGS_MAIN; lastUserAction = millis(); feedbackClick(); }
  }
  else if (settingsSub == SETTINGS_SLEEP) {
    if (bA.pressed) { settingsSleepIdx = (settingsSleepIdx + 1) % 5;
      lastUserAction = millis(); feedbackClick(); }
    if (bB.pressed) { settingsSleepIdx = (settingsSleepIdx + 4) % 5;
      lastUserAction = millis(); feedbackClick(); }
    if (bC.pressed) {
      // 将选中值写入全局熄屏超时，并持久化到 NVS
      const int sleepMs[] = {15000, 30000, 60000, 120000, 0};
      screenSleepMs = sleepMs[settingsSleepIdx];
      {
        Preferences prefs;
        prefs.begin("desktopdog", false);
        prefs.putInt("sleep_idx", settingsSleepIdx);
        prefs.end();
      }
      settingsSub = SETTINGS_MAIN; lastUserAction = millis(); feedbackClick();
    }
  }
  else if (settingsSub == SETTINGS_AI) {
    if (bA.pressed) { settingsAiIdx = (settingsAiIdx + 1) % 3;
      lastUserAction = millis(); feedbackClick(); }
    if (bB.pressed) { settingsAiIdx = (settingsAiIdx + 2) % 3;
      lastUserAction = millis(); feedbackClick(); }
    if (bC.pressed) { settingsSub = SETTINGS_MAIN; lastUserAction = millis(); feedbackClick(); }
  }
}
