/**
 * buzzer_hal.h — LED / 蜂鸣器 反馈控制硬件抽象层
 */

#ifndef BUZZER_HAL_H
#define BUZZER_HAL_H

#include "../config.h"

/** 简短点击反馈：LED 点亮 5ms (非阻塞，由 feedbackTick 关闭) */
void feedbackClick();

/** LED 闪烁序列 count 次 (非阻塞，由 feedbackTick 驱动) */
void feedbackFlash(int count, int onMs, int offMs);

/** 每帧在主循环调用，驱动非阻塞 LED 状态机 */
void feedbackTick();

#endif // BUZZER_HAL_H
