/**
 * status_bar.h — 状态栏渲染 (9px, y=0~8)
 * 包含模式图标、时间文字、WiFi 图标、电池图标
 */

#ifndef STATUS_BAR_H
#define STATUS_BAR_H

#include "../config.h"

/** 绘制状态栏 (y=0~8) */
void drawStatusBar();

/** 根据当前模式状态更新 statusBarTag (RUN/PAU/DON 等) */
void updateStatusBarTag();

/** 根据当前模式更新 currentModeIcon */
void updateModeIcon();

#endif // STATUS_BAR_H
