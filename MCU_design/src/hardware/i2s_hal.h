/**
 * i2s_hal.h — I2S 总线硬件抽象层
 * 管理 ESP32-C3 I2S_NUM_0 的模式切换: RX(录音, INMP441) ↔ TX(播放, MAX98357A)
 *
 * 引脚分配 (per README.md):
 *   GPIO5  = BCLK (MAX98357A + INMP441 共用)
 *   GPIO6  = LRC  (MAX98357A + INMP441 共用)
 *   GPIO7  = DIN  → MAX98357A (TX)
 *   GPIO10 = DOUT ← INMP441 (RX)
 *   GPIO20 = AMP_SD (MAX98357A 开关)
 */

#ifndef I2S_HAL_H
#define I2S_HAL_H

#include <driver/i2s.h>
#include "../config.h"

void i2sInitRecord();                     // RX 16kHz (INMP441)
void i2sInitPlay();                       // TX 16kHz (默认)
void i2sInitPlayAt(int sampleRate);       // TX 指定采样率 (TTS: 24000)
void i2sDeinit();

#endif
