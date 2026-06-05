/**
 * display_hal.cpp — SSD1306 OLED 显示硬件抽象层实现
 */

#include "display_hal.h"
#include "../ui/icons.h"

// ---- 全局 display 对象 ----
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);

// ---- 初始化 ----
bool displayInit() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    return false;
  }
  display.clearDisplay();
  display.display();
  return true;
}

// ---- 居中打印 ----
void printCentred(const char* str, int y) {
  int len = strlen(str);
  int x = (SCREEN_W - len * 6) / 2;
  if (x < 0) x = 0;
  display.setCursor(x, y);
  display.print(str);
}

// ---- 绘制 PROGMEM 图标 ----
void drawIcon(int x, int y, const uint8_t* bmp, int w, int h) {
  int bytesPerRow = (w + 7) / 8;
  for (int row = 0; row < h; row++) {
    for (int bx = 0; bx < bytesPerRow; bx++) {
      uint8_t b = pgm_read_byte(bmp + row * bytesPerRow + bx);
      if (!b) continue;
      for (int bit = 0; bit < 8; bit++) {
        int px = bx * 8 + bit;
        if (px >= w) break;
        if (b & (0x80 >> bit)) {
          display.drawPixel(x + px, y + row, SSD1306_WHITE);
        }
      }
    }
  }
}

// ---- 带高亮的行打印 ----
void printLine(int y, const char* str, bool hi) {
  display.setCursor(2, y);
  if (hi) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  } else {
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  }
  display.print(str);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
}

// ---- 绘制内容帧 (128x55 位图，带 X 偏移) ----
void drawContentFrame(const uint8_t* bmp, int offsetX) {
  display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, SSD1306_BLACK);
  for (int row = 0; row < CONTENT_H; row++) {
    int base = row * (CONTENT_W / 8);
    int screenY = CONTENT_Y + row;
    for (int bx = 0; bx < CONTENT_W / 8; bx++) {
      uint8_t b = pgm_read_byte(bmp + base + bx);
      if (!b) continue;
      for (int bit = 0; bit < 8; bit++) {
        if (b & (0x80 >> bit)) {
          int px = bx * 8 + bit + offsetX;
          if (px >= 0 && px < SCREEN_W) {
            display.drawPixel(px, screenY, SSD1306_WHITE);
          }
        }
      }
    }
  }
}

// ---- 绘制全屏 (状态栏 + 内容帧) ----
void drawFullScreen(const uint8_t* bmp, int offsetX) {
  // drawStatusBar() 是 ui 层函数，这里只负责内容帧和提交
  // 状态栏由外部在调用前绘制
  drawContentFrame(bmp, offsetX);
  display.display();
}
