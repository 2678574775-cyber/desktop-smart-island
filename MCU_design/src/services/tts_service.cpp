/**
 * tts_service.cpp — 智谱 glm-tts 语音合成 + I2S 播放
 * 24000 Hz WAV → 跳过 44B header → 16bit PCM → I2S TX @ 24kHz
 */

#include "tts_service.h"
#include "../config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "../hardware/i2s_hal.h"

void ttsPlay(const String& text) {
  if (text.length() == 0) return;

  // URL 编码 (仅编码非 ASCII, 用于 JSON 中)
  String escaped = text;
  escaped.replace("\\", "\\\\");
  escaped.replace("\"", "\\\"");

  String body = "{\"model\":\"glm-tts\",\"input\":\""
              + escaped
              + "\",\"voice\":\"tongtong\",\"response_format\":\"wav\"}";

  Serial.printf("[TTS] requesting %d chars...\n", text.length());

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15);

  if (!client.connect("open.bigmodel.cn", 443)) {
    Serial.println(F("[TTS] connect failed"));
    return;
  }

  client.print("POST /api/paas/v4/audio/speech HTTP/1.1\r\n");
  client.print("Host: open.bigmodel.cn\r\n");
  client.print("Authorization: Bearer " ZHIPU_API_KEY "\r\n");
  client.print("Content-Type: application/json\r\n");
  client.printf("Content-Length: %d\r\n", body.length());
  client.print("\r\n");
  client.print(body);
  client.flush();

  // ── 读取 HTTP 响应头 ──
  String headers;
  unsigned long t0 = millis();
  while (client.connected() && millis() - t0 < 8000) {
    if (client.available()) {
      char c = client.read();
      headers += c;
      if (headers.endsWith("\r\n\r\n")) break;  // header 结束
    }
  }

  // 检查 HTTP 状态码
  if (headers.indexOf("200 OK") < 0) {
    // 可能是 JSON 错误
    while (client.available()) { Serial.write(client.read()); }
    Serial.println();
    client.stop();
    return;
  }

  // ── 跳过 WAV 头部 (44 字节: RIFF+fmt+data) ──
  for (int i = 0; i < 44 && client.connected(); i++) {
    while (!client.available()) { delay(1); }
    client.read();
  }

  // ── I2S 播放 PCM 数据 (24000 Hz) ──
  i2sInitPlayAt(TTS_SAMPLE_RATE);
  digitalWrite(AMP_SD_PIN, HIGH);
  delay(5);

  size_t total = 0;
  int16_t buf[512];
  unsigned long lastData = millis();

  while (millis() - lastData < 2000) {  // 2 秒无数据则结束
    int avail = client.available();
    if (avail <= 0) {
      delay(5);
      continue;
    }
    lastData = millis();

    if (avail > (int)sizeof(buf)) avail = sizeof(buf);
    int rd = client.read((uint8_t*)buf, avail);
    if (rd <= 0) break;

    size_t wr = 0;
    i2s_write(I2S_NUM_0, buf, rd, &wr, portMAX_DELAY);
    total += rd;
  }

  delay(80);  // 等 DMA 排空

  client.stop();
  digitalWrite(AMP_SD_PIN, LOW);
  i2sInitRecord();  // 切回录音模式

  Serial.printf("[TTS] done, %u bytes played\n", total);
}
