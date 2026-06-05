/**
 * Weather 模式 (per README.md)
 *   A: 滚动预报 (循环)
 *   B: 回今日
 *   C: 切换今日/预报
 *   双击D: 切换 Clock
 */

#include "mode_weather.h"
#include "../hardware/display_hal.h"
#include "../hardware/buzzer_hal.h"

void renderWeatherMode() {
  display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (wxPage == WX_TODAY) {
    printCentred("Wuhan", CONTENT_Y + 1);
    display.setTextSize(2); printCentred("26 C", CONTENT_Y + 10); display.setTextSize(1);
    printCentred("Sunny", CONTENT_Y + 27);
    printCentred("Hum:65%  Wind:N3", CONTENT_Y + 36);
    printCentred("Feels: 28C", CONTENT_Y + 45);
    display.setCursor(2, CONTENT_Y + 53); display.print(F("C:Forecast"));
  } else {
    display.setCursor(2, CONTENT_Y + 1); display.print(F("3-Day Forecast"));
    display.drawFastHLine(2, CONTENT_Y + 9, SCREEN_W - 4, SSD1306_WHITE);
    const char* ds[] = {"Today","Tomorrow","Sat"};
    const char* ws[] = {"Sunny","Cloudy","Rain"};
    const char* ts[] = {"22~28C","18~24C","15~20C"};
    for (int i = 0; i < 3; i++) {
      char row[24]; snprintf(row, sizeof(row), "%-8s %-6s %s", ds[i], ws[i], ts[i]);
      printLine(CONTENT_Y + 11 + i * 12, row, i == wxDayCursor);
    }
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(2, CONTENT_Y + 48); display.print(F("A:Day  B:Today  C:Toggle"));
  }
}

void handleWeatherButtons() {
  // C: 切换今日/预报
  if (buttons[2].pressed) { wxPage = (wxPage == WX_TODAY) ? WX_FORECAST : WX_TODAY;
    lastUserAction = millis(); feedbackClick(); }
  // B: 回今日
  if (buttons[1].pressed) { wxPage = WX_TODAY; wxDayCursor = 0;
    lastUserAction = millis(); feedbackClick(); }
  // A: 滚动预报
  if (wxPage == WX_FORECAST && buttons[0].pressed) { wxDayCursor = (wxDayCursor + 1) % 3;
    lastUserAction = millis(); feedbackClick(); }
}
