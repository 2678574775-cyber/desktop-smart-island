/**
 * Countdown 模式 (per README.md)
 *   IDLE:  A短按-5min / A长按连减   B短按+5min / B长按连加   C短按开始
 *   RUN:   B短按暂停/继续   B双击取消返回
 *   DONE:  C短按再来一次
 *   双击D: 切换 TODO
 */

#include "mode_countdown.h"
#include <time.h>
#include "../hardware/display_hal.h"
#include "../hardware/buzzer_hal.h"

// 长按连加/连减辅助: 按住超过 400ms 后每 150ms 触发一次
static int  autoStepA = 0;
static int  autoStepB = 0;
static unsigned long autoNextA = 0, autoNextB = 0;

void drawCdProgressBar(int y, float pct, bool flash) {
  int bx = 4, bw = 120;
  display.drawRect(bx, y, bw, 4, SSD1306_WHITE);
  int fw = (int)((bw - 2) * constrain(pct, 0.0f, 1.0f));
  if (fw > 0 && (!flash || (millis() % 1000 < 500)))
    display.fillRect(bx + 1, y + 1, fw, 2, SSD1306_WHITE);
}

void getCdEndTime(char* buf, int sz, unsigned long remainMs) {
  struct tm ti;
  if (getLocalTime(&ti)) {
    unsigned long ts = ti.tm_hour * 3600UL + ti.tm_min * 60UL + ti.tm_sec + (remainMs / 1000UL);
    snprintf(buf, sz, "End: %02d:%02d", (ts / 3600) % 24, (ts / 60) % 60);
  } else snprintf(buf, sz, "End: --:--");
}

void renderCountdownMode() {
  display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, SSD1306_BLACK);
  display.setTextColor(SSD1306_WHITE);

  if (cdState == CD_IDLE) {
    char b[12]; snprintf(b, sizeof(b), "%02d:00", cdMinutes);
    display.setTextSize(2); printCentred(b, CONTENT_Y + 3); display.setTextSize(1);
    printCentred("Focus Timer", CONTENT_Y + 23);
    drawCdProgressBar(CONTENT_Y + 32, 0.0f, false);
    display.setCursor(4, CONTENT_Y + 42);
    display.print(F("A/B:+-5min  C:Start"));
    display.setCursor(4, CONTENT_Y + 50);
    display.print(F("Hold A/B for fast adj"));
  }
  else if (cdState == CD_RUNNING || cdState == CD_PAUSED) {
    unsigned long rm = (cdState == CD_PAUSED) ? cdPausedRemain
      : ((millis() - cdStartMs < cdTotalMs) ? cdTotalMs - (millis() - cdStartMs) : 0);
    int m = rm / 60000, s = (rm / 1000) % 60;
    char b[12]; snprintf(b, sizeof(b), "%02d:%02d", m, s);
    display.setTextSize(2); printCentred(b, CONTENT_Y + 1); display.setTextSize(1);
    printCentred(cdIsRest ? (cdState == CD_PAUSED ? "Rest Paused" : "Rest...")
                          : (cdState == CD_PAUSED ? "Paused" : "Running..."), CONTENT_Y + 20);
    float pct = (cdTotalMs > 0) ? (1.0f - (float)rm / cdTotalMs) : 0.0f;
    drawCdProgressBar(CONTENT_Y + 30, pct, rm < 60000);
    char eb[16]; getCdEndTime(eb, sizeof(eb), rm);
    display.setCursor(4, CONTENT_Y + 38); display.print(eb);
    display.setCursor(4, CONTENT_Y + 48);
    display.print(cdState == CD_RUNNING ? F("B:Pause  BB:Cancel") : F("B:Resume  BB:Cancel"));
  }
  else if (cdState == CD_DONE) {
    display.setTextSize(2); printCentred("(^_^)", CONTENT_Y + 3); display.setTextSize(1);
    printCentred("Done!", CONTENT_Y + 23);
    char b[24]; snprintf(b, sizeof(b), "%d min focused!", cdMinutes);
    printCentred(b, CONTENT_Y + 34);
    display.setCursor(4, CONTENT_Y + 46); display.print(F("C:Again"));
  }
}

void handleCountdownButtons() {
  Button& bA = buttons[0];
  Button& bB = buttons[1];
  Button& bC = buttons[2];
  unsigned long N = millis();

  if (cdState == CD_IDLE) {
    // A 短按-5min  /  长按连减
    if (bA.released && !bA.longPressActive) { if (cdMinutes > 5) cdMinutes -= 5; lastUserAction = N; feedbackClick(); }
    if (bA.raw == LOW && autoStepA) {
      if (N >= autoNextA) { autoNextA = N + 150; if (cdMinutes > 1) cdMinutes--; }
    }
    if (bA.raw == LOW && !autoStepA && bA.pressTime > 0 && N - bA.pressTime >= 400)
      { autoStepA = 1; autoNextA = N; }

    // B 短按+5min  /  长按连加
    if (bB.released && !bB.longPressActive) { if (cdMinutes < 120) cdMinutes += 5; lastUserAction = N; feedbackClick(); }
    if (bB.raw == LOW && autoStepB) {
      if (N >= autoNextB) { autoNextB = N + 150; if (cdMinutes < 120) cdMinutes++; }
    }
    if (bB.raw == LOW && !autoStepB && bB.pressTime > 0 && N - bB.pressTime >= 400)
      { autoStepB = 1; autoNextB = N; }

    // C 开始
    if (bC.released && !bC.longPressActive) {
      cdState = CD_RUNNING; cdIsRest = false; cdTotalMs = cdMinutes * 60000UL;
      cdStartMs = N; lastUserAction = N; feedbackClick();
    }
  }

  // 长按松手 → 停止连按
  if (bA.raw == HIGH) autoStepA = 0;
  if (bB.raw == HIGH) autoStepB = 0;

  if (cdState == CD_RUNNING) {
    if (bB.pressed) { cdState = CD_PAUSED; cdPausedRemain = cdTotalMs - (N - cdStartMs);
      lastUserAction = N; feedbackClick(); }
    if (bB.doubleTap) { cdState = CD_IDLE; cdPausedRemain = 0; cdIsRest = false;
      lastUserAction = N; feedbackFlash(2, 50, 80); }
  }
  else if (cdState == CD_PAUSED) {
    if (bB.pressed) { cdState = CD_RUNNING; cdTotalMs = cdPausedRemain; cdStartMs = N;
      lastUserAction = N; feedbackClick(); }
    if (bB.doubleTap) { cdState = CD_IDLE; cdPausedRemain = 0; cdIsRest = false;
      lastUserAction = N; feedbackFlash(2, 50, 80); }
  }
  else if (cdState == CD_DONE) {
    if (bC.released) { cdState = CD_IDLE; cdIsRest = false; lastUserAction = N; feedbackClick(); }
  }
}
