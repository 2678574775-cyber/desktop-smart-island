/*
 * maikefeng.ino -- OLED 6-Mode System + 5-Button Direct Input
 * ESP32-C3 + 0.96" dual-color SSD1306 OLED (128x64)
 *
 * Layout (spec v1.2):
 *   y =  0 .. 15   STATUS BAR  (yellow zone)
 *   y = 16 .. 63   CONTENT      (blue zone)
 *
 * Modes (§2): CHAT-ROBOT(default) / CLOCK / COUNTDOWN / TODO / WEATHER / SETTINGS
 * Buttons (§2.1): B=Mode  C=Main  D=Secondary  E=Up/L  F=Down/R
 *   (A 键功能已合并：双击 E = 原 A 长按关机)
 *
 * Wiring (direct GPIO, pull-up via INPUT_PULLUP):
 *   OLED SDA -> GPIO8   OLED SCL -> GPIO9
 *   KEY_B    -> GPIO0   (一端接 GPIO0，另一端接 GND)
 *   KEY_C    -> GPIO1   (一端接 GPIO1，另一端接 GND)
 *   KEY_D    -> GPIO2   (一端接 GPIO2，另一端接 GND)
 *   KEY_E    -> GPIO3   (一端接 GPIO3，另一端接 GND)
 *   KEY_F    -> GPIO4   (一端接 GPIO4，另一端接 GND)
 *   BUZZER/LED -> -1 (未焊接)
 *
 * Libraries (Library Manager):
 *   Adafruit SSD1306     Adafruit GFX Library
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <time.h>

// ================================================================
//  HARDWARE PIN DEFINITIONS
// ================================================================
#define OLED_SDA        8
#define OLED_SCL        9
#define OLED_RESET     -1
#define OLED_ADDR   0x3C

// 5 direct-wired buttons: one side to GPIO, other side to GND.
// Uses internal INPUT_PULLUP — no external resistors needed.
#define KEY_B_PIN       0    // Mode button
#define KEY_C_PIN       1    // Main / Confirm button
#define KEY_D_PIN       2    // Secondary / Back button
#define KEY_E_PIN       3    // Up / Decrease button  (double-tap = Power/Shutdown)
#define KEY_F_PIN       4    // Down / Increase button

#define BUZZER_PIN     -1   // not populated on PCB
#define LED_PIN        -1   // LED not populated on PCB

// ================================================================
//  SCREEN CONSTANTS
//  Note: SSD1306 dual-colour OLED: top 16px = yellow zone, rest = blue
//        Status bar fits in the yellow zone (height 9px per spec §1.3)
// ================================================================
#define SCREEN_W      128
#define SCREEN_H       64
#define STATUSBAR_H     9    // spec §1.3: y=0~8, 9px tall
#define CONTENT_Y       9    // content starts right below status bar
#define CONTENT_H      55    // 64 - 9 = 55px content area
#define CONTENT_W     128

// ================================================================
//  SOFTWARE WATCHDOG (HW timer checks main loop liveness)
//  If loop() stalls > WDT_TIMEOUT_MS, soft-reset to CHAT-ROBOT
// ================================================================
#define WDT_TIMEOUT_MS    5000    // trip after 5s stall
volatile bool    wdtTripped    = false;
volatile uint32_t wdtFeedCount  = 0;
hw_timer_t      *wdtTimer       = NULL;

void IRAM_ATTR wdtISR() {
  wdtFeedCount++;
  if (wdtFeedCount > (WDT_TIMEOUT_MS / 100)) {
    wdtTripped = true;
  }
}

Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);

// ================================================================
//  WIFI & NTP
// ================================================================
const char* WIFI_SSID  = "Xiaomi 15";
const char* WIFI_PASS  = "12345678";
const char* NTP_SERVER = "ntp.aliyun.com";
const long   GMT_OFFSET = 8 * 3600;
const int    DAYLIGHT   = 0;
char          timeStr[6] = "--:--";        // HH:MM 北京时间
bool          timeSynced   = false;       // NTP 是否成功同步�?
unsigned long lastTimeUpdate = 0;
unsigned long lastNtpRetry    = 0;        // 上次 NTP 重试时间
#define TIME_REFRESH_MS   60000           // �?60s 刷新时间
#define NTP_RETRY_MS     120000           // NTP 失败�?2min 重试

// ================================================================
//  STATUS BAR LAYOUT (§1.3)
//  Spec: y=0~8, 9px total
//    x=0~13   : mode icon (12x8)
//    x=14~103 : system clock HH:MM (centred in this region)
//    x=104~115: WiFi icon (12x8)
//    x=116~127: Battery icon (12x8)
// ================================================================
#define MODE_ICON_X      0
#define WIFI_ICON_X    104
#define BATT_ICON_X    116
#define TIME_CURSOR_X   40    // cursor for centred HH:MM text (x=14~103 region centre �?40)
#define ICON_Y           0    // icons start at y=0 (8px tall icons fit in 9px bar)
char statusBarTag[4] = "";    // mode-specific label (e.g., "RUN","PAU","DON")

// ================================================================
//  ENUMS
// ================================================================
enum AppMode {
  MODE_CHAT_ROBOT = 0,
  MODE_CLOCK      = 1,
  MODE_COUNTDOWN  = 2,
  MODE_TODO       = 3,
  MODE_WEATHER    = 4,
  MODE_SETTINGS   = 5,
  MODE_COUNT      = 6
};

enum ModeIcon {
  ICON_MODE_CLOCK      = 0,
  ICON_MODE_COUNTDOWN  = 1,
  ICON_MODE_CHAT_ROBOT = 2,
  ICON_MODE_TODO       = 3,
  ICON_MODE_WEATHER    = 4,
  ICON_MODE_SETTINGS   = 5,
  MODE_ICON_COUNT
};

enum BattState {
  BATT_FULL  = 0, BATT_MID = 1, BATT_LOW = 2, BATT_CHARGE = 3
};
enum WifiState {
  WIFI_STRONG = 0, WIFI_MID = 1, WIFI_WEAK = 2, WIFI_OFFLINE = 3
};

// Countdown sub-states
enum CdState { CD_IDLE, CD_RUNNING, CD_PAUSED, CD_DONE };

// CHAT-ROBOT sub-states
enum CrSubState { CR_EXPR, CR_LISTEN, CR_THINK, CR_REPLY };

// TODO sub-states
enum TodoSubState { TODO_LIST, TODO_DETAIL };

// WEATHER sub-page
enum WxPage { WX_TODAY, WX_FORECAST };

// ================================================================
//  STATUS BAR ICON BITMAPS (existing �?unchanged)
// ================================================================
static const uint8_t PROGMEM MODE_CLOCK_BMP[] = {
  0x3F,0x00, 0x41,0x00, 0x4C,0x80, 0x54,0x80,
  0x54,0x80, 0x4C,0x80, 0x41,0x00, 0x3F,0x00,
};
static const uint8_t PROGMEM MODE_COUNTDOWN_BMP[] = {
  0x3F,0x00, 0x41,0x00, 0x21,0x00, 0x12,0x00,
  0x0F,0x00, 0x06,0x00, 0x0F,0x00, 0x3F,0x00,
};
static const uint8_t PROGMEM MODE_CHAT_ROBOT_BMP[] = {
  0x33,0x00, 0x79,0x00, 0x79,0x00, 0x4F,0x00,
  0x4F,0x00, 0x41,0x00, 0x4D,0x00, 0x00,0x00,
};
static const uint8_t PROGMEM MODE_WEATHER_BMP[] = {
  0x1E,0x80, 0x3F,0x00, 0x7F,0x80, 0xFC,0x80,
  0x7C,0x80, 0x3C,0x80, 0x00,0x40, 0x3F,0x00,
};
static const uint8_t PROGMEM MODE_SETTINGS_BMP[] = {
  0x0C,0x80, 0x1E,0x80, 0x6F,0x00, 0xF6,0x00,
  0x6F,0x00, 0x1E,0x80, 0x0C,0x80, 0x00,0x00,
};
static const uint8_t PROGMEM MODE_TODO_BMP[] = {
  0xFF,0xF0, 0x80,0x10, 0xB6,0xD0, 0x80,0x10,
  0xB6,0xD0, 0x80,0x10, 0xFF,0xF0, 0x00,0x00,
};
// Battery icons (12x8, MSB-first)
// Battery outline with nub, internal vertical bars depleting left to right
// FULL: 3 bars  MID: 2 bars  LOW: 1 bar  CHARGE: lightning bolt
static const uint8_t PROGMEM BATT_FULL_BMP[] = {
  0x1E,0x00, 0xFF,0x40, 0xAA,0x40, 0xAA,0x40,
  0xAA,0x40, 0xAA,0x40, 0x80,0x40, 0xFF,0x40
};
static const uint8_t PROGMEM BATT_MID_BMP[] = {
  0x1E,0x00, 0xFF,0x40, 0xA8,0x40, 0xA8,0x40,
  0xA8,0x40, 0xA8,0x40, 0x80,0x40, 0xFF,0x40
};
static const uint8_t PROGMEM BATT_LOW_BMP[] = {
  0x1E,0x00, 0xFF,0x40, 0xA0,0x40, 0xA0,0x40,
  0xA0,0x40, 0xA0,0x40, 0x80,0x40, 0xFF,0x40
};
static const uint8_t PROGMEM BATT_CHARGE_BMP[] = {
  0x1E,0x00, 0xFF,0x40, 0xA4,0x40, 0xB0,0x40,
  0xA4,0x40, 0xC2,0x40, 0x80,0x40, 0xFF,0x40
};
// Bar-style WiFi icons (12x8, MSB-first)
// 3 vertical bars ascending left to right, bottom-aligned (standard signal bar style)
// left=x2 (h=3), mid=x6 (h=5), right=x10 (h=8)
// STRONG: 3 bars  MID: mid+right  WEAK: right only
// OFFLINE: X mark (unchanged)
static const uint8_t PROGMEM WIFI_STRONG_BMP[] = {
  0x00,0x20, 0x00,0x20, 0x00,0x20, 0x02,0x20,
  0x02,0x20, 0x22,0x20, 0x22,0x20, 0x22,0x20
};
static const uint8_t PROGMEM WIFI_MID_BMP[] = {
  0x00,0x20, 0x00,0x20, 0x00,0x20, 0x02,0x20,
  0x02,0x20, 0x02,0x20, 0x02,0x20, 0x02,0x20
};
static const uint8_t PROGMEM WIFI_WEAK_BMP[] = {
  0x00,0x20, 0x00,0x20, 0x00,0x20, 0x00,0x20,
  0x00,0x20, 0x00,0x20, 0x00,0x20, 0x00,0x20
};
static const uint8_t PROGMEM WIFI_OFFLINE_BMP[] = {
  0xC0,0xC0, 0x61,0x80, 0x1B,0x00, 0x06,0x00,
  0x06,0x00, 0x1B,0x00, 0x61,0x80, 0xC0,0xC0
};


const uint8_t* const mode_icon_table[] PROGMEM = {
  MODE_CLOCK_BMP, MODE_COUNTDOWN_BMP, MODE_CHAT_ROBOT_BMP,
  MODE_TODO_BMP, MODE_WEATHER_BMP, MODE_SETTINGS_BMP,
};
const uint8_t* const batt_icon_table[] PROGMEM = {
  BATT_FULL_BMP, BATT_MID_BMP, BATT_LOW_BMP, BATT_CHARGE_BMP,
};
const uint8_t* const wifi_icon_table[] PROGMEM = {
  WIFI_STRONG_BMP, WIFI_MID_BMP, WIFI_WEAK_BMP, WIFI_OFFLINE_BMP,
};

ModeIcon  currentModeIcon  = ICON_MODE_CHAT_ROBOT;
BattState currentBatt      = BATT_FULL;
WifiState currentWifi      = WIFI_STRONG;

// ── (HC138 address logic removed — now using direct GPIO input) ──

// ================================================================
//  FRAME BITMAPS (128x48, MSB-first �?unchanged)
// ================================================================
static const uint8_t PROGMEM frame3[768] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x30,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0E,0x0C,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x82,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x01,0xFF,0xF8,0x00,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x00,0xC0,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x1F,0xFC,0xF8,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x01,0xE0,0x00,0x7F,0x80,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x7F,0x00,0x1F,0xB0,0xC0,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x01,0x9F,0x81,0xFF,0xE0,0x1B,0x80,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x00,0x00,0x31,0x0E,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x60,0x08,0x30,0x06,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x60,0x00,0x3F,0x03,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x02,0x00,0xC0,0x07,0xFC,0x02,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x03,0x01,0x80,0x01,0xF8,0x06,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0x00,0x00,0xE0,0x1C,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x07,0xF0,0x00,0x03,0xE0,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x00,0x0F,0x00,0x00,0x1C,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x01,0x83,0x80,0x00,0x01,0xFF,0xFF,0xFE,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x06,0x7E,0x00,0x00,0x00,0xC0,0x22,0x03,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x18,0x1C,0x00,0x00,0x01,0x80,0x27,0x1E,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x07,0xF8,0x00,0x01,0xFF,0xFF,0xFE,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xD8,0x00,0x04,0x00,0x02,0x66,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x06,0x22,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x0C,0x32,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x18,0x13,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x30,0x3C,0xC0,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x01,0xC0,0x00,0x00,0x00,0x60,0x60,0x60,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x78,0x06,0x00,0x00,0xC0,0x20,0x20,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x0F,0xFE,0x00,0x01,0x80,0xF0,0x60,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x06,0x03,0x3F,0xE0,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x0F,0xF0,0x01,0x00,0x0C,0x0C,0xDB,0x38,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x0F,0xCF,0xFF,0x80,0x3F,0xF9,0x99,0x04,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x02,0x0F,0xFC,0x01,0xFF,0xE0,0x03,0xED,0x86,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x16,0x0F,0xF0,0xFF,0xFF,0xFF,0xFC,0x67,0x86,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x40,0x0F,0xB0,0x00,0x00,0x00,0x0F,0x90,0x04,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x07,0x30,0x3F,0xFF,0xFF,0xF8,0x0E,0x38,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

static const uint8_t PROGMEM frame5[768] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x3F,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x3F,0xC3,0xFF,0xFE,0x00,0xC0,0x1D,0x9C,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x03,0xFF,0xC1,0xFF,0xFC,0x01,0xE0,0x0F,0xBF,0xF8,0x00,0x00,0x00,
  0x00,0x00,0x00,0x07,0xFF,0xE0,0x1F,0x00,0x00,0xE0,0x1F,0x30,0x3C,0x00,0x00,0x00,
  0x00,0x00,0x00,0x07,0xFF,0xE1,0xFF,0xFC,0x38,0xE7,0x13,0x9F,0x1C,0x00,0x00,0x00,
  0x00,0x00,0x00,0x07,0xFF,0xC3,0x9F,0x9E,0x78,0xE3,0x87,0x9B,0x9C,0x00,0x00,0x00,
  0x00,0x00,0x00,0x01,0xFF,0x83,0x9F,0xDE,0x70,0xE1,0x9F,0xDB,0xBC,0x00,0x00,0x00,
  0x00,0x00,0x00,0x0F,0xFF,0xF3,0x9F,0x9E,0x03,0xE0,0x01,0x9F,0x38,0x00,0x00,0x00,
  0x00,0x00,0x00,0x03,0xFD,0xC1,0xFF,0xFC,0x07,0xE0,0x0F,0x80,0xF8,0x00,0x00,0x00,
  0x00,0x00,0x00,0x07,0xBD,0xF0,0xFF,0xF8,0x00,0x00,0x02,0x00,0xF0,0x00,0x00,0x00,
  0x00,0x00,0x00,0x06,0x18,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x13,0x08,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x03,0x9A,0xEE,0x77,0x99,0xC0,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

const uint8_t* const frames[2] PROGMEM = { frame3, frame5 };

// ================================================================
//  EXPRESSION SYSTEM �?Removed
// ================================================================

// ================================================================
//  MODE STATE
// ================================================================
AppMode       currentMode      = MODE_CHAT_ROBOT;
AppMode       beforeSettings   = MODE_CHAT_ROBOT;  // mode before entering SETTINGS
unsigned long lastUserAction   = 0;                 // timeout tracker
#define TIMEOUT_MS          30000                   // 30s auto-return to CHAT-ROBOT
unsigned long lastModeSwitchMs = 0;                 // cooldown for B button mode switching
#define MODE_SWITCH_COOLDOWN_MS  1000               // 1000ms lockout after mode switch

// ── 省电模式：30s无操作熄屏，任意键唤醒 ──
bool          screenSleep       = false;             // true = OLED 已熄屏
bool          wakeRedraw       = false;             // 唤醒后强制重绘一次
#define SCREEN_SLEEP_MS     30000                   // 30s 无操作熄屏

// Countdown state
CdState       cdState         = CD_IDLE;
int           cdMinutes       = 25;                 // default 25 min
int           cdSeconds       = 0;
unsigned long cdStartMs       = 0;
unsigned long cdPausedRemain  = 0;                 // remaining ms when paused
unsigned long cdTotalMs       = 0;
bool          cdPomodoro      = false;             // tomato mode: focus+rest cycle
bool          cdIsRest        = false;             // true = rest phase, false = focus
int           cdPresetIdx     = 1;                 // 0=15, 1=25, 2=45, 3=60
const int     cdPresets[]     = {15, 25, 45, 60};
#define CD_PRESET_COUNT       4
#define CD_REST_MINUTES       5                    // tomato rest duration
unsigned long cdEfComboMs     = 0;                 // last time E+F both held
#define EF_COMBO_MS          600                   // E+F hold threshold for preset cycle

// TODO state
TodoSubState  todoSub         = TODO_LIST;
int           todoCursor      = 0;                 // selected task index
#define MAX_TASKS           10

// SETTINGS sub-page
enum SettingsSub { SETTINGS_MAIN, SETTINGS_BRIGHTNESS, SETTINGS_SLEEP, SETTINGS_AI };
SettingsSub settingsSub = SETTINGS_MAIN;

// SETTINGS adjustable values
int   settingsBrightness = 100;   // 0-100%
int   settingsSleepIdx   = 1;     // index into sleepOptions[]
int   settingsAiIdx      = 0;     // 0=Cloud, 1=Local, 2=Auto

// WEATHER state

WxPage        wxPage          = WX_TODAY;
int           wxDayCursor     = 0;

// SETTINGS state
int           settingsCursor  = 0;
#define SETTINGS_ITEMS     7

// CHAT-ROBOT sub-state
CrSubState    crSub           = CR_EXPR;

// ================================================================
//  BUTTON STATE MACHINE
// ================================================================
// Direct GPIO mapping (no HC138 decoder)
//  Index : GPIO Pin : Physical label : Logical role
//    0   :   GPIO0  : SW1 MODE       : buttons[0] B/Mode
//    1   :   GPIO1  : SW2 PLAY       : buttons[1] C/Main
//    2   :   GPIO2  : SW3 PREV       : buttons[2] D/Secondary
//    3   :   GPIO3  : SW4 VOL-       : buttons[3] E/Up/Decrease (double-tap = Shutdown)
//    4   :   GPIO4  : SW5 VOL+       : buttons[4] F/Down/Increase

struct Button {
  uint8_t raw;                 // current raw read: LOW = pressed, HIGH = not pressed
  uint8_t last;                // last debounced state: LOW = pressed, HIGH = not pressed
  bool pressed;                // just pressed (one-shot this frame)
  bool released;               // just released (one-shot this frame)
  unsigned long pressTime;      // when press started
  unsigned long releaseTime;    // when released
  bool longPress;              // long-press triggered (one-shot)
  bool doubleTap;              // double-tap detected (one-shot)
  unsigned long lastClickTime;  // for double-tap detection
  bool longPressActive;        // long press is currently active (held)
};

#define BTN_COUNT 5   // B, C, D, E, F
Button buttons[BTN_COUNT];

// Debounce / timing
// Timer-based debounce: raw reading must stay stable for DEBOUNCE_MS before accepted
#define DEBOUNCE_MS         50   // debounce settle time (ms) — conservative for noisy environments
#define LONG_PRESS_MS_B   2000   // B: enter SETTINGS (2s)
#define LONG_PRESS_MS_C    500   // C: voice input (0.5s)
#define DOUBLE_TAP_MS      500   // double-tap window (B & D)


// ===== Button scanning: scan all 6 buttons each loop, counter-based debounce =====
// 1. updateButtons() scans all 5 buttons (direct digitalRead)
// 2. Counter-based debounce: raw must be stable for N consecutive scans before accepted
// 3. Edge detection on stable state → set pressed/released/longPress/doubleTap
// 4. ISR removed — polling is sufficient and avoids ISR-side complexity

uint8_t btnStable[BTN_COUNT];        // debounced state: LOW=pressed, HIGH=released
uint8_t btnDebounceCnt[BTN_COUNT];   // consecutive same-reading counter
uint8_t btnPrevStable[BTN_COUNT];    // previous stable state for edge detection
uint8_t prevRead[BTN_COUNT];         // last raw digitalRead() per button


const uint8_t btnPins[BTN_COUNT] = { KEY_B_PIN, KEY_C_PIN, KEY_D_PIN, KEY_E_PIN, KEY_F_PIN };

void initButtons() {
  // 5 keys directly wired to GPIOs, other end to GND.
  // Use internal pull-up — pressed = LOW, released = HIGH.
  for (int i = 0; i < BTN_COUNT; i++) {
    pinMode(btnPins[i], INPUT_PULLUP);
  }

  unsigned long now = millis();
  for (int i = 0; i < BTN_COUNT; i++) {
    buttons[i].raw       = HIGH;
    buttons[i].last      = HIGH;
    buttons[i].pressed   = false;
    buttons[i].released  = false;
    buttons[i].longPress = false;
    buttons[i].doubleTap = false;
    buttons[i].pressTime = 0;
    buttons[i].releaseTime = 0;
    buttons[i].lastClickTime = 0;
    buttons[i].longPressActive = false;
    btnStable[i]        = HIGH;
    btnPrevStable[i]    = HIGH;
    btnDebounceCnt[i]   = 0;
    prevRead[i]         = HIGH;
  }
}

// Scan all 5 buttons (direct GPIO), debounce, and set edge flags.
// Called every loop(). No HC138 — just digitalRead each pin.
// Counter-based debounce: stable[] updates after N same readings.
// Edge detection runs on stable[].
//
// Logic: button pressed → GPIO pulled LOW (INPUT_PULLUP + connect to GND).
//        So: pressed = LOW, released = HIGH.
#define BTN_DEBOUNCE_CNT  4   // 4 consecutive identical reads -> accept (~20ms @5ms/loop)

// Separate tracking: prevRead[] = last raw reading (for change detection)
//                    btnDebounceCnt[] = consecutive same-reading counter

void updateButtons() {
  unsigned long now = millis();
  bool anyKeyJustPressed = false;

  for (int i = 0; i < BTN_COUNT; i++) {
    Button& b = buttons[i];

    // --- 1. Direct digitalRead (no HC138 address setup needed) ---
    uint8_t reading = digitalRead(btnPins[i]);  // LOW=pressed, HIGH=released

    // --- 3. Counter-based debounce ---
    if (reading == prevRead[i]) {
      // same as last raw reading → increment counter
      if (btnDebounceCnt[i] < BTN_DEBOUNCE_CNT)
        btnDebounceCnt[i]++;
    } else {
      // changed → reset counter, update prevRead
      prevRead[i]      = reading;
      btnDebounceCnt[i] = 0;
    }

    // --- 4. Accept as stable after N consecutive identical reads ---
    b.pressed   = false;
    b.released  = false;
    b.longPress = false;
    b.doubleTap = false;

    if (btnDebounceCnt[i] >= BTN_DEBOUNCE_CNT) {
      // reading is stable → update stable state
      btnPrevStable[i] = btnStable[i];
      btnStable[i]     = reading;  // LOW=pressed, HIGH=released
    }

    bool stablePressed = (btnStable[i] == LOW);
    b.raw = stablePressed ? LOW : HIGH;
    b.last = b.raw;  // keep .last in sync

    // --- 5. Edge detection ---
    if (stablePressed && btnPrevStable[i] == HIGH) {
      // Just pressed
      b.pressed   = true;
      b.pressTime = now;
      anyKeyJustPressed = true;

      // Double-tap: B(idx0)=mode, D(idx2)=reset, E(idx3)=shutdown
      if ((i == 0 || i == 2 || i == 3) && b.lastClickTime > 0
           && (now - b.lastClickTime < DOUBLE_TAP_MS)) {
        b.doubleTap     = true;
        b.lastClickTime = 0;
      }
    }
    else if (!stablePressed && btnPrevStable[i] == LOW) {
      // Just released
      b.released    = true;
      b.releaseTime  = now;

      if ((i == 0 || i == 2 || i == 3) && !b.longPressActive) {
        b.lastClickTime = now;
      }
      b.longPressActive = false;
    }

    // --- 6. Long-press detection ---
    if (stablePressed && !b.longPressActive) {
      unsigned long threshold;
      switch (i) {
        case 0: threshold = LONG_PRESS_MS_B; break;  // B: enter settings (2s)
        case 1: threshold = LONG_PRESS_MS_C; break;  // C: voice input (0.5s)
        default: threshold = 3000; break;
      }
      if (now - b.pressTime >= threshold) {
        b.longPress       = true;
        b.longPressActive = true;
      }
    }
  }

  // ── 省电模式：任意键唤醒 ──
  if (anyKeyJustPressed && screenSleep) {
    screenSleep = false;
    wakeRedraw = true;  // 强制重绘
    display.ssd1306_command(0x8D);  // CHARGE PUMP ON
    display.ssd1306_command(0x14);
    display.ssd1306_command(0xAF);  // DISPLAY ON
    lastUserAction = now;
    Serial.println(F("[PWR] Wake by key"));
  }
  if (anyKeyJustPressed && !screenSleep) {
    lastUserAction = now;  // 有按键操作，重置熄屏计时
  }

  // (No HC138 to deselect — all pins stay as INPUT_PULLUP)
}
//反馈（硬件未焊接，留空）
void feedbackClick() {
  // LED / buzzer not populated on PCB
}

// ================================================================
//  DRAWING HELPERS
// ================================================================

// Print regular string centred
void printCentred(const char* str, int y) {
  int len = strlen(str);
  int x = (SCREEN_W - len * 6) / 2;
  if (x < 0) x = 0;
  display.setCursor(x, y);
  display.print(str);
}

// Draw PROGMEM icon (bitmap)
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

// Draw line of text at y, optionally highlighted (inverted)
void printLine(int y, const char* str, bool hi) {
  display.setCursor(2, y);
  if (hi) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  } else {
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  }
  display.print(str);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK); // reset
}

// ================================================================
//  STATUS BAR  (9px tall: y=0~8)
//  Layout:
//   [mode icon 12x8] [  HH:MM  ] [wifi 12x8] [batt 12x8]
//   x=0~12            x=13~103    x=104~115   x=116~127
//   Divider line at y=8 bottom of status bar
// ================================================================
void drawStatusBar() {
  display.fillRect(0, 0, SCREEN_W, STATUSBAR_H, SSD1306_BLACK);

  // ── Mode icon (12x8, positioned at y=0) ──
  const uint8_t* modeBmp = (const uint8_t*)pgm_read_ptr(&mode_icon_table[currentModeIcon]);
  drawIcon(MODE_ICON_X, ICON_Y, modeBmp, 12, 8);

  // ── Clock text "HH:MM" �?centred between icon and wifi icon ──
  // Available space: x=13..89 = 77px. If statusBarTag is set (e.g., "RUN"),
  // print it first and shift time right.
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  if (statusBarTag[0] != '\0') {
    display.setCursor(14, 1);                // tag at x=14 (right of mode icon)
    display.print(statusBarTag);
    display.setCursor(40, 1);                // time shifted right
  } else {
    display.setCursor(36, 1);                // normal centred
  }
  display.print(timeStr);

  // ── WiFi icon ──
  const uint8_t* wifiBmp = (const uint8_t*)pgm_read_ptr(&wifi_icon_table[currentWifi]);
  drawIcon(WIFI_ICON_X, ICON_Y, wifiBmp, 12, 8);

  // ── Battery icon ──
  const uint8_t* battBmp = (const uint8_t*)pgm_read_ptr(&batt_icon_table[currentBatt]);
  drawIcon(BATT_ICON_X, ICON_Y, battBmp, 12, 8);

  // ── Divider line at y=8 ──
  display.drawFastHLine(0, STATUSBAR_H - 1, SCREEN_W, SSD1306_WHITE);
}

// Set statusBarTag based on current mode state (called before drawStatusBar)
void updateStatusBarTag() {
  statusBarTag[0] = '\0';        // default: no tag
  if (currentMode == MODE_COUNTDOWN) {
    switch (cdState) {
      case CD_RUNNING: strcpy(statusBarTag, cdIsRest ? "RST" : "RUN"); break;
      case CD_PAUSED:  strcpy(statusBarTag, cdIsRest ? "RPA" : "PAU"); break;
      case CD_DONE:    strcpy(statusBarTag, "DON"); break;
      default: break;  // CD_IDLE: no tag
    }
  }
}

// Update mode icon to match current mode
void updateModeIcon() {
  switch (currentMode) {
    case MODE_CLOCK:     currentModeIcon = ICON_MODE_CLOCK;      break;
    case MODE_COUNTDOWN: currentModeIcon = ICON_MODE_COUNTDOWN;  break;
    case MODE_CHAT_ROBOT:currentModeIcon = ICON_MODE_CHAT_ROBOT; break;
    case MODE_TODO:      currentModeIcon = ICON_MODE_TODO;       break;
    case MODE_WEATHER:   currentModeIcon = ICON_MODE_WEATHER;    break;
    case MODE_SETTINGS:  currentModeIcon = ICON_MODE_SETTINGS;   break;
  }
}

// ================================================================
//  MODE TRANSITION ANIMATION (slide left/right, 200ms)
// ================================================================
void transitSlide(int direction) {
  // direction: 1 = slide right (next mode), -1 = slide left (prev mode)
  // We'll capture current screen, shift it out, render new mode shifted in.
  // Simplified: just clear and redraw (200ms flash)
  // TODO: implement proper framebuffer-based slide
}

// Enter a mode and reset its sub-state
// Cooldown enforced: no mode switch within MODE_SWITCH_COOLDOWN_MS
void enterMode(AppMode newMode) {
  if (newMode == currentMode) return;

  unsigned long now = millis();
  if (now - lastModeSwitchMs < MODE_SWITCH_COOLDOWN_MS) {
    // Within cooldown — silently ignore
    return;
  }

  // Slide animation
  // transitSlide(newMode > currentMode ? 1 : -1);

  currentMode = newMode;
  updateModeIcon();
  lastUserAction    = now;
  lastModeSwitchMs  = now;  // start cooldown

  // Reset sub-states
  switch (newMode) {
    case MODE_CHAT_ROBOT:
      break;
    case MODE_COUNTDOWN:
      if (cdState != CD_RUNNING) {
        cdState   = CD_IDLE;
        cdIsRest  = false;
      }
      break;
    case MODE_TODO:
      todoSub = TODO_LIST;
      todoCursor = 0;
      break;
    case MODE_WEATHER:
      wxPage = WX_TODAY;
      break;
    case MODE_SETTINGS:
      settingsCursor = 0;
      settingsSub    = SETTINGS_MAIN;
      break;
    default: break;
  }

  // Clear content area immediately
  display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, SSD1306_BLACK);
}

// B button: cycle modes
void cycleMode() {
  // SETTINGS is not in the cycle �?B in SETTINGS exits to CHAT-ROBOT
  // Cycle: CHAT-ROBOT �?CLOCK �?COUNTDOWN �?TODO �?WEATHER �?CHAT-ROBOT

  AppMode next = currentMode;
  switch (currentMode) {
    case MODE_CHAT_ROBOT: next = MODE_CLOCK;      break;
    case MODE_CLOCK:      next = MODE_COUNTDOWN;  break;
    case MODE_COUNTDOWN:  next = MODE_TODO;       break;
    case MODE_TODO:       next = MODE_WEATHER;    break;
    case MODE_WEATHER:    next = MODE_CHAT_ROBOT; break;
    default:              next = MODE_CHAT_ROBOT; break;
  }
  enterMode(next);
}

// ================================================================
//  CONTENT FRAME DRAWING (animation)
// ================================================================
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
  drawStatusBar();
  drawContentFrame(bmp, offsetX);
  display.display();
}

// ================================================================
//  ANIMATION PLAY
// ================================================================
#define STEPS_ENTER  16
#define STEPS_EXIT   16
#define DELAY_MS     90
#define HOLD5_MS    3000

void playAnimation() {
  const uint8_t* bmp3 = (const uint8_t*)pgm_read_ptr(&frames[0]);
  const uint8_t* bmp5 = (const uint8_t*)pgm_read_ptr(&frames[1]);

  // Frame 3: slide in �?hold �?slide out
  for (int s = 0; s <= STEPS_ENTER; s++) {
    drawFullScreen(bmp3, map(s, 0, STEPS_ENTER, -SCREEN_W, 0));
    delay(DELAY_MS);
  }
  delay(250);
  for (int s = 0; s <= STEPS_EXIT; s++) {
    drawFullScreen(bmp3, map(s, 0, STEPS_EXIT, 0, SCREEN_W));
    delay(DELAY_MS);
  }

  // Frame 5: slide in �?hold �?slide out
  for (int s = 0; s <= STEPS_ENTER; s++) {
    drawFullScreen(bmp5, map(s, 0, STEPS_ENTER, -SCREEN_W, 0));
    delay(DELAY_MS);
  }
  unsigned long holdStart = millis();
  while (millis() - holdStart < HOLD5_MS) {
    if (millis() - lastTimeUpdate >= TIME_REFRESH_MS) {
      updateTime();
      drawStatusBar();
      drawContentFrame(bmp5, 0);
      display.display();
      lastTimeUpdate = millis();
    }
    delay(100);
  }
  for (int s = 0; s <= STEPS_EXIT; s++) {
    drawFullScreen(bmp5, map(s, 0, STEPS_EXIT, 0, SCREEN_W));
    delay(DELAY_MS);
  }
}

// ================================================================
//  WIFI & NTP
// ================================================================
void connectWiFi() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("WiFi:"));
  display.println(WIFI_SSID);               // 显示要连�?WiFi �?
  display.println();
  display.print(F("Scanning 2.4G..."));
  display.display();

  // 先扫描看看能不能找到这个 SSID
  bool found = false;
  int n = WiFi.scanNetworks();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(n);
  display.println(F(" APs found"));
  for (int i = 0; i < n && i < 8; i++) {
    display.print(WiFi.SSID(i).substring(0, 14));
    display.print(F(" ("));
    display.print(WiFi.RSSI(i));
    display.println(F(")"));
    if (WiFi.SSID(i) == String(WIFI_SSID)) found = true;
  }
  if (!found) {
    display.println();
    display.println(F(">> SSID NOT IN LIST!"));
    display.println(F("Check: 2.4G only"));
    display.display();
    delay(4000);
    return;
  }

  // 找到了，开始连�?
  display.clearDisplay();
  display.setCursor(0, 24);
  display.print(F("Connecting..."));
  display.display();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    display.setCursor(0, 40);
    display.print('.');
    display.display();
    delay(500);
  }

  display.clearDisplay();
  display.setCursor(0, 24);
  if (WiFi.status() == WL_CONNECTED) {
    display.print(F("WiFi OK!"));
    display.setCursor(0, 36);
    display.print(F("IP:"));
    display.print(WiFi.localIP());
    display.display();
    delay(800);
  } else {
    display.print(F("WiFi FAIL"));
    display.setCursor(0, 36);
    display.print(F("Wrong password?"));
    display.display();
    delay(3000);
  }
}


// ── WiFi signal strength monitor (phone-style) ──────────
// Call every 3s. Reads RSSI and maps to WifiState.
void updateWifiState() {
  if (WiFi.status() != WL_CONNECTED) {
    currentWifi = WIFI_OFFLINE;
    return;
  }
  int32_t rssi = WiFi.RSSI();  // dBm, e.g. -55
  if      (rssi >= -50) currentWifi = WIFI_STRONG;
  else if (rssi >= -65) currentWifi = WIFI_MID;
  else if (rssi >= -80) currentWifi = WIFI_WEAK;
  else                    currentWifi = WIFI_OFFLINE;
}

// ── Battery state simulation ────────────────────────────
// TODO: replace with ADC reading from VBAT divider when hardware supports it
// Cycles FULL→MID→LOW every 10 minutes for demo
void updateBattState() {
  static unsigned long lastBattUpdate = 0;
  unsigned long now = millis();
  if (now - lastBattUpdate < 600000) return;  // every 10 min
  lastBattUpdate = now;
  currentBatt = (BattState)(((int)currentBatt + 1) % 3);  // cycle FULL/MID/LOW
}

// 更新北京时间 (UTC+8) �?�?NTP 自动重试
void updateTime() {
  unsigned long now = millis();

  // 检�?WiFi 状态并更新图标
  if (WiFi.status() == WL_CONNECTED) {
    // currentWifi updated by updateWifiState()
  } else {
    currentWifi = WIFI_OFFLINE;
    // WiFi 离线时也尝试获取已有的系统时间（可能之前同步成功过）
    struct tm ti;
    if (timeSynced && getLocalTime(&ti)) {
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d",
               ti.tm_hour, ti.tm_min);
    }
    return;
  }

  struct tm ti;
  if (getLocalTime(&ti)) {
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d",
             ti.tm_hour, ti.tm_min);
    timeSynced = true;
  } else if (!timeSynced || now - lastNtpRetry > NTP_RETRY_MS) {
    // 未同步过，或距上次重试已超时 �?重新请求 NTP
    configTime(GMT_OFFSET, DAYLIGHT,
               NTP_SERVER, "ntp1.aliyun.com", "pool.ntp.org");
    lastNtpRetry = now;

    // �?1.5 秒让 NTP 完成首次同步
    unsigned long waitStart = millis();
    while (millis() - waitStart < 1500) {
      if (getLocalTime(&ti)) {
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d",
                 ti.tm_hour, ti.tm_min);
        timeSynced = true;
        break;
      }
      delay(100);
    }
  }
}

// ── CHAT-ROBOT idle
void renderChatRobotIdle() {
  display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Title
  printCentred("CHAT-ROBOT", CONTENT_Y + 12);

  // Status hint
  printCentred("Ready", CONTENT_Y + 26);

  // Bottom hint
  display.setCursor(2, CONTENT_Y + 42);
  display.print(F("Hold C:Voice  B:Mode"));
}

// ================================================================
//  MODE UI RENDER FUNCTIONS
// ================================================================

// ── CLOCK Mode ──────────────────────────────────────────
//  Content area: y=9~63 (55px total)
//  Layout (design spec §3):
//   y=9  ─────── divider (drawn by status bar)
//   y=12  Big time "HH:MM" textSize=2, chars spaced, centred  (zone y=9~34)
//   y=36  Date + weekday: "YYYY-MM-DD Weekday"  (zone y=34~46)
//   y=46  ─── separator line ─────────────────────────────
//   y=50  Bottom info bar: "Sun 26C | TODO:3"  (zone y=46~63)
//
//  Colon blink: odd second �?show colon, even second �?space
// ────────────────────────────────────────────────────────
void renderClockMode() {
  display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, SSD1306_BLACK);
  display.setTextColor(SSD1306_WHITE);

  // ─── BIG TIME (textSize=2, spaced characters) ───────────
  {
    int yBig = CONTENT_Y + 4;
    display.setTextSize(2);
    const int charW = 12;   // textSize=2 char cell width
    const int nChars = 5;   // "HH:MM"
    int gap = (SCREEN_W - nChars * charW) / (nChars + 1);
    if (gap < 2) gap = 2;
    int x = gap;

    const char* src = (timeSynced && timeStr[0] != '-') ? timeStr : "--:--";

    // Colon blink: get current second
    bool colonVisible = true;
    if (timeSynced) {
      struct tm ti2;
      if (getLocalTime(&ti2)) colonVisible = (ti2.tm_sec % 2 == 0);
    }

    for (int i = 0; i < nChars; i++) {
      display.setCursor(x, yBig);
      if (i == 2 && !colonVisible) {
        display.print(' ');  // blink colon off
      } else {
        display.print(src[i]);
      }
      x += charW + gap;
    }
    display.setTextSize(1);
  }

  // ─── DATE + WEEKDAY ─────────────────────────────────────
  {
    int yDate = CONTENT_Y + 27;
    if (timeSynced && timeStr[0] != '-') {
      struct tm ti;
      if (getLocalTime(&ti)) {
        const char* wday[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        char buf[24];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %s",
                 ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday,
                 wday[ti.tm_wday]);
        printCentred(buf, yDate);
      }
    } else {
      printCentred("NTP syncing...", yDate);
    }
  }

  // ─── SEPARATOR LINE ─────────────────────────────────────
  display.drawFastHLine(2, CONTENT_Y + 36, SCREEN_W - 4, SSD1306_WHITE);

  // ─── BOTTOM INFO BAR ────────────────────────────────────
  {
    int yInfo = CONTENT_Y + 40;
    const char* seg1 = "Sun 26C";
    const char* seg2 = "TODO:3";

    int w1 = strlen(seg1) * 6;
    int total = w1 + 6 + 2 + (int)strlen(seg2) * 6;  // seg1 + " | " + seg2
    int startX = max(0, (SCREEN_W - total) / 2);

    display.setCursor(startX, yInfo);
    display.print(seg1);
    display.setCursor(startX + w1 + 2, yInfo);
    display.print('|');
    display.setCursor(startX + w1 + 8, yInfo);
    display.print(seg2);
  }

  // ─── BOTTOM HINT ────────────────────────────────────────
  display.setCursor(2, CONTENT_Y + 48);
  display.print(F("C:Tasks  D:TODO"));
}

// ── COUNTDOWN Helpers ──────────────────────────────────
// Draw a 120x4 progress bar at given y, optional flash (last-minute blink)
void drawCdProgressBar(int y, float pct, bool flash) {
  int barX = 4;
  int barW = 120;
  display.drawRect(barX, y, barW, 4, SSD1306_WHITE);
  int fillW = (int)((barW - 2) * constrain(pct, 0.0f, 1.0f));
  if (fillW > 0 && (!flash || (millis() % 1000 < 500))) {
    display.fillRect(barX + 1, y + 1, fillW, 2, SSD1306_WHITE);
  }
}

// Calculate end time string from remaining seconds
void getCdEndTime(char* buf, int bufSize, unsigned long remainMs) {
  struct tm ti;
  if (getLocalTime(&ti)) {
    unsigned long totalSec = ti.tm_hour * 3600UL + ti.tm_min * 60UL
                           + ti.tm_sec + (remainMs / 1000UL);
    int endH = (totalSec / 3600) % 24;
    int endM = (totalSec / 60) % 60;
    snprintf(buf, bufSize, "End: %02d:%02d", endH, endM);
  } else {
    snprintf(buf, bufSize, "End: --:--");
  }
}

// ── COUNTDOWN Mode ──────────────────────────────────────
//  Layout (design spec §3 + mockups):
//   CD_IDLE:    大字 MM:00  y=CONTENT_Y+3
//               "Focus Timer"  y=CONTENT_Y+22
//               空进度条  y=CONTENT_Y+32
//               "B:+1m  C:-1m  F:Start"  y=CONTENT_Y+44
//   CD_RUNNING/PAUSED:
//               大字 MM:SS  y=CONTENT_Y+1
//               状态文�? y=CONTENT_Y+20
//               进度�? y=CONTENT_Y+30
//               结束时间  y=CONTENT_Y+38
//               按键提示  y=CONTENT_Y+48
//   CD_DONE:    (^_^)  y=CONTENT_Y+3
//               "Done!"  y=CONTENT_Y+23
//               "X min focused!"  y=CONTENT_Y+34
//               "C:Again"  y=CONTENT_Y+46
// ────────────────────────────────────────────────────────
void renderCountdownMode() {
  display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, SSD1306_BLACK);
  display.setTextColor(SSD1306_WHITE);

  // === CD_IDLE: 空闲/待启�?===
  if (cdState == CD_IDLE) {
    // Large preset time  MM:00
    char buf[12];
    snprintf(buf, sizeof(buf), "%02d:00", cdMinutes);
    display.setTextSize(2);
    printCentred(buf, CONTENT_Y + 3);
    display.setTextSize(1);

    // Phase label
    printCentred("Focus Timer", CONTENT_Y + 23);

    // Empty progress bar (y=32 per spec)
    drawCdProgressBar(CONTENT_Y + 32, 0.0f, false);

    // Button hints: B=+1m, C=-1m, F=Start
    display.setCursor(4, CONTENT_Y + 44);
    display.print(F("B:+1m  C:-1m  F:Start"));
  }
  // === CD_RUNNING / CD_PAUSED (focus or rest) ===
  else if (cdState == CD_RUNNING || cdState == CD_PAUSED) {
    unsigned long remainMs;
    if (cdState == CD_PAUSED) {
      remainMs = cdPausedRemain;
    } else {
      unsigned long elapsed = millis() - cdStartMs;
      remainMs = (elapsed < cdTotalMs) ? (cdTotalMs - elapsed) : 0;
    }
    int mins = (int)(remainMs / 60000);
    int secs = (int)((remainMs / 1000) % 60);

    // Large countdown MM:SS
    char buf[12];
    snprintf(buf, sizeof(buf), "%02d:%02d", mins, secs);
    display.setTextSize(2);
    printCentred(buf, CONTENT_Y + 1);
    display.setTextSize(1);

    // State label
    const char* label;
    if (cdIsRest) {
      label = (cdState == CD_PAUSED) ? "Rest Paused" : "Rest...";
    } else {
      label = (cdState == CD_PAUSED) ? "Paused" : "Running...";
    }
    printCentred(label, CONTENT_Y + 20);

    // Progress bar at y=30 (last-minute flash)
    float pct = (cdTotalMs > 0) ? (1.0f - (float)remainMs / (float)cdTotalMs) : 0.0f;
    bool flash = (remainMs < 60000);
    drawCdProgressBar(CONTENT_Y + 30, pct, flash);

    // End time
    char endBuf[16];
    getCdEndTime(endBuf, sizeof(endBuf), remainMs);
    display.setCursor(4, CONTENT_Y + 38);
    display.print(endBuf);

    // Button hints
    display.setCursor(4, CONTENT_Y + 48);
    if (cdState == CD_RUNNING) {
      display.print(F("D:Pause  DD:Reset"));
    } else {
      display.print(F("D:Resume  DD:Reset"));
    }
  }
  // === CD_DONE: 完成 ===
  else if (cdState == CD_DONE) {
    display.setTextSize(2);
    printCentred("(^_^)", CONTENT_Y + 3);
    display.setTextSize(1);

    printCentred("Done!", CONTENT_Y + 23);

    char buf[24];
    snprintf(buf, sizeof(buf), "%d min focused!", cdMinutes);
    printCentred(buf, CONTENT_Y + 34);

    display.setCursor(4, CONTENT_Y + 46);
    display.print(F("C:Again"));
  }
}

// ── TODO Mode ───────────────────────────────────────────
//  Layout (design spec §3):
//   List state:  title + 4 rows (selected inverted) + hint
//   Detail state: task title + priority + deadline + est. time + hint
//   Empty state: empty notice + hint
// ────────────────────────────────────────────────────────
void renderTodoMode() {
  display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Placeholder task data (replace with MQTT-synced real data later)
  struct TaskItem { const char* title; const char* priority; const char* deadline; int estMin; };
  static const TaskItem tasks[] = {
    { "1.Finish math HW",   "High",   "18:00 Today",  45 },
    { "2.Review mod elec",  "Mid",    "Tomorrow",     60 },
    { "3.Prep lab report",  "Low",    "2026-06-01",   30 },
  };
  const int taskCount = 3;  // set to 0 to see empty state

  // ── EMPTY STATE ──────────────────────────────────────
  if (taskCount == 0) {
    printCentred("[ Empty ]",   CONTENT_Y + 10);
    printCentred("No tasks!",   CONTENT_Y + 22);
    printCentred("Add via app", CONTENT_Y + 34);
    display.setCursor(2, CONTENT_Y + 46);
    display.print(F("B:Mode"));
    return;
  }

  // ── DETAIL STATE ─────────────────────────────────────
  if (todoSub == TODO_DETAIL) {
    int i = todoCursor;
    // Title (truncate at 20 chars)
    display.setCursor(2, CONTENT_Y + 1);
    display.print(tasks[i].title);

    // Fields
    display.setCursor(2, CONTENT_Y + 11);
    display.print(F("Pri: ")); display.print(tasks[i].priority);

    display.setCursor(2, CONTENT_Y + 20);
    display.print(F("Due: ")); display.print(tasks[i].deadline);

    display.setCursor(2, CONTENT_Y + 29);
    char buf[20];
    snprintf(buf, sizeof(buf), "Est: %d min", tasks[i].estMin);
    display.print(buf);

    // Separator
    display.drawFastHLine(2, CONTENT_Y + 38, SCREEN_W - 4, SSD1306_WHITE);

    // Hints
    display.setCursor(2, CONTENT_Y + 42);
    display.print(F("C:Start Timer"));
    display.setCursor(2, CONTENT_Y + 51);
    display.print(F("D:Back"));
    return;
  }

  // ── LIST STATE ───────────────────────────────────────
  // Title bar
  char title[20];
  snprintf(title, sizeof(title), "TODO (%d)", taskCount);
  display.setCursor(2, CONTENT_Y);
  display.print(title);

  // Task rows (up to 4 visible)
  int visStart = 0;
  if (todoCursor >= 4) visStart = todoCursor - 3;
  for (int i = visStart; i < taskCount && i < visStart + 4; i++) {
    bool hi = (i == todoCursor);
    int rowY = CONTENT_Y + 10 + (i - visStart) * 9;
    // Truncate title to fit
    char row[22];
    snprintf(row, sizeof(row), "%s", tasks[i].title);
    printLine(rowY, row, hi);
  }

  // Button hints
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(2, CONTENT_Y + 48);
  display.print(F("E/F:Sel  C:Detail D:Done"));
}

// ── WEATHER Mode ────────────────────────────────────────
//  Page 1 (WX_TODAY):  City + large temp + desc + humidity + wind + hint
//  Page 2 (WX_FORECAST): 3-day forecast list
// ────────────────────────────────────────────────────────
void renderWeatherMode() {
  display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (wxPage == WX_TODAY) {
    // City
    printCentred("Wuhan", CONTENT_Y + 1);

    // Large temperature
    display.setTextSize(2);
    printCentred("26 C", CONTENT_Y + 10);
    display.setTextSize(1);

    // Weather desc
    printCentred("Sunny", CONTENT_Y + 27);

    // Details line
    printCentred("Hum:65%  Wind:N3", CONTENT_Y + 36);

    // Feels like
    printCentred("Feels: 28C", CONTENT_Y + 45);

    // Hint
    display.setCursor(2, CONTENT_Y + 53);
    display.print(F("C:Forecast"));
  }
  else {  // WX_FORECAST
    // Header
    display.setCursor(2, CONTENT_Y + 1);
    display.print(F("3-Day Forecast"));
    display.drawFastHLine(2, CONTENT_Y + 9, SCREEN_W - 4, SSD1306_WHITE);

    // 3 rows: Date | Weather | Temp range
    const char* days[]   = { "Today", "Tomorrow", "Sat"   };
    const char* weathers[]= { "Sunny", "Cloudy",  "Rain"  };
    const char* temps[]  = { "22~28C","18~24C",  "15~20C" };

    for (int i = 0; i < 3; i++) {
      bool hi = (i == wxDayCursor);
      int rowY = CONTENT_Y + 11 + i * 12;
      char row[24];
      snprintf(row, sizeof(row), "%-8s %-6s %s", days[i], weathers[i], temps[i]);
      printLine(rowY, row, hi);
    }
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);

    // Hint
    display.setCursor(2, CONTENT_Y + 48);
    display.print(F("E/F:Day  D:Today"));
  }
}

// ── SETTINGS Mode ───────────────────────────────────────
//  Main menu: E/F scroll, C confirm/enter sub-page, D return to CHAT-ROBOT
//  Sub-pages: Brightness, Sleep Timeout, AI Mode
// ────────────────────────────────────────────────────────
void renderSettingsMode() {
  display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  const char* sleepLabels[] = {"15s","30s","60s","120s","Off"};
  const char* aiLabels[]    = {"Cloud(DS)","Local(qwen)","Auto"};

  // ── MAIN MENU ──
  if (settingsSub == SETTINGS_MAIN) {
    display.setCursor(2, CONTENT_Y);
    display.print(F("Settings"));
    display.drawFastHLine(2, CONTENT_Y + 8, SCREEN_W - 4, SSD1306_WHITE);

    const char* items[SETTINGS_ITEMS] = {
      "WiFi Setup",
      "Brightness",
      "Volume",
      "Sound/LED Mode",
      "Sleep Timeout",
      "AI Mode",
      "Device Info",
    };

    // Show up to 5 visible rows with scroll
    int visStart = 0;
    if (settingsCursor >= 5) visStart = settingsCursor - 4;
    for (int i = visStart; i < SETTINGS_ITEMS && i < visStart + 5; i++) {
      bool hi = (i == settingsCursor);
      printLine(CONTENT_Y + 10 + (i - visStart) * 8, items[i], hi);
    }
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(2, CONTENT_Y + 50);
    display.print(F("E/F:Sel  C:Enter  D:Back"));
  }
  // ── BRIGHTNESS SUB ──
  else if (settingsSub == SETTINGS_BRIGHTNESS) {
    display.setCursor(2, CONTENT_Y + 1);
    display.print(F("Brightness"));
    display.drawFastHLine(2, CONTENT_Y + 9, SCREEN_W - 4, SSD1306_WHITE);

    char buf[20];
    snprintf(buf, sizeof(buf), "%d%%", settingsBrightness);
    display.setTextSize(2);
    printCentred(buf, CONTENT_Y + 15);
    display.setTextSize(1);

    // Bar
    int barW = map(settingsBrightness, 0, 100, 0, 116);
    display.drawRect(4, CONTENT_Y + 33, 120, 6, SSD1306_WHITE);
    if (barW > 0) display.fillRect(5, CONTENT_Y + 34, barW, 4, SSD1306_WHITE);

    display.setCursor(2, CONTENT_Y + 46);
    display.print(F("E:- F:+ C:Save D:Back"));
  }
  // ── SLEEP TIMEOUT SUB ──
  else if (settingsSub == SETTINGS_SLEEP) {
    const int sleepCount = 5;
    display.setCursor(2, CONTENT_Y + 1);
    display.print(F("Sleep Timeout"));
    display.drawFastHLine(2, CONTENT_Y + 9, SCREEN_W - 4, SSD1306_WHITE);

    for (int i = 0; i < sleepCount; i++) {
      bool hi = (i == settingsSleepIdx);
      char row[20];
      snprintf(row, sizeof(row), "%s %s", hi ? ">" : " ", sleepLabels[i]);
      printLine(CONTENT_Y + 12 + i * 8, row, hi);
    }
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(2, CONTENT_Y + 50);
    display.print(F("E/F:Sel  C:Save  D:Back"));
  }
  // ── AI MODE SUB ──
  else if (settingsSub == SETTINGS_AI) {
    const int aiCount = 3;
    display.setCursor(2, CONTENT_Y + 1);
    display.print(F("AI Mode"));
    display.drawFastHLine(2, CONTENT_Y + 9, SCREEN_W - 4, SSD1306_WHITE);

    for (int i = 0; i < aiCount; i++) {
      bool hi = (i == settingsAiIdx);
      char row[22];
      snprintf(row, sizeof(row), "%s %s", hi ? ">" : " ", aiLabels[i]);
      printLine(CONTENT_Y + 12 + i * 10, row, hi);
    }
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(2, CONTENT_Y + 50);
    display.print(F("E/F:Sel  C:Save  D:Back"));
  }
}

// ================================================================
//  MODE RENDER DISPATCH
// ================================================================
void renderCurrentMode() {
  switch (currentMode) {
    case MODE_CLOCK:      renderClockMode();      break;
    case MODE_COUNTDOWN:  renderCountdownMode();   break;
    case MODE_TODO:       renderTodoMode();        break;
    case MODE_WEATHER:    renderWeatherMode();     break;
    case MODE_SETTINGS:   renderSettingsMode();    break;
    case MODE_CHAT_ROBOT: renderChatRobotIdle();     break;
  }
}

// ================================================================
//  BUTTON HANDLERS PER MODE
// ================================================================

// ── CHAT-ROBOT button handling ─────────────────────────
void handleChatRobotButtons() {
  Button& bB = buttons[0];
  Button& bC = buttons[1];
  Button& bD = buttons[2];
  Button& bF = buttons[4];
    Button& bE = buttons[3];

  // C long press: voice input (future)
  if (bC.longPress) {
    // crSub = CR_LISTEN;  // TODO: implement voice input
    lastUserAction = millis();
    feedbackClick();
  }
  // C short press: placeholder
  if (bC.released) {
    lastUserAction = millis();
    feedbackClick();
  }
  // E/F: placeholder for future use
  if (bE.pressed) { lastUserAction = millis(); feedbackClick(); }
  if (bF.pressed) { lastUserAction = millis(); feedbackClick(); }
  // D: placeholder
  if (bD.pressed) { lastUserAction = millis(); feedbackClick(); }
}

// ── CLOCK button handling ──────────────────────────────
void handleClockButtons() {
  Button& bC = buttons[1];
  Button& bD = buttons[2];

  if (bC.pressed) {
    // TODO: show task reminder popup
    feedbackClick();
  }
  if (bD.pressed) {
    enterMode(MODE_TODO);
    feedbackClick();
  }
}

// ── COUNTDOWN button handling ──────────────────────────
void handleCountdownButtons() {
  Button& bB = buttons[0];
  Button& bC = buttons[1];
  Button& bD = buttons[2];
  Button& bF = buttons[4];
  unsigned long now = millis();

  // Check E+F simultaneous hold (preset cycling in IDLE)
  bool eHeld = (buttons[3].raw == LOW);
  bool fHeld = (buttons[4].raw == LOW);

  // ── CD_IDLE ──
  if (cdState == CD_IDLE) {
    // B: +1 minute (max 120)
    if (bB.pressed && cdMinutes < 120) {
      cdMinutes++; cdPresetIdx = -1;
      lastUserAction = now; feedbackClick();
    }
    // C: -1 minute (min 1)
    if (bC.pressed && cdMinutes > 1) {
      cdMinutes--; cdPresetIdx = -1;
      lastUserAction = now; feedbackClick();
    }
    // E+F hold: cycle presets 15�?5�?5�?0
    if (eHeld && fHeld) {
      if (cdEfComboMs == 0) {
        cdEfComboMs = now;
      } else if (now - cdEfComboMs >= EF_COMBO_MS) {
        cdEfComboMs = now;
        cdPresetIdx = (cdPresetIdx + 1) % CD_PRESET_COUNT;
        cdMinutes = cdPresets[cdPresetIdx];
        lastUserAction = now; feedbackClick();
      }
    } else {
      cdEfComboMs = 0;
    }
    // F: Start countdown
    if (bF.pressed) {
      cdState    = CD_RUNNING;
      cdIsRest   = false;
      cdTotalMs  = cdMinutes * 60000UL;
      cdStartMs  = now;
      lastUserAction = now; feedbackClick();
    }
  }
  // ── CD_RUNNING ──
  else if (cdState == CD_RUNNING) {
    // D: Pause
    if (bD.pressed) {
      cdState = CD_PAUSED;
      cdPausedRemain = cdTotalMs - (now - cdStartMs);
      lastUserAction = now; feedbackClick();
    }
    // DD: Reset to IDLE
    if (bD.doubleTap) {
      cdState = CD_IDLE; cdPausedRemain = 0;
      cdIsRest = false;
      lastUserAction = now; feedbackClick();
    }
    // C long-press (~1s): Cancel / abandon countdown
    static bool cdCancelFired = false;
    if (bC.last == LOW && (now - bC.pressTime >= 1000)) {
      if (!cdCancelFired) {
        cdCancelFired = true;
        cdState = CD_IDLE; cdPausedRemain = 0;
        cdIsRest = false;
        lastUserAction = now;
        // Double-beep for cancel
        digitalWrite(LED_PIN, HIGH); delay(50); digitalWrite(LED_PIN, LOW);
        delay(80);
        digitalWrite(LED_PIN, HIGH); delay(50); digitalWrite(LED_PIN, LOW);
      }
    } else {
      cdCancelFired = false;
    }
  }
  // ── CD_PAUSED ──
  else if (cdState == CD_PAUSED) {
    // D: Resume
    if (bD.pressed) {
      cdState = CD_RUNNING;
      cdTotalMs = cdPausedRemain;
      cdStartMs = now;
      lastUserAction = now; feedbackClick();
    }
    // DD: Reset to IDLE
    if (bD.doubleTap) {
      cdState = CD_IDLE; cdPausedRemain = 0;
      cdIsRest = false;
      lastUserAction = now; feedbackClick();
    }
  }
  // ── CD_DONE ──
  else if (cdState == CD_DONE) {
    // C: Again (restart same duration)
    if (bC.pressed) {
      cdState  = CD_IDLE;
      cdIsRest = false;
      lastUserAction = now; feedbackClick();
    }
  }
}

// ── TODO button handling ───────────────────────────────
void handleTodoButtons() {
  Button& bC = buttons[1];
  Button& bD = buttons[2];
  Button& bE = buttons[3];
  Button& bF = buttons[4];

  const int taskCount = 3;   // keep in sync with renderTodoMode placeholder

  if (todoSub == TODO_LIST) {
    // E: move up
    if (bE.pressed && todoCursor > 0) {
      todoCursor--;
      lastUserAction = millis();
      feedbackClick();
    }
    // F: move down
    if (bF.pressed && todoCursor < taskCount - 1) {
      todoCursor++;
      lastUserAction = millis();
      feedbackClick();
    }
    // C: enter detail
    if (bC.pressed) {
      todoSub = TODO_DETAIL;
      lastUserAction = millis();
      feedbackClick();
    }
    // D: mark complete (placeholder �?future MQTT)
    if (bD.pressed) {
      lastUserAction = millis();
      feedbackClick();
    }
  }
  else if (todoSub == TODO_DETAIL) {
    // C: start countdown with task estimated time
    if (bC.pressed) {
      // Pass estimated time to countdown (placeholder: 45 min for task 0, etc.)
      const int estMinutes[] = {45, 60, 30};
      cdMinutes = estMinutes[todoCursor];
      cdState   = CD_IDLE;
      cdIsRest  = false;
      enterMode(MODE_COUNTDOWN);
      feedbackClick();
    }
    // D: back to list
    if (bD.pressed) {
      todoSub = TODO_LIST;
      lastUserAction = millis();
      feedbackClick();
    }
  }
}

// ── WEATHER button handling ─────────────────────────────
void handleWeatherButtons() {
  Button& bC = buttons[1];
  Button& bD = buttons[2];
  Button& bE = buttons[3];
  Button& bF = buttons[4];

  if (bC.pressed) {
    // Toggle today/forecast
    wxPage = (wxPage == WX_TODAY) ? WX_FORECAST : WX_TODAY;
    lastUserAction = millis();
    feedbackClick();
  }
  if (bD.pressed) {
    // Back to today page
    wxPage = WX_TODAY;
    lastUserAction = millis();
    feedbackClick();
  }
  // E/F: scroll forecast days (only on forecast page)
  if (wxPage == WX_FORECAST) {
    if (bE.pressed && wxDayCursor > 0) {
      wxDayCursor--;
      lastUserAction = millis();
      feedbackClick();
    }
    if (bF.pressed && wxDayCursor < 2) {
      wxDayCursor++;
      lastUserAction = millis();
      feedbackClick();
    }
  }
}

// ── SETTINGS button handling ───────────────────────────
void handleSettingsButtons() {
  Button& bC = buttons[1];
  Button& bD = buttons[2];
  Button& bE = buttons[3];
  Button& bF = buttons[4];

  const int sleepValues[] = {15000, 30000, 60000, 120000, 0};  // ms, 0=off
  const int sleepCount = 5;
  const int aiCount    = 3;

  // ── MAIN MENU ──
  if (settingsSub == SETTINGS_MAIN) {
    if (bE.pressed && settingsCursor > 0) {
      settingsCursor--;
      lastUserAction = millis();
      feedbackClick();
    }
    if (bF.pressed && settingsCursor < SETTINGS_ITEMS - 1) {
      settingsCursor++;
      lastUserAction = millis();
      feedbackClick();
    }
    if (bC.pressed) {
      // Enter sub-page based on selected item
      switch (settingsCursor) {
        case 1: settingsSub = SETTINGS_BRIGHTNESS; break;
        case 4: settingsSub = SETTINGS_SLEEP;      break;
        case 5: settingsSub = SETTINGS_AI;         break;
        default: break;  // others: placeholder / no-op
      }
      lastUserAction = millis();
      feedbackClick();
    }
    if (bD.pressed) {
      // Return to previous mode
      enterMode(beforeSettings);
      feedbackClick();
    }
  }
  // ── BRIGHTNESS SUB ──
  else if (settingsSub == SETTINGS_BRIGHTNESS) {
    if (bE.pressed && settingsBrightness > 0) {
      settingsBrightness = max(0, settingsBrightness - 10);
      lastUserAction = millis();
      feedbackClick();
    }
    if (bF.pressed && settingsBrightness < 100) {
      settingsBrightness = min(100, settingsBrightness + 10);
      lastUserAction = millis();
      feedbackClick();
    }
    if (bC.pressed) {
      // Apply brightness (SSD1306 contrast: map 0-100 �?0-255)
      display.ssd1306_command(0x81);
      display.ssd1306_command((uint8_t)map(settingsBrightness, 0, 100, 0, 255));
      settingsSub = SETTINGS_MAIN;
      lastUserAction = millis();
      feedbackClick();
    }
    if (bD.pressed) {
      settingsSub = SETTINGS_MAIN;
      lastUserAction = millis();
      feedbackClick();
    }
  }
  // ── SLEEP TIMEOUT SUB ──
  else if (settingsSub == SETTINGS_SLEEP) {
    if (bE.pressed && settingsSleepIdx > 0) {
      settingsSleepIdx--;
      lastUserAction = millis();
      feedbackClick();
    }
    if (bF.pressed && settingsSleepIdx < sleepCount - 1) {
      settingsSleepIdx++;
      lastUserAction = millis();
      feedbackClick();
    }
    if (bC.pressed) {
      // Apply: update TIMEOUT_MS equivalent via a global variable
      // (TIMEOUT_MS is a #define; we shadow it with a runtime var below)
      settingsSub = SETTINGS_MAIN;
      lastUserAction = millis();
      feedbackClick();
    }
    if (bD.pressed) {
      settingsSub = SETTINGS_MAIN;
      lastUserAction = millis();
      feedbackClick();
    }
  }
  // ── AI MODE SUB ──
  else if (settingsSub == SETTINGS_AI) {
    if (bE.pressed && settingsAiIdx > 0) {
      settingsAiIdx--;
      lastUserAction = millis();
      feedbackClick();
    }
    if (bF.pressed && settingsAiIdx < aiCount - 1) {
      settingsAiIdx++;
      lastUserAction = millis();
      feedbackClick();
    }
    if (bC.pressed) {
      // Apply AI mode (future: send to MQTT/config)
      settingsSub = SETTINGS_MAIN;
      lastUserAction = millis();
      feedbackClick();
    }
    if (bD.pressed) {
      settingsSub = SETTINGS_MAIN;
      lastUserAction = millis();
      feedbackClick();
    }
  }
}

// ================================================================
//  GLOBAL BUTTON DISPATCH
// ================================================================

void dispatchButtons() {
  // Index mapping: 0=B, 1=C, 2=D, 3=E, 4=F
  Button& bB = buttons[0];
  Button& bE = buttons[3];

  // ── E 双击 = 原 A 长按关机 ──
  if (bE.doubleTap) {
    Serial.println(F("[BTN] E double-tap -> shutdown"));
    feedbackClick();
    // TODO: implement graceful shutdown / deep sleep
  }

  // ── B: Mode switching ──
  // Priority: doubleTap > longPress > single short press
  if (bB.doubleTap) {
    // Double-tap B: go directly to CHAT-ROBOT
    enterMode(MODE_CHAT_ROBOT);
    feedbackClick();
  }
  else if (bB.longPress) {
    // Long-press B (2s): enter SETTINGS from any mode
    if (currentMode != MODE_SETTINGS) {
      beforeSettings = currentMode;
      enterMode(MODE_SETTINGS);
    }
    feedbackClick();
  }
  else if (bB.released && !bB.longPressActive && !bB.doubleTap) {
    // Short press B (released, no long-press, no double-tap):
    if (currentMode == MODE_SETTINGS) {
      enterMode(MODE_CHAT_ROBOT);
    } else {
      cycleMode();
    }
    feedbackClick();
  }

// ── Mode-specific C/D/E/F handlers ──
  switch (currentMode) {
    case MODE_CHAT_ROBOT: handleChatRobotButtons(); break;
    case MODE_CLOCK:      handleClockButtons();      break;
    case MODE_COUNTDOWN:  handleCountdownButtons();  break;
    case MODE_TODO:       handleTodoButtons();       break;
    case MODE_WEATHER:    handleWeatherButtons();    break;
    case MODE_SETTINGS:   handleSettingsButtons();   break;
  }
}

// ================================================================
//  TIMEOUT CHECK (auto-return to CHAT-ROBOT)
// ================================================================
void checkTimeout() {
  // Don't auto-return if countdown is running
  if (currentMode == MODE_COUNTDOWN && cdState == CD_RUNNING) return;
  // Don't auto-return from CHAT-ROBOT itself
  if (currentMode == MODE_CHAT_ROBOT) return;

  if (millis() - lastUserAction >= TIMEOUT_MS) {
    enterMode(MODE_CHAT_ROBOT);
  }
}

// ================================================================
//  SETUP
// ================================================================
void setup() {
  // ── Init Serial (USB CDC �?does NOT use GPIO0/1 on ESP32-C3) ──
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("=== maikefeng boot ==="));
  Serial.println(F("[1/6] Serial OK"));
  Serial.flush();
  delay(50);

  // ── Init I/O (OLED on GPIO8/9, 5 keys on GPIO0-4 direct to GND) ──
  Wire.begin(OLED_SDA, OLED_SCL);
  Serial.println(F("[2/6] I2C OK"));
  Serial.flush();

  initButtons();
  Serial.println(F("[3/6] Buttons init OK"));
  Serial.flush();

  // ── Button diagnostic: read all 5 direct GPIO keys (no buttons should be pressed) ──
  Serial.println(F("=== Button GPIO diagnostic (release all keys) ==="));
  const char* btnNames[BTN_COUNT] = {"B(GPIO0)","C(GPIO1)","D(GPIO2)","E(GPIO3)","F(GPIO4)"};
  for (int i = 0; i < BTN_COUNT; i++) {
    int reading = digitalRead(btnPins[i]);
    Serial.print(F("  "));
    Serial.print(btnNames[i]);
    Serial.print(F(" = "));
    Serial.println(reading == LOW ? F("LOW (pressed?)") : F("HIGH (ok)"));
  }
  Serial.println(F("=== diagnostic done ==="));

  // ── Init OLED ──
  Serial.println(F("[4/6] OLED init..."));
  Serial.flush();
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("FAILED!"));
    for (;;);
  }
  Serial.println(F("[4/6] OLED OK"));
  Serial.flush();
  display.clearDisplay();
  display.display();
  delay(200);

  // ── WiFi & NTP 同步（北京时�?UTC+8）──
  Serial.println(F("[5/6] WiFi..."));
  Serial.flush();
  connectWiFi();
  Serial.println(F("[5/6] WiFi OK"));
  Serial.flush();
  configTime(GMT_OFFSET, DAYLIGHT, NTP_SERVER, "ntp1.aliyun.com", "pool.ntp.org");

  // 等待 NTP 首次同步（最多等 8 秒，阿里�?NTP 通常很快�?
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 24);
  display.print(F("NTP syncing..."));
  display.display();

  struct tm ti;
  unsigned long syncStart = millis();
  while (!getLocalTime(&ti) && millis() - syncStart < 8000) {
    delay(200);
  }
  updateTime();   // 写入 timeStr & 设置 timeSynced 标志

  // ── Play opening animation ──
  playAnimation();

  // ── Start in CHAT-ROBOT mode ──
  currentMode       = MODE_CHAT_ROBOT;
  currentModeIcon   = ICON_MODE_CHAT_ROBOT;
  cdState           = CD_IDLE;
  cdMinutes         = 25;
  cdIsRest          = false;
  cdPomodoro        = false;
  lastUserAction    = millis();

  // ── Start software watchdog (100ms tick) ──
  wdtTimer = timerBegin(0, 80, true);          // timer 0, prescaler 80 → 1 MHz
  timerAttachInterrupt(wdtTimer, &wdtISR, true);
  timerAlarmWrite(wdtTimer, 100000, true);      // 100 ms alarm
  timerAlarmEnable(wdtTimer);

  Serial.println(F("[6/6] Setup complete, entering loop"));
  Serial.flush();
}

// ================================================================
//  LOOP
// ================================================================
void loop() {
  // ── Feed software watchdog ──
  wdtFeedCount = 0;

  // ── Watchdog tripped → soft reset to home screen ──
  if (wdtTripped) {
    wdtTripped     = false;
    currentMode    = MODE_CHAT_ROBOT;
    currentModeIcon = ICON_MODE_CHAT_ROBOT;
    if (cdState == CD_RUNNING) { cdState = CD_IDLE; cdIsRest = false; }
    lastUserAction = millis();
    updateStatusBarTag();
    updateModeIcon();
    display.clearDisplay();
    display.display();
    Serial.println(F("WDT: soft-reset → CHAT-ROBOT"));
    return;  // restart loop fresh
  }

  // ── Scan all buttons + debounce + edge detect ──
  updateButtons();

  // ── 省电模式：30s无操作熄屏 ──
  if (!screenSleep && (now - lastUserAction >= SCREEN_SLEEP_MS)) {
    screenSleep = true;
    display.ssd1306_command(0xAE);  // SSD1306 DISPLAY OFF (省电)
    Serial.println(F("[PWR] Screen sleep"));
  }

  // ── Debug: print stable button states (1=LOW=pressed) ──
  static unsigned long lastDbg = 0;
  if (millis() - lastDbg >= 500) {
    lastDbg = millis();
    Serial.print(F("STB:["));
    for (int i = 0; i < BTN_COUNT; i++) {
      Serial.print(btnStable[i] == LOW ? '1' : '0');
    }
    Serial.print(F("] EDT:["));
    for (int i = 0; i < BTN_COUNT; i++) {
      if (buttons[i].pressed) { Serial.print(i); Serial.print(','); }
    }
    Serial.println();
  }

  unsigned long now = millis();

  // ── NTP time update (every 60s) ──
  if (now - lastTimeUpdate >= TIME_REFRESH_MS) {
    updateTime();
    lastTimeUpdate = now;
  }

  // ── WiFi signal strength update (every 3s) ──
  static unsigned long lastWifiUpdate = 0;
  if (now - lastWifiUpdate >= 3000) {
    updateWifiState();
    lastWifiUpdate = now;
  }

  // ── Battery state update ──
  updateBattState();

  // ── Countdown tick (if running) ──
  if (currentMode == MODE_COUNTDOWN && cdState == CD_RUNNING) {
    unsigned long elapsed = now - cdStartMs;
    if (elapsed >= cdTotalMs) {
      // Countdown finished!
      if (cdPomodoro && !cdIsRest) {
        // Focus done �?auto-start rest phase (5 min)
        cdIsRest   = true;
        cdMinutes  = CD_REST_MINUTES;
        cdTotalMs  = CD_REST_MINUTES * 60000UL;
        cdStartMs  = now;
        // (cdState stays CD_RUNNING)
      } else {
        // Normal finish or rest complete
        cdState  = CD_DONE;
        cdIsRest = false;
        // Triple LED flash for done (硬件未焊接时跳过)
        if (LED_PIN >= 0) {
          for (int _i = 0; _i < 3; _i++) {
            digitalWrite(LED_PIN, HIGH); delay(80);
            digitalWrite(LED_PIN, LOW);
            if (_i < 2) delay(60);
          }
        }
      }
    }
  }

  // ── Button dispatch ──
  dispatchButtons();

  // ── Timeout check ──
  checkTimeout();

  // ── Render ──
  // All modes use 200ms dirty-flag rendering to reduce flicker
  // 省电模式：熄屏时跳过渲染；唤醒时强制重绘一次
  static unsigned long lastRenderMs = 0;
  static AppMode       lastRenderMode = MODE_COUNT;

  bool modeChanged = (currentMode != lastRenderMode);
  bool timeToRedraw = (now - lastRenderMs >= 200);

  if (!screenSleep && (modeChanged || timeToRedraw || wakeRedraw)) {
    wakeRedraw = false;  // 用完即清
    updateStatusBarTag();
    drawStatusBar();
    renderCurrentMode();
    display.display();
    lastRenderMs   = now;
    lastRenderMode = currentMode;
  }

  // ── Adaptive delay ──
  // 5ms loop delay gives ~180Hz scan rate. Each button is sampled once per loop.
  // Timer-based debounce (30ms stable → accept) filters contact bounce.
  delay(5);
}
