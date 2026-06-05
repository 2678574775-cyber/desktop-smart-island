/**
 * mode_countdown.h — 倒计时模式 (含番茄钟)
 */

#ifndef MODE_COUNTDOWN_H
#define MODE_COUNTDOWN_H

#include "../config.h"

void renderCountdownMode();
void handleCountdownButtons();

/** 倒计时工具函数 */
void drawCdProgressBar(int y, float pct, bool flash);
void getCdEndTime(char* buf, int bufSize, unsigned long remainMs);

#endif
