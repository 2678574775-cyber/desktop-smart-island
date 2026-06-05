/**
 * buzzer_hal.cpp — LED / 蜂鸣器 反馈控制实现
 *
 * 架构: 非阻塞时间驱动状态机。
 *   feedbackClick() / feedbackFlash() 只发起操作、记录时间戳，
 *   feedbackTick() 由主循环每帧调用，驱动 LED 开关与序列推进。
 *
 * 状态机:
 *   phase=0 idle    → 无活动
 *   phase=1 click   → LED 点亮中, 计时到 → LOW, 回 idle
 *   phase=2 flashOn → LED 点亮中, 计时到 → LOW, 进 flashOff
 *   phase=3 flashOff→ LED 熄灭中, 计时到 → 剩余 >0 ? flashOn : idle
 */

#include "buzzer_hal.h"

// ---- 非阻塞状态 -----
static int  g_phase    = 0;   // 0=idle, 1=click, 2=flashOn, 3=flashOff
static unsigned long g_timer    = 0;
static int  g_flashRemain = 0;
static int  g_flashOnMs   = 0;
static int  g_flashOffMs  = 0;

static const unsigned long CLICK_MS = 5;

// ---- 发起操作 ----

void feedbackClick() {
  if (LED_PIN < 0) return;  // 硬件未焊接时跳过
  digitalWrite(LED_PIN, HIGH);
  g_phase = 1;
  g_timer = millis();
}

void feedbackFlash(int count, int onMs, int offMs) {
  if (LED_PIN < 0) return;  // 硬件未焊接时跳过
  g_flashRemain = count;
  g_flashOnMs   = onMs;
  g_flashOffMs  = offMs;
  digitalWrite(LED_PIN, HIGH);
  g_phase = 2;
  g_timer = millis();
}

// ---- 每帧驱动 ----

void feedbackTick() {
  if (LED_PIN < 0) return;  // 硬件未焊接时跳过
  unsigned long now = millis();

  switch (g_phase) {

    case 1:  // click 脉冲
      if (now - g_timer >= CLICK_MS) {
        digitalWrite(LED_PIN, LOW);
        g_phase = 0;
      }
      break;

    case 2:  // flash ON 阶段
      if (now - g_timer >= (unsigned long)g_flashOnMs) {
        digitalWrite(LED_PIN, LOW);
        g_flashRemain--;
        if (g_flashRemain <= 0) {
          g_phase = 0;
        } else {
          g_phase = 3;
          g_timer = now;
        }
      }
      break;

    case 3:  // flash OFF 阶段
      if (now - g_timer >= (unsigned long)g_flashOffMs) {
        digitalWrite(LED_PIN, HIGH);
        g_phase = 2;
        g_timer = now;
      }
      break;

    default: break;
  }
}
