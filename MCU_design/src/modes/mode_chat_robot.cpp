/**
 * Chatbot 模式 (per README.md)
 *   A长按: 录音 → 松手识别 → AI → 显示回复
 *   B短按: 显/隐文字
 * 提示: D单击切模式  D长按进设置  D双击切CLOCK
 */

#include "mode_chat_robot.h"
#include "../hardware/display_hal.h"
#include "../hardware/buzzer_hal.h"
#include "../hardware/mic_hal.h"
#include "../services/asr_service.h"
#include "../services/ai_service.h"
#include "../services/tts_service.h"

static String s_reply;
static bool   s_showText = true;
static int    s_state = 0;   // 0=idle, 1=recording, 2=processing
static unsigned long s_t0 = 0;

void renderChatRobotIdle() {
  display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (s_state == 1) {
    printCentred("Chatbot", CONTENT_Y + 4);
    printCentred("Listening...", CONTENT_Y + 18);
    int s = (millis() - s_t0) / 1000;
    display.setCursor(2, CONTENT_Y + 36);
    display.printf("Recording %ds...", s);
    display.setCursor(2, CONTENT_Y + 48);
    display.print(F("Release to send"));
  }
  else if (s_state == 2) {
    printCentred("Chatbot", CONTENT_Y + 8);
    printCentred("Processing...", CONTENT_Y + 24);
    display.setCursor(2, CONTENT_Y + 42);
    display.print(F("ASR→AI→TTS..."));
  }
  else {
    printCentred("Chatbot", CONTENT_Y + 2);
    if (s_reply.length() > 0 && s_showText) {
      // 显示回复 (自动换行)
      display.setCursor(2, CONTENT_Y + 14);
      String r = s_reply;
      while (r.length() > 0) {
        int n = r.length() > 21 ? 21 : r.length();
        for (int i = 0; i < n; i++) display.print(r[i]);
        display.println();
        r = r.substring(n);
      }
      display.setCursor(2, CONTENT_Y + 48);
      display.print(F("B:Hide  Hold A:Speak"));
    } else {
      printCentred("Hold A to speak", CONTENT_Y + 24);
      display.setCursor(2, CONTENT_Y + 42);
      display.print(F("B:Show/Hide text"));
    }
  }
}

void handleChatRobotButtons() {
  Button& bA = buttons[0];
  Button& bB = buttons[1];

  if (s_state != 0) return;

  // A 长按录音 → 松手上传
  if (bA.longPress) {
    s_state = 1; s_t0 = millis();
    lastUserAction = millis();
    micStartRecord();
    feedbackClick();
  }

  // B 短按 → 切换显/隐文字
  if (bB.released && !bB.longPressActive) {
    s_showText = !s_showText;
    lastUserAction = millis();
    feedbackClick();
  }
}

void chatRobotTick() {
  // 录音中: 松手时处理
  if (s_state == 1 && buttons[0].raw == HIGH && !micIsRecording()) {
    // 已松手且录音已停 → 处理
  }
  // 如果录音中且松手了
  static bool wasHeld = false;
  if (s_state == 1) {
    if (buttons[0].raw == LOW) wasHeld = true;
    if (wasHeld && buttons[0].raw == HIGH && micIsRecording()) {
      // 松手 → 停止录音 → 处理
      micStopRecord();
      s_state = 2;
      wasHeld = false;
    }
  }
}

// 在主循环中检查录音完成
void chatRobotProcess() {
  if (s_state == 1) return;  // 录音中
  if (s_state != 2) return;   // 不在处理中

  // ASR → AI → TTS
  String text = asrRecognize(micGetData(), micGetSampleCount());

  if (text.length() > 0) {
    s_reply = aiChat(text);
    if (s_reply.length() == 0) s_reply = "网络出错";
    s_showText = true;
    ttsPlay(s_reply);
  }

  s_state = 0;
  lastUserAction = millis();
}
