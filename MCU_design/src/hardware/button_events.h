/**
 * button_events.h — 按键事件管理层
 * 位于硬件层之上、模式层之下，统一管理去抖读取、全局分发、模式路由
 *
 * 调用链:
 *   main.cpp loop() → processButtonEvents()
 *     ├─ readButtons()                    // HAL: 去抖 + 边沿检测
 *     ├─ dispatchGlobalButtons()          // D/E 跨模式: 电源、模式切换、设置入口
 *     └─ dispatchModeButtons()            // A/B/C 按模式分发
 */

#ifndef BUTTON_EVENTS_H
#define BUTTON_EVENTS_H

#include "../config.h"

/**
 * 主循环每帧调用一次。
 * 封装 readButtons() + 全局分发 + 模式路由。
 * 返回 true 表示本帧有按键事件发生 (用于更新 lastUserAction)。
 */
bool processButtonEvents();

#endif // BUTTON_EVENTS_H
