/**
 * wifi_service.cpp — WiFi 连接 + OLED 交互式设置
 *
 * wifiSetup() 流程:
 *   1. 扫描 WiFi → OLED 显示 AP 列表
 *   2. A 滚动 / C 选择 / B 取消
 *   3. 密码输入: A 换字符 / B 下一格 / C 删一格 / D 确定连接
 */

#include "wifi_service.h"
#include <WiFi.h>
#include <Preferences.h>
#include "../hardware/display_hal.h"
#include "../hardware/button_hal.h"

// ---- 保存凭据到 NVS ----
static void wifiSaveCreds(const char* ssid, const char* pass) {
  Preferences prefs;
  prefs.begin("desktopdog", false);
  prefs.putString("wifi_ssid", ssid);
  prefs.putString("wifi_pass", pass);
  prefs.end();
  Serial.printf("[WiFi] Credentials saved: %s\n", ssid);
}

// ---- 开机自动连接 ----
void connectWiFi() {
  // 无凭据 → 直接启动扫码设置
  if (strlen(WIFI_SSID) == 0) {
    Serial.println(F("[WiFi] No saved credentials, launching setup..."));
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 24);
    display.print(F("No WiFi saved"));
    display.setCursor(0, 36);
    display.print(F("C:Setup  D:Skip"));
    display.display();

    // 等待用户按 C 进入设置，或 D 跳过
    while (true) {
      readButtons();
      if (buttons[2].pressed) {  // C → 进入设置
        wifiSetup();
        return;
      }
      if (buttons[3].pressed) {  // D → 离线使用
        Serial.println(F("[WiFi] User chose offline mode"));
        display.clearDisplay();
        display.setCursor(0, 28);
        display.print(F("Offline mode"));
        display.display();
        delay(800);
        return;
      }
      delay(30);
    }
  }

  // 有凭据 → 尝试连接
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("WiFi: "));
  display.println(WIFI_SSID);
  display.display();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    display.clearDisplay();
    display.setCursor(0, 24);
    display.print(F("WiFi OK!"));
    display.setCursor(0, 36);
    display.print(WiFi.localIP());
    display.display();
    delay(800);
  } else {
    // 连接失败 → 可能是密码变了, 启动重设
    Serial.println(F("[WiFi] Saved credentials failed, launching setup..."));
    display.clearDisplay();
    display.setCursor(0, 20);
    display.print(F("WiFi FAIL"));
    display.setCursor(0, 32);
    display.print(WIFI_SSID);
    display.setCursor(0, 44);
    display.print(F("Re-setup needed"));
    display.display();
    delay(1500);
    wifiSetup();
  }
}

// ================================================================
//  OLED 交互式 WiFi 设置
// ================================================================

// 密码输入可用字符
static const char PWD_CHARS[] =
  " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+[]{};:',.<>/?";
static const int PWD_CHAR_COUNT = sizeof(PWD_CHARS) - 1;

static void drawCenter(const char* s, int y) {
  int x = (SCREEN_W - strlen(s) * 6) / 2;
  if (x < 0) x = 0;
  display.setCursor(x, y);
  display.print(s);
}

// ── 步骤 1: 扫描并选择网络 ──
static String wifiSelectAP() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print(F("Scanning WiFi..."));
  display.display();

  int n = WiFi.scanNetworks(false, true);  // async=false, show_hidden=true
  if (n <= 0) {
    display.clearDisplay();
    drawCenter("No networks found", 28);
    display.display();
    delay(2000);
    return "";
  }

  int cursor = 0;
  unsigned long lastAction = millis();

  while (true) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("%d APs found", n);
    display.drawFastHLine(0, 9, SCREEN_W, SSD1306_WHITE);

    // 显示 5 行
    int start = (cursor >= 4) ? cursor - 3 : 0;
    for (int i = start; i < n && i < start + 5; i++) {
      int y = 10 + (i - start) * 10;
      display.setCursor(4, y);
      display.print(i == cursor ? '>' : ' ');
      // SSID + RSSI
      char line[22];
      String ssid = WiFi.SSID(i);
      if (ssid.length() > 15) ssid = ssid.substring(0, 13) + "..";
      int rssi = WiFi.RSSI(i);
      snprintf(line, sizeof(line), "%s (%d%%)", ssid.c_str(), constrain(map(rssi, -100, -30, 0, 100), 0, 100));
      display.print(line);
    }

    display.setCursor(0, 56);
    display.print(F("A:Sel C:OK B:Cancel"));

    display.display();

    // —— 轮询按键 (直接 readButtons, 避免 processButtonEvents 重入) ——
    readButtons();
    if (buttons[0].pressed && cursor < n - 1) { cursor++; lastAction = millis(); }
    if (buttons[2].pressed) {
      String ssid = WiFi.SSID(cursor);
      WiFi.scanDelete();
      return ssid;
    }
    if (buttons[1].pressed) { WiFi.scanDelete(); return ""; }

    delay(30);
  }
}

// ── 步骤 2: 密码输入 ──
static bool wifiInputPwd(const String& ssid, char* pwdOut, int maxLen) {
  char buf[33] = {0};
  int len = 0;
  int charIdx = 0;  // 字符表索引, 默认从空格开始

  while (true) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.print(ssid.length() > 18 ? ssid.substring(0, 15) + ".." : ssid);

    display.drawFastHLine(0, 9, SCREEN_W, SSD1306_WHITE);

    // 当前密码 (掩码显示)
    display.setCursor(0, 14);
    for (int i = 0; i < len; i++) display.print('*');
    if (len < maxLen - 1) display.print('_');  // 光标

    // 当前选择的字符
    display.setCursor(0, 26);
    display.print(F("Char: ["));
    display.print(PWD_CHARS[charIdx]);
    display.print(']');

    display.setCursor(0, 40);
    display.print(F("A:Change B:Next"));
    display.setCursor(0, 50);
    display.print(F("C:Del D:Connect"));

    display.display();

    // —— 轮询 ——
    readButtons();
    if (buttons[0].pressed) {
      charIdx++;
      if (charIdx >= PWD_CHAR_COUNT) charIdx = 0;
    }
    if (buttons[1].pressed) {
      if (len < maxLen - 1) { buf[len++] = PWD_CHARS[charIdx]; buf[len] = 0; }
    }
    if (buttons[2].pressed) {
      if (len > 0) { len--; buf[len] = 0; }
    }
    if (buttons[3].pressed) {
      if (len > 0) {
        strncpy(pwdOut, buf, maxLen);
        return true;
      }
    }

    delay(30);
  }
}

// ── 公开入口 ──
void wifiSetup() {
  display.clearDisplay();
  display.display();

  // 步骤 1: 选网络
  String ssid = wifiSelectAP();
  if (ssid.length() == 0) return;

  // 步骤 2: 输密码
  char pwd[32] = {0};
  if (!wifiInputPwd(ssid, pwd, sizeof(pwd))) return;

  // 步骤 3: 连接
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print(F("Connecting to:"));
  display.setCursor(0, 32);
  display.print(ssid);
  display.display();

  WiFi.disconnect();
  delay(200);
  WiFi.begin(ssid.c_str(), pwd);

  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 12000) { delay(300); }

  display.clearDisplay();
  if (WiFi.status() == WL_CONNECTED) {
    // 保存新凭据
    strncpy(WIFI_SSID, ssid.c_str(), 31);
    strncpy(WIFI_PASS, pwd, 31);
    WIFI_SSID[31] = 0;
    WIFI_PASS[31] = 0;
    wifiSaveCreds(WIFI_SSID, WIFI_PASS);  // 持久化到 NVS

    display.setCursor(0, 24);
    display.print(F("Connected!"));
    display.setCursor(0, 36);
    display.print(WiFi.localIP());
  } else {
    display.setCursor(0, 24);
    display.print(F("Connection failed"));
  }
  display.display();
  delay(2000);
}
