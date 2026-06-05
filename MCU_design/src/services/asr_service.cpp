/**
 * asr_service.cpp — 智谱 GLM-ASR-2512 语音识别
 * 使用原始 SSL 连接流式发送 base64, 避免巨大临时缓冲区
 *
 * 流程: PCM → (流式 WAV header + base64) → HTTPS POST → 解析 JSON
 */

#include "asr_service.h"
#include "../config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

// ---- 将 3 字节编码为 4 个 base64 字符 ----
static const char b64Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static inline void b64Encode3(uint8_t a, uint8_t b, uint8_t c, char out[4]) {
  out[0] = b64Table[a >> 2];
  out[1] = b64Table[((a & 0x03) << 4) | (b >> 4)];
  out[2] = b64Table[((b & 0x0F) << 2) | (c >> 6)];
  out[3] = b64Table[c & 0x3F];
}

/** 将缓冲区 base64 编码后直接写入 WiFiClient */
static void streamBase64(WiFiClientSecure& client, const uint8_t* data, size_t len) {
  const size_t CHUNK = 480;  // 必须是 3 的倍数, 一次发 640 个 base64 字符
  for (size_t i = 0; i < len; i += CHUNK) {
    size_t n = min(CHUNK, len - i);
    char buf[CHUNK / 3 * 4 + 1];
    size_t out = 0;
    for (size_t j = 0; j + 2 < n; j += 3) {
      b64Encode3(data[i + j], data[i + j + 1], data[i + j + 2], buf + out);
      out += 4;
    }
    // 处理末尾不足 3 字节的部分
    if (n % 3 != 0) {
      size_t rem = n - (n / 3 * 3);
      uint8_t tail[3] = {0, 0, 0};
      for (size_t j = 0; j < rem; j++) tail[j] = data[i + n / 3 * 3 + j];
      b64Encode3(tail[0], tail[1], tail[2], buf + out);
      out += 4;
      // 替换尾部 padding: 1 字节剩余 → 最后 2 个 '=', 2 字节剩余 → 最后 1 个 '='
      if (rem == 1) buf[out - 2] = '=';
      buf[out - 1] = '=';
    }
    buf[out] = '\0';
    client.print(buf);
  }
}

/** 构建 44 字节 WAV 头并写入 client */
static void writeWavHeader(WiFiClientSecure& client, int dataBytes) {
  uint8_t hdr[44] = {0};
  int fileSize = dataBytes + 36;

  hdr[0] = 'R'; hdr[1] = 'I'; hdr[2] = 'F'; hdr[3] = 'F';
  hdr[4] = fileSize & 0xFF; hdr[5] = (fileSize >> 8) & 0xFF;
  hdr[6] = (fileSize >> 16) & 0xFF; hdr[7] = (fileSize >> 24) & 0xFF;
  hdr[8] = 'W'; hdr[9] = 'A'; hdr[10] = 'V'; hdr[11] = 'E';
  hdr[12] = 'f'; hdr[13] = 'm'; hdr[14] = 't'; hdr[15] = ' ';
  hdr[16] = 16;  // PCM subchunk size
  hdr[20] = 1;   // PCM format = 1
  hdr[22] = 1;   // mono
  hdr[24] = SAMPLE_RATE & 0xFF; hdr[25] = (SAMPLE_RATE >> 8) & 0xFF;
  hdr[28] = (SAMPLE_RATE * 2) & 0xFF; hdr[29] = ((SAMPLE_RATE * 2) >> 8) & 0xFF;
  hdr[32] = 2;   // block align (16bit mono = 2)
  hdr[34] = 16;  // bits per sample
  hdr[36] = 'd'; hdr[37] = 'a'; hdr[38] = 't'; hdr[39] = 'a';
  hdr[40] = dataBytes & 0xFF; hdr[41] = (dataBytes >> 8) & 0xFF;
  hdr[42] = (dataBytes >> 16) & 0xFF; hdr[43] = (dataBytes >> 24) & 0xFF;

  streamBase64(client, hdr, 44);
}

// ---- 公开 API ----

bool asrInit() {
  return (WiFi.status() == WL_CONNECTED);
}

String asrRecognize(const int16_t* pcmData, int pcmLen) {
  if (WiFi.status() != WL_CONNECTED || pcmLen == 0) return "";

  int dataBytes = pcmLen * 2;
  int wavTotal  = dataBytes + 44;

  Serial.printf("[ASR] sending %d samples via Zhipu GLM-ASR...\n", pcmLen);

  WiFiClientSecure client;
  client.setInsecure();               // 跳过证书验证 (ESP32-C3 上更稳定)
  client.setTimeout(12);

  if (!client.connect("open.bigmodel.cn", 443)) {
    Serial.println(F("[ASR] connect failed"));
    return "";
  }

  // ── 发送 HTTP 请求 ──
  String jsonPrefix = "{\"model\":\"glm-asr-2512\",\"file_base64\":\"";
  int contentLen = jsonPrefix.length() + (wavTotal * 4 / 3 + 4) + 3;  // +"}" + \r\n\r\n

  client.print("POST /api/paas/v4/audio/transcriptions HTTP/1.1\r\n");
  client.print("Host: open.bigmodel.cn\r\n");
  client.print("Authorization: Bearer " ZHIPU_API_KEY "\r\n");
  client.print("Content-Type: application/json\r\n");
  client.printf("Content-Length: %d\r\n", contentLen);
  client.print("\r\n");

  // ── 流式发送 JSON body ──
  client.print(jsonPrefix);
  writeWavHeader(client, dataBytes);
  streamBase64(client, (const uint8_t*)pcmData, dataBytes);
  client.print("\"}");
  client.flush();

  // ── 读取响应 ──
  String resp;
  unsigned long t0 = millis();
  while (client.connected() || client.available()) {
    if (client.available()) {
      resp += client.readString();
      if (resp.length() > 4096) break;  // 防止无限读
    }
    if (millis() - t0 > 10000) break;
    delay(10);
  }
  client.stop();

  // ── 解析: "text":"识别内容" ──
  int p = resp.indexOf("\"text\":\"");
  if (p < 0) {
    // 打印错误信息用于调试
    int e = resp.indexOf("\"message\":\"");
    if (e > 0) {
      e += 12;
      int e2 = resp.indexOf('"', e);
      Serial.printf("[ASR] error: %s\n", resp.substring(e, e2).c_str());
    }
    return "";
  }

  p += 9;
  int pe = resp.indexOf('"', p);
  String text = resp.substring(p, pe);

  if (text.length() == 0) return "";
  if (text.endsWith("。")) text = text.substring(0, text.length() - 3);

  Serial.printf("[ASR] -> %s\n", text.c_str());
  return text;
}
