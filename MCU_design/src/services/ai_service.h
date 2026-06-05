/**
 * ai_service.h — 智谱 AI 对话 (GLM-4-Flash)
 * API Key 在 config.h 中定义为 ZHIPU_API_KEY
 */

#ifndef AI_SERVICE_H
#define AI_SERVICE_H

#include <Arduino.h>

String aiChat(const String& text);

#endif
