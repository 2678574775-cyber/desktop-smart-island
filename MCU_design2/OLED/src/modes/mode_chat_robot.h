/**
 * Chatbot 模式 — ASR 语音识别 + OLED 逐行文字输出
 *
 * 流程: 长按A录音 → 松手 → ASR云端识别 → OLED显示识别文字
 *       可选: 识别文字 → AI对话 → OLED显示AI回复
 *
 * 按键:
 *   A 长按: 录音 (松手识别)
 *   B 短按: 上翻历史
 *   C 短按: 下翻历史
 *   D 短按: 切换模式
 */

#ifndef MODE_CHAT_ROBOT_H
#define MODE_CHAT_ROBOT_H
#include "../config.h"

void renderChatRobotIdle();
void handleChatRobotButtons();
void chatRobotTick();
void chatRobotProcess();

#endif
