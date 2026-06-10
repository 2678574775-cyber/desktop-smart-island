/**
 * config.h — 全局配置 (per README.md 按键映射)
 * A=GPIO2  B=GPIO4  C=GPIO3  D=GPIO1  (E=GPIO0 硬件损坏已移除)
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// OLED
#define OLED_SDA        8
#define OLED_SCL        9
#define OLED_RESET     -1
#define OLED_ADDR     0x3C

// 按键 (4 键, 匹配 README 物理标签)
#define KEY_PIN_A       2     // A: GPIO2 ⚠️Strapping
#define KEY_PIN_B       4     // B: GPIO4
#define KEY_PIN_C       3     // C: GPIO3
#define KEY_PIN_D       1     // D: GPIO1

// I2S 音频
#define I2S_BCLK        5
#define I2S_LRC         6
#define I2S_DIN         7
#define I2S_DOUT       10
#define AMP_SD_PIN     20

#define BUZZER_PIN     -1
#define LED_PIN        -1

// 电池 ADC (GPIO0 原按键E，已损坏移除 → 改为电池电压检测)
#define BATT_ADC_PIN     0     // ADC1_CH0, 需外接分压电路: Vbat→100k→GPIO0→100k→GND
#define BATT_R1        100     // 分压电阻 R1 (kΩ, 接 Vbat)
#define BATT_R2        100     // 分压电阻 R2 (kΩ, 接 GND)
#define BATT_READ_MS 10000     // 电池电压读取间隔 (ms)

// 屏幕
#define SCREEN_W        128
#define SCREEN_H         64
#define STATUSBAR_H       9
#define CONTENT_Y        16
#define CONTENT_H        48
#define CONTENT_W       128
#define MODE_ICON_X       0
#define WIFI_ICON_X     104
#define BATT_ICON_X     116
#define TIME_CURSOR_X    40
#define ICON_Y            0

// 按键参数
#define BTN_COUNT          4
#define DEBOUNCE_CNT       8
#define LONG_PRESS_MS_A 1000     // A键录音阈值 (1s)
#define DOUBLE_TAP_MS    500     // D键双击窗口
#define BTN_DEBUG          0

// 超时 / 省电 / 看门狗
#define TIMEOUT_MS      30000
#define WDT_TIMEOUT_MS    5000
#define TIME_REFRESH_MS  60000
#define NTP_RETRY_MS    120000

// 动画
#define STEPS_ENTER       16
#define STEPS_EXIT        16
#define DELAY_MS          90
#define HOLD5_MS        3000

// 倒计时
#define CD_PRESET_COUNT     4
#define CD_REST_MINUTES     5
extern const int cdPresets[CD_PRESET_COUNT];

// TODO
#define MAX_TASKS          10
#define SETTINGS_ITEMS      7

// 音频
#define SAMPLE_RATE      16000
#define TTS_SAMPLE_RATE  24000
#define RECORD_MAX_SEC   3
#define RECORD_BUF_SIZE  (SAMPLE_RATE * 2 * RECORD_MAX_SEC)

// 智谱 API
#ifndef ZHIPU_API_KEY
#define ZHIPU_API_KEY "YOUR_ZHIPU_API_KEY"  // 请替换为你的API Key
#endif

// ── 枚举 ──
enum AppMode {
  MODE_CHAT_ROBOT = 0, MODE_CLOCK = 1, MODE_COUNTDOWN = 2,
  MODE_TODO = 3, MODE_WEATHER = 4, MODE_SETTINGS = 5, MODE_COUNT = 6
};
enum ModeIcon {
  ICON_MODE_CLOCK = 0, ICON_MODE_COUNTDOWN = 1, ICON_MODE_CHAT_ROBOT = 2,
  ICON_MODE_TODO = 3, ICON_MODE_WEATHER = 4, ICON_MODE_SETTINGS = 5, MODE_ICON_COUNT
};
enum BattState { BATT_FULL = 0, BATT_MID = 1, BATT_LOW = 2, BATT_CHARGE = 3 };
enum WifiState { WIFI_STRONG = 0, WIFI_MID = 1, WIFI_WEAK = 2, WIFI_OFFLINE = 3 };
enum CdState { CD_IDLE, CD_RUNNING, CD_PAUSED, CD_DONE };
enum CrSubState { CR_EXPR, CR_LISTEN, CR_THINK, CR_REPLY };
enum TodoSubState { TODO_LIST, TODO_DETAIL };
enum WxPage { WX_TODAY, WX_FORECAST };
enum SettingsSub { SETTINGS_MAIN, SETTINGS_BRIGHTNESS, SETTINGS_SLEEP, SETTINGS_AI };

struct Button {
  uint8_t raw, last;
  bool pressed, released, longPress, doubleTap, longPressActive;
  unsigned long pressTime, releaseTime, lastClickTime;
};

// ── 前向声明 ──
void enterMode(AppMode newMode);
void cycleMode();

// ── extern ──
extern Button        buttons[BTN_COUNT];
extern uint8_t       btnRaw[BTN_COUNT], debounceCnt[BTN_COUNT], debounceState[BTN_COUNT];
extern AppMode       currentMode, beforeSettings;
extern ModeIcon      currentModeIcon;
extern BattState     currentBatt;
extern WifiState     currentWifi;
extern unsigned long lastUserAction;
extern unsigned long lastUserActionSleep;  // 独立熄屏计时器 (不被 enterMode 重置)
extern char          statusBarTag[4];
extern CdState       cdState;
extern int           cdMinutes, cdSeconds;
extern unsigned long cdStartMs, cdPausedRemain, cdTotalMs;
extern bool          cdPomodoro, cdIsRest;
extern int           cdPresetIdx;
extern TodoSubState  todoSub;
extern int           todoCursor;
extern SettingsSub   settingsSub;
extern int           settingsCursor, settingsBrightness, settingsSleepIdx, settingsAiIdx;
extern WxPage        wxPage;
extern int           wxDayCursor;
extern CrSubState    crSub;
extern char          timeStr[6];
extern bool          timeSynced;
extern unsigned long lastTimeUpdate, lastNtpRetry;
extern char          WIFI_SSID[32], WIFI_PASS[32];
extern const char*   NTP_SERVER;
extern const long    GMT_OFFSET;
extern const int     DAYLIGHT;
extern uint8_t*      micRecordBuf;
extern int           micRecordLen;
extern bool          micRecording, screenSleep;
extern int           screenSleepMs;

#endif
