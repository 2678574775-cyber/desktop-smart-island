/**
 * display_hal.cpp — SSD1306 OLED 显示硬件抽象层实现
 */

#include "display_hal.h"
#include "../ui/icons.h"
#include "../ui/OLED_Font.h"   // 中文字库 (16x16 点阵)

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

// ---- 页-列格式位图 → Adafruit 像素绘制 ----
static void drawPageColBitmap(int x, int y, const uint8_t* data,
                               int w, int h, uint16_t color) {
  int pages = (h + 7) / 8;
  int page  = y / 8;
  int shift = y % 8;

  for (int pg = 0; pg < pages; pg++) {
    int dp = page + pg;
    for (int col = 0; col < w; col++) {
      int dx = x + col;
      if (dx < 0 || dx >= SCREEN_W) continue;

      uint8_t b = data[pg * w + col];
      if (!b) continue;

      if (dp >= 0 && dp < 8) {
        uint8_t b1 = b << shift;
        for (int bit = 0; bit < 8; bit++) {
          if (b1 & (1 << bit)) display.drawPixel(dx, dp * 8 + bit, color);
        }
      }
      if (shift > 0 && dp + 1 >= 0 && dp + 1 < 8) {
        uint8_t b2 = b >> (8 - shift);
        for (int bit = 0; bit < 8; bit++) {
          if (b2 & (1 << bit)) display.drawPixel(dx, (dp + 1) * 8 + bit, color);
        }
      }
    }
  }
}

// ---- 中文打印 (UTF-8, 16x16 点阵) ----
void printChinese(int x, int y, const String& text) {
  uint8_t buf[OLED_CHN_CHAR_WIDTH + 1] = {0};
  uint8_t ci = 0;
  static int missCount = 0;

  for (int i = 0; i < (int)text.length(); i++) {
    char c = text[i];
    if ((c & 0x80) == 0) {
      uint8_t idx = (uint8_t)c;
      if (idx >= ' ' && idx <= '~') {
        drawPageColBitmap(x, y, OLED_F8x16[idx - ' '], 8, 16, SSD1306_WHITE);
      }
      x += 8;
      ci = 0;
      continue;
    }
    buf[ci++] = c;
    if (ci >= OLED_CHN_CHAR_WIDTH) {
      buf[ci] = '\0';
      ci = 0;
      bool found = false;
      for (int idx = 0; ; idx++) {
        if (strcmp(OLED_CF16x16[idx].Index, "") == 0) break;
        if (strcmp(OLED_CF16x16[idx].Index, (const char*)buf) == 0) {
          drawPageColBitmap(x, y, OLED_CF16x16[idx].Data, 16, 16, SSD1306_WHITE);
          found = true;
          break;
        }
      }
      if (!found) {
        missCount++;
        display.drawRect(x + 2, y + 2, 12, 12, SSD1306_WHITE);  // □ 占位
        Serial.printf("[FONT] miss #%d: %02X %02X %02X\n",
                      missCount, buf[0], buf[1], buf[2]);
      }
      x += 16;
    }
  }
}

int chineseLineHeight() {
  return 16;
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

// ---- 绘制内容帧 ----
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

void drawFullScreen(const uint8_t* bmp, int offsetX) {
  drawContentFrame(bmp, offsetX);
  display.display();
}
