/**
 * audio_test.h — 音频硬件诊断工具
 * 在 OLED 上直接测试 INMP441 麦克风 + MAX98357A 功放, 不依赖串口
 *
 * 用法:
 *   audioTestMic()   — 录音 3 秒, OLED 显示实时 VU 表 + 最终诊断
 *   audioTestSpk()   — 播放 440Hz 正弦波 1 秒, OLED 显示状态
 *   需要先调用 audioTestInit() 设置 AMP_SD 引脚
 */

#ifndef AUDIO_TEST_H
#define AUDIO_TEST_H

#include <driver/i2s.h>
#include "../config.h"

/** 初始化功放控制引脚 (开机时调用一次) */
void audioTestInit();

/** 麦克风录音诊断: 录音 3 秒, OLED 显示实时 VU 柱 + 统计结果 */
void audioTestMic();

/** 功放播放诊断: 播放 440Hz 1 秒, OLED 显示播放状态 */
void audioTestSpk();

#endif
