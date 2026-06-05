/**
 * Clock 模式 (per README.md)
 *   B: 跳转 TODO
 *   C: 任务弹窗 (预留)
 *   双击D: 切换 Weather
 */

#include "mode_clock.h"
#include <time.h>
#include "../hardware/display_hal.h"
#include "../hardware/buzzer_hal.h"

void renderClockMode() {
  display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, SSD1306_BLACK);
  display.setTextColor(SSD1306_WHITE);

  int yBig = CONTENT_Y + 4;
  display.setTextSize(2);
  const int cw = 12, n = 5;
  int gap = (SCREEN_W - n * cw) / (n + 1); if (gap < 2) gap = 2;
  int x = gap;
  const char* src = (timeSynced && timeStr[0] != '-') ? timeStr : "--:--";
  bool cv = true;
  if (timeSynced) { struct tm t; if (getLocalTime(&t)) cv = (t.tm_sec % 2 == 0); }
  for (int i = 0; i < n; i++) {
    display.setCursor(x, yBig); display.print((i == 2 && !cv) ? ' ' : src[i]); x += cw + gap;
  }
  display.setTextSize(1);

  int yd = CONTENT_Y + 27;
  if (timeSynced && timeStr[0] != '-') {
    struct tm t;
    if (getLocalTime(&t)) {
      const char* wd[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
      char b[24]; snprintf(b, sizeof(b), "%04d-%02d-%02d %s",
        t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, wd[t.tm_wday]);
      printCentred(b, yd);
    }
  } else { printCentred("NTP syncing...", yd); }

  display.drawFastHLine(2, CONTENT_Y + 36, SCREEN_W - 4, SSD1306_WHITE);
  int yi = CONTENT_Y + 40;
  const char *s1="Sun 26C", *s2="TODO:3";
  int w1 = strlen(s1)*6, tot = w1+6+2+(int)strlen(s2)*6, sx = max(0, (SCREEN_W-tot)/2);
  display.setCursor(sx, yi); display.print(s1);
  display.setCursor(sx+w1+2, yi); display.print('|');
  display.setCursor(sx+w1+8, yi); display.print(s2);
  display.setCursor(2, CONTENT_Y + 48);
  display.print(F("B:TODO  C:Tasks"));
}

void handleClockButtons() {
  if (buttons[1].pressed) { enterMode(MODE_TODO); feedbackClick(); }
  if (buttons[2].pressed) { feedbackClick(); }
}
