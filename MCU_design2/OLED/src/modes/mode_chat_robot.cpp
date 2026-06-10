/**
 * Chatbot 模式 — ASR 语音识别 + OLED 逐行文字输出
 *
 * 完整管线:
 *   INMP441 硅麦 → I2S 录音 (16kHz/16bit) → 智谱 ASR 云端识别
 *   → OLED 逐行显示识别文字 → (可选) AI 对话 → OLED 显示回复
 *
 * 按键:
 *   A 长按: 录音 (松手自动识别)
 *   B 短按: 上翻历史
 *   C 短按: 下翻历史 / 清空
 */

#include "mode_chat_robot.h"
#include <WiFi.h>
#include "../hardware/display_hal.h"
#include "../hardware/buzzer_hal.h"
#include "../hardware/mic_hal.h"
#include "../services/asr_service.h"
#include "../services/ai_service.h"
#include "../services/tts_service.h"

// ================================================================
//  对话历史 (逐行存储)
// ================================================================
#define MAX_CONV_LINES  40
static String  convLines[MAX_CONV_LINES]; // "You: xxx" / "AI: xxx"
static int     convCount  = 0;            // 总行数
static int     convScroll = 0;            // 滚动偏移 (0=最新)
static bool    enableAI   = true;         // 是否启用 AI 回复

// ================================================================
//  状态机
// ================================================================
static int           s_state    = 0;       // 0=idle, 1=recording, 2=processing
static unsigned long s_t0       = 0;
static String        s_asrText  = "";      // 最近一次 ASR 识别结果

// ================================================================
//  辅助函数
// ================================================================

/** 向对话历史追加一行 */
static void convAppend(const String& role, const String& text) {
  if (convCount >= MAX_CONV_LINES) {
    // 移除最旧的一行
    for (int i = 0; i < MAX_CONV_LINES - 1; i++) {
      convLines[i] = convLines[i + 1];
    }
    convCount = MAX_CONV_LINES - 1;
  }
  convLines[convCount] = role + ": " + text;
  convCount++;
  convScroll = 0;  // 回到最新
}

// ================================================================
//  渲染
// ================================================================

void renderChatRobotIdle() {
  display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // ── 录音中 ──
  if (s_state == 1) {
    printCentred("Chatbot", CONTENT_Y + 0);
    printCentred("Listening...", CONTENT_Y + 10);
    int sec = (millis() - s_t0) / 1000;
    display.setCursor(2, CONTENT_Y + 24);
    display.printf("Recording %ds...", sec);
    display.setCursor(2, CONTENT_Y + 34);
    display.print(F("Release A to finish"));
    // 音量指示条 (简易)
    int barW = map(sec, 0, 3, 6, 120);
    if (barW > 120) barW = 120;
    display.drawRect(4, CONTENT_Y + 44, 120, 3, SSD1306_WHITE);
    display.fillRect(5, CONTENT_Y + 45, barW - 2, 1, SSD1306_WHITE);
  }

  // ── 识别/处理中 ──
  else if (s_state == 2) {
    printCentred("Processing...", CONTENT_Y + 8);
    display.setCursor(2, CONTENT_Y + 20);
    display.print(F("ASR recognizing..."));
    display.setCursor(2, CONTENT_Y + 30);
    display.print(F("Sending to cloud..."));
    // 简易动画点
    int dots = ((millis() - s_t0) / 300) % 4;
    display.setCursor(60, CONTENT_Y + 40);
    for (int i = 0; i < dots; i++) display.print('.');
  }

  // ── 空闲: 显示对话历史 ──
  else {
    if (convCount == 0) {
      // 空历史 → 提示
      printCentred("Chatbot", CONTENT_Y + 2);
      printCentred("Hold A to speak", CONTENT_Y + 14);
      display.setCursor(2, CONTENT_Y + 28);
      display.print(F("INMP441 I2S 16kHz"));
      display.setCursor(2, CONTENT_Y + 38);
      display.print(F("Zhipu GLM-ASR-2512"));
      display.setCursor(2, CONTENT_Y + 48);
      display.print(F("B:Up  C:Down  D:Mode"));
    } else {
      // 有历史 → 显示 2 条 (16px 行高) + 底部导航
      int total    = convCount;
      int lineH    = 16;
      int maxLines = 2;

      int start = total - maxLines - convScroll;
      if (start < 0) start = 0;
      int end = total - convScroll;
      if (end > total) end = total;
      if (end - start > maxLines) start = end - maxLines;
      if (start < 0) start = 0;

      for (int i = start; i < end; i++) {
        int y = CONTENT_Y + (i - start) * lineH;
        String line = convLines[i];
        if (line.startsWith("You:")) line = String("你") + line.substring(4);
        else if (line.startsWith("AI:")) line = String("机") + line.substring(3);
        printChinese(0, y, line);
      }

      // 底部导航条
      int navY = CONTENT_Y + 2 * lineH;  // 32
      display.fillRect(0, navY, SCREEN_W, 16, SSD1306_BLACK);
      display.setCursor(2, navY + 4);
      display.print(F("A:说"));
      display.setCursor(44, navY + 4);
      display.print(F("B:翻"));
      display.setCursor(86, navY + 4);
      display.print(F("C:清"));
    }
  }
}

// ================================================================
//  按键处理
// ================================================================

void handleChatRobotButtons() {
  Button& bA = buttons[0];
  Button& bB = buttons[1];
  Button& bC = buttons[2];

  if (s_state != 0) return;  // 忙时忽略

  // A 长按 → 开始录音
  if (bA.longPress) {
    s_state = 1;
    s_t0    = millis();
    lastUserAction = millis();
    micStartRecord();
    feedbackClick();
  }

  // B 短按 → 上翻历史 (scroll up = show older)
  if (bB.released && !bB.longPressActive) {
    if (convCount > 5 && convScroll < convCount - 5) {
      convScroll++;
    }
    lastUserAction = millis();
    feedbackClick();
  }

  // C 短按 → 下翻历史 / 清空
  if (bC.released && !bC.longPressActive) {
    if (convScroll > 0) {
      convScroll--;
    }
    lastUserAction = millis();
    feedbackClick();
  }

  // C 长按 → 清空对话历史
  if (bC.longPress) {
    convCount  = 0;
    convScroll = 0;
    feedbackClick();
    lastUserAction = millis();
  }
}

// ================================================================
//  每帧 Tick (检测录音松手)
// ================================================================

void chatRobotTick() {
  static bool wasHeld = false;

  if (s_state == 1) {
    // 检测按键状态
    if (buttons[0].raw == LOW) {
      wasHeld = true;
    }
    // 松手 → 停止录音 → 进入处理
    // 注意: micIsRecording() 可能因 3s 超时已自动停止, 所以不以此作为必要条件
    if (wasHeld && buttons[0].raw == HIGH) {
      if (micIsRecording()) {
        micStopRecord();           // 手动松手, 停止录音
      }
      // 即使用户按住超过 3s, mic 已自动停止, 松手后也要进入处理
      s_state = 2;
      s_t0    = millis();
      wasHeld = false;
    }
  }
}

// ================================================================
//  处理: ASR → (AI) → 显示
// ================================================================

void chatRobotProcess() {
  if (s_state != 2) return;

  int samples = micGetSampleCount();
  Serial.printf("[CHAT] mic samples=%d\n", samples);

  // ── 无录音数据 ──
  if (samples == 0) {
    Serial.println(F("[CHAT] no audio data — silent"));
    convAppend("You", "(silent)");
    s_state = 0;
    lastUserAction = millis();
    return;
  }

  // ── 检查 WiFi ──
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[CHAT] WiFi not connected, ASR skipped"));
    convAppend("You", "(no WiFi)");
    s_state = 0;
    lastUserAction = millis();
    return;
  }

  // ── PCM 数据诊断 ──
  {
    const int16_t* pcm = micGetData();
    int64_t sumSq = 0;
    int16_t peak = 0;
    int32_t dcSum = 0;
    for (int i = 0; i < samples; i++) {
      int16_t v = pcm[i];
      if (v > peak) peak = v;
      else if (-v > peak) peak = -v;
      sumSq += (int64_t)v * v;
      dcSum += v;
    }
    int rms = (int)sqrt((float)sumSq / samples);
    int dc  = (int)(dcSum / samples);
    Serial.printf("[CHAT] PCM: samples=%d peak=%d rms=%d dc=%d\n", samples, peak, rms, dc);
    // 打印前 16 个样本
    Serial.print("[CHAT] PCM[0..15]:");
    for (int i = 0; i < 16 && i < samples; i++) {
      Serial.printf(" %d", pcm[i]);
    }
    Serial.println();
    if (rms < 50) {
      Serial.println(F("[CHAT] ⚠ PCM amplitude too low — check mic connection or channel"));
    }
  }

  // ── Step 1: ASR 语音识别 ──
  Serial.println(F("[CHAT] calling asrRecognize..."));
  s_asrText = asrRecognize(micGetData(), samples);

  if (s_asrText.length() == 0) {
    // 识别失败 — 串口已由 asr_service 打印错误
    Serial.println(F("[CHAT] ASR returned empty"));
    convAppend("You", "(unrecognized)");
    s_state = 0;
    lastUserAction = millis();
    return;
  }

  Serial.printf("[CHAT] ASR recognized: %s\n", s_asrText.c_str());

  // ── Step 2: 显示识别文字 ──
  convAppend("You", s_asrText);

  // ── Step 3: AI 对话 (可选) ──
  if (enableAI && WiFi.status() == WL_CONNECTED) {
    Serial.println(F("[CHAT] calling AI chat..."));
    String reply = aiChat(s_asrText);
    if (reply.length() > 0) {
      Serial.printf("[CHAT] AI reply: %s\n", reply.c_str());
      convAppend("AI", reply);
      // ttsPlay(reply);  // TTS 余额不足, 暂时禁用
    } else {
      Serial.println(F("[CHAT] AI returned empty"));
    }
  }

  s_state = 0;
  lastUserAction = millis();
}
