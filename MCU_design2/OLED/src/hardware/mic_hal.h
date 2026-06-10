/**
 * mic_hal.h — 麦克风录音硬件抽象层 (INMP441)
 * 基于 I2S RX 模式, 负责 24bit → 16bit PCM 转换和录音缓冲管理
 *
 * 使用方式:
 *   micStartRecord()   — 开始录音, 初始化 I2S RX
 *   micFeedRecord()    — 主循环每帧调用, 分块读取 I2S 数据
 *   micStopRecord()    — 停止录音, 关闭 I2S
 *   micIsRecording()   — 查询录音状态
 *
 * 音频参数 (per config.h):
 *   采样率: 16000 Hz (百度 ASR 标准)
 *   位深: 16bit PCM (从 INMP441 原生 24bit 下转换)
 *   声道: 单声道 (L/R 接 GND)
 *   最长录音: 3 秒 (ESP32-C3 内存限制)
 */

#ifndef MIC_HAL_H
#define MIC_HAL_H

#include <Arduino.h>
#include "../config.h"

// ================================================================
//  录音控制
// ================================================================

/** 开始录音: 分配缓冲区, 初始化 I2S RX 模式 */
void micStartRecord();

/** 每帧调用: 用 i2s_read() 分块读取, 24→16bit 转换, 自动 3 秒停止 */
void micFeedRecord();

/** 强制停止录音 (不清除已采集数据) */
void micStopRecord();

/** 录音是否进行中 */
bool micIsRecording();

// ================================================================
//  录音数据访问
// ================================================================

/** 获取录音后的 16bit PCM 原始数据指针 (仅供 ASR 服务读取) */
int16_t* micGetData();

/** 获取已采集的采样数 */
int micGetSampleCount();

#endif // MIC_HAL_H
