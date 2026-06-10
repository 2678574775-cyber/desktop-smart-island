/**
 * main.cpp — 桌面智能灵动岛 主入口
 * 硬件: ESP32-C3 SuperMini + SSD1306 OLED + 5 键直连 GPIO
 *
 * 架构分层:
 *   config.h          全局配置/枚举/状态
 *   hardware/*        OLED显示 / 按键扫描 / LED反馈
 *   ui/*              状态栏 / 图标 / 动画
 *   services/*        WiFi / NTP
 *   modes/*           6 种应用模式 (CHAT-ROBOT/CLOCK/COUNTDOWN/TODO/WEATHER/SETTINGS)
 *
 * main.cpp 职责:
 *   全局状态定义 | enterMode/cycleMode | 超时检测 | setup/loop
 *   按键事件处理已提取到 hardware/button_events
 */

#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <Preferences.h>

#include "config.h"

#include "hardware/display_hal.h"
#include "hardware/button_hal.h"
#include "hardware/button_events.h"
#include "hardware/buzzer_hal.h"
#include "hardware/batt_hal.h"
#include "hardware/i2s_hal.h"
#include "hardware/mic_hal.h"
#include "hardware/audio_test.h"

#include "ui/icons.h"
#include "ui/status_bar.h"
#include "ui/animation.h"

#include "services/wifi_service.h"
#include "services/ntp_service.h"
#include "services/asr_service.h"

#include "modes/mode_chat_robot.h"
#include "modes/mode_clock.h"
#include "modes/mode_countdown.h"
#include "modes/mode_todo.h"
#include "modes/mode_weather.h"
#include "modes/mode_settings.h"

// ================================================================
//  WiFi & NTP 配置 (凭据从 Preferences 加载，无保存凭据时启动扫码设置)
// ================================================================
char WIFI_SSID[32] = "";
char WIFI_PASS[32] = "";
const char* NTP_SERVER = "ntp.aliyun.com";
const long   GMT_OFFSET = 8 * 3600;
const int    DAYLIGHT   = 0;

// ================================================================
//  按键底层数据
// ================================================================
Button buttons[BTN_COUNT];
uint8_t  btnRaw[BTN_COUNT];
uint8_t  debounceCnt[BTN_COUNT];
uint8_t  debounceState[BTN_COUNT];

// ================================================================
//  音频 + 省电
// ================================================================
uint8_t* micRecordBuf  = NULL;       // 堆分配 (mic_hal 内部管理)
int      micRecordLen  = 0;
bool     micRecording  = false;
bool     screenSleep   = false;       // OLED 熄屏标志
int      screenSleepMs = 30000;       // 熄屏超时 (ms), 可通过设置调整

// ================================================================
//  看门狗 (硬件定时器, 100ms tick)
// ================================================================
volatile bool    wdtTripped   = false;
volatile uint32_t wdtFeedCount = 0;
hw_timer_t*      wdtTimer     = NULL;

void IRAM_ATTR wdtISR() {
  wdtFeedCount++;
  if (wdtFeedCount > (WDT_TIMEOUT_MS / 100)) {
    wdtTripped = true;
  }
}

// ================================================================
//  模式状态
// ================================================================
AppMode       currentMode      = MODE_CHAT_ROBOT;
AppMode       beforeSettings   = MODE_CHAT_ROBOT;
ModeIcon      currentModeIcon  = ICON_MODE_CHAT_ROBOT;
BattState     currentBatt      = BATT_FULL;
WifiState     currentWifi      = WIFI_STRONG;
unsigned long lastUserAction   = 0;
unsigned long lastUserActionSleep = 0;  // 独立熄屏计时器: 仅由真实按键事件更新

// ================================================================
//  状态栏
// ================================================================
char statusBarTag[4] = "";

// ================================================================
//  倒计时状态
// ================================================================
CdState       cdState         = CD_IDLE;
int           cdMinutes       = 25;
int           cdSeconds       = 0;
unsigned long cdStartMs       = 0;
unsigned long cdPausedRemain  = 0;
unsigned long cdTotalMs       = 0;
bool          cdPomodoro      = false;
bool          cdIsRest        = false;
int           cdPresetIdx     = 1;
const int     cdPresets[]     = {15, 25, 45, 60};

// ================================================================
//  TODO 状态
// ================================================================
TodoSubState  todoSub         = TODO_LIST;
int           todoCursor      = 0;

// ================================================================
//  设置状态
// ================================================================
SettingsSub   settingsSub     = SETTINGS_MAIN;
int           settingsCursor  = 0;
int           settingsBrightness = 100;
int           settingsSleepIdx   = 1;
int           settingsAiIdx      = 0;

// ================================================================
//  天气状态
// ================================================================
WxPage        wxPage          = WX_TODAY;
int           wxDayCursor     = 0;

// ================================================================
//  CHAT-ROBOT 状态
// ================================================================
CrSubState    crSub           = CR_EXPR;

// ================================================================
//  时间状态
// ================================================================
char          timeStr[6]      = "--:--";
bool          timeSynced      = false;
unsigned long lastTimeUpdate  = 0;
unsigned long lastNtpRetry    = 0;

// ================================================================
//  核心函数: 模式管理
// ================================================================

/** 进入指定模式，重置子状态 */
void enterMode(AppMode newMode) {
  if (newMode == currentMode) return;

  currentMode = newMode;
  updateModeIcon();
  lastUserAction = millis();

  // 重置子状态
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

  // 清除内容区域
  display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, SSD1306_BLACK);
}

/** D 键循环切换主模式: CHAT-ROBOT → CLOCK → TODO → ... (Weather/Countdown 通过双击D进入) */
void cycleMode() {
  AppMode next = currentMode;
  switch (currentMode) {
    case MODE_CHAT_ROBOT: next = MODE_CLOCK; break;
    case MODE_CLOCK:      next = MODE_TODO;  break;
    case MODE_TODO:       next = MODE_CHAT_ROBOT; break;
    default:              next = MODE_CHAT_ROBOT; break;
  }
  enterMode(next);
}

// ================================================================
//  核心函数: 超时检测
// ================================================================

/** 超时检测: 30s 无操作自动返回 CHAT-ROBOT */
void checkTimeout() {
  if (currentMode == MODE_COUNTDOWN && (cdState == CD_RUNNING || cdState == CD_PAUSED)) return;
  if (currentMode == MODE_CHAT_ROBOT) return;

  if (millis() - lastUserAction >= TIMEOUT_MS) {
    enterMode(MODE_CHAT_ROBOT);
  }
}

/** 模式渲染分发 */
void renderCurrentMode() {
  switch (currentMode) {
    case MODE_CLOCK:      renderClockMode();       break;
    case MODE_COUNTDOWN:  renderCountdownMode();    break;
    case MODE_TODO:       renderTodoMode();         break;
    case MODE_WEATHER:    renderWeatherMode();      break;
    case MODE_SETTINGS:   renderSettingsMode();     break;
    case MODE_CHAT_ROBOT: renderChatRobotIdle();    break;
  }
}

// ================================================================
//  SETUP
// ================================================================
void setup() {
  // [1/6] 初始化串口 (USB CDC，不占用 GPIO0/1)
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("=== desktop_pet_dog boot ==="));
  Serial.println(F("[1/6] Serial OK"));

  // 加载持久化设置 (WiFi 凭据 + 熄屏超时)
  {
    Preferences prefs;
    prefs.begin("desktopdog", true);  // 只读
    String ssid = prefs.getString("wifi_ssid", "");
    String pass = prefs.getString("wifi_pass", "");
    if (ssid.length() > 0) {
      strncpy(WIFI_SSID, ssid.c_str(), 31);
      strncpy(WIFI_PASS, pass.c_str(), 31);
      Serial.printf("[Prefs] WiFi loaded: %s\n", WIFI_SSID);
    } else {
      Serial.println(F("[Prefs] No saved WiFi credentials"));
    }
    settingsSleepIdx = prefs.getInt("sleep_idx", 1);  // 默认 1=30s
    const int sleepMsLut[] = {15000, 30000, 60000, 120000, 0};
    screenSleepMs = sleepMsLut[settingsSleepIdx];
    Serial.printf("[Prefs] Sleep timeout: %ds\n", screenSleepMs / 1000);
    prefs.end();
  }

  Serial.flush();
  delay(50);

  // [2/6] 初始化 I2C
  Wire.begin(OLED_SDA, OLED_SCL);
  Serial.println(F("[2/6] I2C OK"));
  Serial.flush();

  // [3/6] 初始化按键和功放 (5 键直连 GPIO)
  initButtons();
  audioTestInit();   // AMP_SD 引脚拉低, 功放默认关机
  battInit();        // 电池 ADC 初始化
  Serial.println(F("[3/6] Buttons + AMP + Batt OK"));
  Serial.println(F("[4/6] OLED init..."));
  Serial.flush();
  if (!displayInit()) {
    Serial.println(F("FAILED!"));
    for (;;);
  }
  Serial.println(F("[4/6] OLED OK"));
  Serial.flush();
  delay(200);

  // [5/6] WiFi 连接 & NTP 同步
  Serial.println(F("[5/6] WiFi..."));
  Serial.flush();
  connectWiFi();
  Serial.println(F("[5/6] WiFi OK"));
  Serial.flush();
  ntpInit();

  // 等待 NTP 首次同步 (最多 8s)
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
  updateTime();

  // 初始化 ASR (只需检查 WiFi, Zhipu API 不需要单独获取 token)
  if (asrInit()) {
    Serial.println(F("[ASR] Zhipu GLM-ASR ready"));
  } else {
    Serial.println(F("[ASR] no WiFi, Zhipu ASR unavailable"));
  }

  // 播放开机动画
  playAnimation();

  // [6/6] 初始状态
  currentMode       = MODE_CHAT_ROBOT;
  currentModeIcon   = ICON_MODE_CHAT_ROBOT;
  cdState           = CD_IDLE;
  cdMinutes         = 25;
  cdIsRest          = false;
  cdPomodoro        = false;
  lastUserAction    = millis();
  lastUserActionSleep = millis();

  // 启动软件看门狗 (100ms tick, 5s 无喂狗 → 软复位)
  wdtTimer = timerBegin(0, 80, true);          // timer 0, prescaler 80 → 1MHz
  timerAttachInterrupt(wdtTimer, &wdtISR, true);
  timerAlarmWrite(wdtTimer, 100000, true);      // 100ms alarm
  timerAlarmEnable(wdtTimer);

  Serial.println(F("[6/6] Setup complete, entering loop"));
  Serial.flush();
}

// ================================================================
//  LOOP
// ================================================================
void loop() {
  // ── 喂狗 ──
  wdtFeedCount = 0;

  // ── 看门狗触发 → 软复位到主页 ──
  if (wdtTripped) {
    wdtTripped     = false;
    currentMode    = MODE_CHAT_ROBOT;
    currentModeIcon = ICON_MODE_CHAT_ROBOT;
    if (cdState == CD_RUNNING) { cdState = CD_IDLE; cdIsRest = false; }
    lastUserAction = millis();
    lastUserActionSleep = millis();
    display.clearDisplay();
    display.display();
    Serial.println(F("[WDT] soft-reset → CHAT-ROBOT"));
    return;
  }

  // 按键处理: 去抖 + 全局分发 + 模式路由 (由 button_events 封装)
  bool hadButtonEvent = processButtonEvents();

  // ── 熄屏唤醒: 任意按键事件唤醒屏幕 ──
  static bool wakeRedraw = false;
  if (hadButtonEvent && screenSleep) {
    screenSleep = false;
    wakeRedraw = true;
    display.ssd1306_command(0x8D);  // charge pump
    display.ssd1306_command(0x14);
    display.ssd1306_command(0xAF);  // display on
    lastUserActionSleep = millis();
    Serial.println(F("[PWR] Wake by key"));
  }
  // ── 更新熄屏计时器: 仅由真实按键事件驱动 ──
  if (hadButtonEvent && !screenSleep) {
    lastUserActionSleep = millis();
  }

  // 非阻塞 LED 状态机驱动
  feedbackTick();

  unsigned long now = millis();

  // ── 省电熄屏: screenSleepMs 无操作后关闭 OLED ──
  if (screenSleepMs > 0 && !screenSleep && (now - lastUserActionSleep >= (unsigned long)screenSleepMs)) {
    screenSleep = true;
    display.ssd1306_command(0xAE);  // display off
    Serial.println(F("[PWR] Screen sleep"));
  }

  // NTP 时间刷新 (每 60s)
  if (now - lastTimeUpdate >= TIME_REFRESH_MS) {
    updateTime();
    lastTimeUpdate = now;
  }

  // 电池电量检测 (每 BATT_READ_MS ms)
  {
    static unsigned long lastBattRead = 0;
    if (now - lastBattRead >= BATT_READ_MS) {
      updateBattery();
      lastBattRead = now;
    }
  }

  // 麦克风录音数据采集 (每帧调用, 录音结束后自动停止)
  micFeedRecord();

  // Chat-Robot 语音流水线 (录音→ASR→AI→TTS)
  chatRobotTick();
  chatRobotProcess();

  // 倒计时 tick
  if (currentMode == MODE_COUNTDOWN && cdState == CD_RUNNING) {
    unsigned long elapsed = now - cdStartMs;
    if (elapsed >= cdTotalMs) {
      // 倒计时结束
      if (cdPomodoro && !cdIsRest) {
        // 番茄钟: 专注结束 → 自动开始休息阶段 (5 min)
        cdIsRest   = true;
        cdMinutes  = CD_REST_MINUTES;
        cdTotalMs  = CD_REST_MINUTES * 60000UL;
        cdStartMs  = now;
      } else {
        // 普通完成或休息结束
        cdState  = CD_DONE;
        cdIsRest = false;
        // 完成三闪 LED 提示
        feedbackFlash(3, 80, 60);
      }
    }
  }

  // 超时检测
  checkTimeout();

  // 渲染 (200ms 脏标记渲染，减少 OLED 闪烁)
  static unsigned long lastRenderMs = 0;
  static AppMode       lastRenderMode = MODE_COUNT;

  bool modeChanged = (currentMode != lastRenderMode);
  bool timeToRedraw = (now - lastRenderMs >= 200);

  // 熄屏时跳过渲染 (省电), 唤醒时强制重绘一帧
  if (!screenSleep && (modeChanged || timeToRedraw || wakeRedraw)) {
    wakeRedraw = false;
    updateStatusBarTag();
    drawStatusBar();
    renderCurrentMode();
    display.display();
    lastRenderMs   = now;
    lastRenderMode = currentMode;
  }

  // 让出 CPU 给 FreeRTOS 调度器 (主循环 ~500-1000Hz)
  delay(1);
}
