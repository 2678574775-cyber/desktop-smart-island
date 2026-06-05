/**
 * animation.h — 开机动画 & 模式切换动画
 */

#ifndef ANIMATION_H
#define ANIMATION_H

#include "../config.h"

/** 播放开机动画 (frame3 slide-in → frame5 slide-in → 保持 → slide-out) */
void playAnimation();

/** 模式切换滑动动画 (预留) */
void transitSlide(int direction);

#endif // ANIMATION_H
