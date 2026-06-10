/**
 * display_hal.h — SSD1306 OLED 显示硬件抽象层
 * 封装 Adafruit_SSD1306，提供项目专用的绘制辅助函数
 */

#ifndef DISPLAY_HAL_H
#define DISPLAY_HAL_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "../config.h"

/** 全局 display 对象 */
extern Adafruit_SSD1306 display;

// ================================================================
//  初始化
// ================================================================

/** 初始化 OLED 显示 */
bool displayInit();

// ================================================================
//  绘制辅助函数
// ================================================================

/** 居中打印 ASCII 字符串 */
void printCentred(const char* str, int y);

/** 打印中文字符串 (UTF-8, 16x16 点阵) */
void printChinese(int x, int y, const String& text);

/** 中文字体行高 */
int chineseLineHeight();

/** 绘制 PROGMEM 图标 (位图) */
void drawIcon(int x, int y, const uint8_t* bmp, int w, int h);

/** 在指定行打印文本，可选高亮 (反色) */
void printLine(int y, const char* str, bool hi);

/** 绘制内容区域帧 (128x55 位图，带 X 偏移，用于滑动动画) */
void drawContentFrame(const uint8_t* bmp, int offsetX);

/** 绘制全屏 (状态栏 + 内容帧) 并提交到显示 */
void drawFullScreen(const uint8_t* bmp, int offsetX);

#endif // DISPLAY_HAL_H
