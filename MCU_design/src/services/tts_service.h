/**
 * tts_service.h — 智谱语音合成 (glm-tts)
 * API: POST https://open.bigmodel.cn/api/paas/v4/audio/speech
 * 返回 WAV 二进制, 24000 Hz, 16bit mono
 *
 * 流程: ttsPlay(text) → POST JSON → 接收 WAV → 跳过 header → I2S 24kHz 播放
 */

#ifndef TTS_SERVICE_H
#define TTS_SERVICE_H

#include <Arduino.h>

/**
 * 文字 → 语音 → 播放 (阻塞, 播放完成返回)
 * @param text  要播放的文字, 最长 1024 字符
 */
void ttsPlay(const String& text);

#endif
