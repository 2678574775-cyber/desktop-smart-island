/**
 * mic_hal.cpp — 麦克风录音硬件抽象层实现 (INMP441 + I2S RX)
 * 包含 24bit→16bit 符号位扩展转换, 自动超时停止
 */

#include "mic_hal.h"
#include "i2s_hal.h"

// ---- 内部状态 ----
static bool    s_recording   = false;        // 录音进行中
static int     s_dataLen     = 0;            // 已采集样本数 (16bit 单声道)
static int16_t* s_buffer     = NULL;         // PCM 缓冲区

// ================================================================
//  公开 API
// ================================================================

void micStartRecord() {
  // 分配录音缓冲区 (96KB: 16kHz × 2byte × 3秒)
  if (s_buffer == NULL) {
    s_buffer = (int16_t*)malloc(RECORD_BUF_SIZE);
  }
  if (s_buffer == NULL) {
    Serial.println(F("[MIC] malloc failed!"));
    return;
  }

  s_dataLen   = 0;
  s_recording = true;

  // 打开功放 (让 MAX98357A 保持静默, 避免录音时自激啸叫)
  digitalWrite(AMP_SD_PIN, LOW);

  i2sInitRecord();
  Serial.println(F("[MIC] recording started"));
}

void micFeedRecord() {
  if (!s_recording || s_buffer == NULL) return;

  // 分块读取 I2S 原始数据
  // I2S 配置: 32bit per sample, 所以每次读 128×4 = 512B
  size_t bytesRead = 0;
  int32_t raw[128];

  esp_err_t err = i2s_read(I2S_NUM_0, raw, sizeof(raw), &bytesRead, portMAX_DELAY);
  if (err != ESP_OK || bytesRead == 0) return;

  // INMP441 24bit 数据在 32bit 帧的高 24 位 → 右移 8 位得 16bit PCM
  int samples = bytesRead / 4;
  for (int i = 0; i < samples; i++) {
    int16_t pcm16 = (int16_t)(raw[i] >> 16);  // 24bit MSB→16bit: take top 16 bits

    if (s_dataLen < (int)(RECORD_BUF_SIZE / 2)) {
      s_buffer[s_dataLen] = pcm16;
      s_dataLen++;
    }
  }

  // 录音时长自动停止 (3 秒)
  if (s_dataLen >= (int)(SAMPLE_RATE * RECORD_MAX_SEC)) {
    micStopRecord();
    Serial.println(F("[MIC] auto-stopped (3s limit)"));
  }
}

void micStopRecord() {
  if (!s_recording) return;

  s_recording = false;
  i2sDeinit();
  Serial.print(F("[MIC] stopped, samples: "));
  Serial.println(s_dataLen);
}

bool micIsRecording() {
  return s_recording;
}

int16_t* micGetData() {
  return s_buffer;
}

int micGetSampleCount() {
  return s_dataLen;
}
