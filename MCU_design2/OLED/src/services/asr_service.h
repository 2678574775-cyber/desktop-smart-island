/**
 * asr_service.h — 智谱语音识别 (GLM-ASR-2512)
 *
 * API: POST https://open.bigmodel.cn/api/paas/v4/audio/transcriptions
 *   参数: model=glm-asr-2512, file_base64=<WAV的base64>
 *
 * 采样率: 16000 Hz (与 mic_hal 一致)
 */

#ifndef ASR_SERVICE_H
#define ASR_SERVICE_H

#include <Arduino.h>

/** 初始化 (目前只是 WiFi 状态检查, 无需单独获取 token) */
bool asrInit();

/**
 * PCM → WAV → base64 → Zhipu ASR → 文字
 * @param pcmData  16bit PCM 采样 (来自 mic_hal)
 * @param pcmLen   采样数
 * @return 识别文本, 失败返回 ""
 */
String asrRecognize(const int16_t* pcmData, int pcmLen);

#endif
