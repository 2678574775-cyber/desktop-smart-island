/**
 * animation.cpp — 动画系统实现
 */

#include "animation.h"
#include "icons.h"
#include "status_bar.h"
#include "../hardware/display_hal.h"
#include "../services/ntp_service.h"

// ---- 模式切换动画 ----
void transitSlide(int direction) {
  // direction: 1 = 右滑 (下一个模式), -1 = 左滑 (上一个模式)
  // 简化实现：直接清屏重绘
  // TODO: 实现基于帧缓冲的真正滑动动画
}

// ---- 开机动画 ----
void playAnimation() {
  const uint8_t* bmp3 = (const uint8_t*)pgm_read_ptr(&frames[0]);
  const uint8_t* bmp5 = (const uint8_t*)pgm_read_ptr(&frames[1]);

  // Frame 3: 滑入 → 停留 → 滑出
  for (int s = 0; s <= STEPS_ENTER; s++) {
    drawStatusBar();
    drawFullScreen(bmp3, map(s, 0, STEPS_ENTER, -SCREEN_W, 0));
    delay(DELAY_MS);
  }
  delay(250);
  for (int s = 0; s <= STEPS_EXIT; s++) {
    drawStatusBar();
    drawFullScreen(bmp3, map(s, 0, STEPS_EXIT, 0, SCREEN_W));
    delay(DELAY_MS);
  }

  // Frame 5: 滑入 → 保持 3s → 滑出
  for (int s = 0; s <= STEPS_ENTER; s++) {
    drawStatusBar();
    drawFullScreen(bmp5, map(s, 0, STEPS_ENTER, -SCREEN_W, 0));
    delay(DELAY_MS);
  }
  unsigned long holdStart = millis();
  while (millis() - holdStart < HOLD5_MS) {
    if (millis() - lastTimeUpdate >= TIME_REFRESH_MS) {
      updateTime();
      drawStatusBar();
      drawContentFrame(bmp5, 0);
      display.display();
      lastTimeUpdate = millis();
    }
    delay(100);
  }
  for (int s = 0; s <= STEPS_EXIT; s++) {
    drawStatusBar();
    drawFullScreen(bmp5, map(s, 0, STEPS_EXIT, 0, SCREEN_W));
    delay(DELAY_MS);
  }
}
