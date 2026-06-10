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

/**
 * 将两段数据 (hdr + pcm) 拼接为一个连续字节流做 base64 编码
 * 再分块写入 client。这样 WAV 文件在 base64 层面是完整连续的，
 * 不会因分段编码导致中间出现错误的填充字符。
 */
static void streamBase64Concat(WiFiClientSecure& client,
                               const uint8_t* hdr, size_t hdrLen,
                               const uint8_t* pcm, size_t pcmLen) {
  size_t total = hdrLen + pcmLen;
  size_t hdrPos = 0, pcmPos = 0;

  // 临时小缓冲: 从 hdr+pcm 中按顺序取 CHUNK 个字节
  const size_t CHUNK = 480;  // 必须是 3 的倍数
  for (size_t pos = 0; pos < total; pos += CHUNK) {
    size_t n = (pos + CHUNK <= total) ? CHUNK : (total - pos);
    uint8_t chunk[CHUNK];  // 栈上分配, 480 字节
    size_t c = 0;
    while (c < n) {
      if (hdrPos < hdrLen) {
        size_t take = n - c;
        if (take > hdrLen - hdrPos) take = hdrLen - hdrPos;
        memcpy(chunk + c, hdr + hdrPos, take);
        hdrPos += take; c += take;
      } else {
        size_t take = n - c;
        memcpy(chunk + c, pcm + pcmPos, take);
        pcmPos += take; c += take;
      }
    }

    // base64 编码这 n 个字节
    char buf[CHUNK / 3 * 4 + 2];
    size_t out = 0;
    for (size_t j = 0; j + 2 < n; j += 3) {
      b64Encode3(chunk[j], chunk[j + 1], chunk[j + 2], buf + out);
      out += 4;
    }
    if (n % 3 != 0) {
      size_t rem = n % 3;
      uint8_t tail[3] = {0, 0, 0};
      for (size_t j = 0; j < rem; j++) tail[j] = chunk[n - rem + j];
      b64Encode3(tail[0], tail[1], tail[2], buf + out);
      out += 4;
      if (rem == 1) buf[out - 2] = '=';
      buf[out - 1] = '=';
    }
    buf[out] = '\0';
    client.print(buf);
  }
}

/** 构建 44 字节 WAV 头到缓冲区 (不再写入 client) */
static void buildWavHeader(uint8_t hdr[44], int dataBytes) {
  memset(hdr, 0, 44);
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
  int wavBytes  = dataBytes + 44;
  int b64Len    = (wavBytes + 2) / 3 * 4;  // base64 精确长度
  String jsonPrefix = "{\"model\":\"glm-asr-2512\",\"file_base64\":\"";
  int contentLen = jsonPrefix.length() + b64Len + 3;   // +"\"}"

  client.print("POST /api/paas/v4/audio/transcriptions HTTP/1.1\r\n");
  client.print("Host: open.bigmodel.cn\r\n");
  client.print("Authorization: Bearer " ZHIPU_API_KEY "\r\n");
  client.print("Content-Type: application/json\r\n");
  client.printf("Content-Length: %d\r\n", contentLen);
  client.print("\r\n");

  // ── 流式发送 JSON body ──
  // 关键修复: WAV 头 + PCM 数据 合并为一个连续流再做 base64,
  // 避免分段编码导致中间出现错误的 '=' 填充
  uint8_t wavHdr[44];
  buildWavHeader(wavHdr, dataBytes);
  client.print(jsonPrefix);
  streamBase64Concat(client, wavHdr, 44, (const uint8_t*)pcmData, dataBytes);
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

  // ── 提取 JSON body (跳过 HTTP 头) ──
  String body = resp;
  int bodyStart = resp.indexOf("\r\n\r\n");
  if (bodyStart > 0) body = resp.substring(bodyStart + 4);

  Serial.printf("[ASR] JSON body (%dB): ", body.length());
  Serial.println(body);

  // ── 解析: "text":"识别内容" ──
  int p = body.indexOf("\"text\":\"");
  if (p < 0) {
    // 可能用其他字段名
    int e = body.indexOf("\"message\":\"");
    if (e > 0) {
      e += 12;
      int e2 = body.indexOf('"', e);
      Serial.printf("[ASR] error: %s\n", body.substring(e, e2).c_str());
    }
    return "";
  }

  p += 8;  // "text":" = 8 字符
  int pe = body.indexOf('"', p);
  String text = body.substring(p, pe);

  if (text.length() == 0 || text == "#") {
    Serial.println(F("[ASR] text field is empty or noise marker"));
    return "";
  }
  if (text.endsWith("。")) text = text.substring(0, text.length() - 3);

  Serial.printf("[ASR] -> %s\n", text.c_str());
  return text;
}
