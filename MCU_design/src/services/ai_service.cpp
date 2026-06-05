/**
 * ai_service.cpp — 智谱 GLM-4-Flash AI 对话
 */

#include "ai_service.h"
#include "../config.h"
#include <WiFi.h>
#include <HTTPClient.h>

String aiChat(const String& text) {
  if (WiFi.status() != WL_CONNECTED) return "";

  HTTPClient http;
  http.setTimeout(15000);
  http.begin("https://open.bigmodel.cn/api/paas/v4/chat/completions");
  http.addHeader("Authorization", "Bearer " ZHIPU_API_KEY);
  http.addHeader("Content-Type", "application/json");

  // 手动转义双引号
  String escaped = text;
  escaped.replace("\\", "\\\\");
  escaped.replace("\"", "\\\"");

  String body = "{\"model\":\"glm-4-flash\",\"messages\":[{\"role\":\"user\",\"content\":\""
              + escaped + "\"}]}";

  int code = http.POST(body);
  if (code != 200) {
    Serial.printf("[AI] HTTP %d\n", code);
    http.end();
    return "";
  }

  String resp = http.getString();
  http.end();

  int p = resp.indexOf("\"content\":\"");
  if (p < 0) return "";
  p += 12;

  int pe = p;
  while (true) {
    pe = resp.indexOf('"', pe);
    if (pe < 0 || resp[pe - 1] != '\\') break;
    pe++;
  }
  if (pe < 0) return "";

  String reply = resp.substring(p, pe);
  reply.replace("\\\"", "\"");
  reply.replace("\\n", "\n");

  Serial.printf("[AI] -> %s\n", reply.c_str());
  return reply;
}
