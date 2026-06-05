/**
 * icons.h — 图标位图数据 (PROGMEM)
 * 包含模式图标、电池图标、WiFi 图标、动画帧
 */

#ifndef ICONS_H
#define ICONS_H

#include <Arduino.h>

// ================================================================
//  模式图标 (12x8 px, 16 bytes each)
// ================================================================
extern const uint8_t PROGMEM MODE_CLOCK_BMP[];
extern const uint8_t PROGMEM MODE_COUNTDOWN_BMP[];
extern const uint8_t PROGMEM MODE_CHAT_ROBOT_BMP[];
extern const uint8_t PROGMEM MODE_WEATHER_BMP[];
extern const uint8_t PROGMEM MODE_SETTINGS_BMP[];
extern const uint8_t PROGMEM MODE_TODO_BMP[];

/** 模式图标查找表 */
extern const uint8_t* const mode_icon_table[] PROGMEM;

// ================================================================
//  电池图标 (12x8 px)
// ================================================================
extern const uint8_t PROGMEM BATT_FULL_BMP[];
extern const uint8_t PROGMEM BATT_MID_BMP[];
extern const uint8_t PROGMEM BATT_LOW_BMP[];
extern const uint8_t PROGMEM BATT_CHARGE_BMP[];

/** 电池图标查找表 */
extern const uint8_t* const batt_icon_table[] PROGMEM;

// ================================================================
//  WiFi 图标 (12x8 px)
// ================================================================
extern const uint8_t PROGMEM WIFI_STRONG_BMP[];
extern const uint8_t PROGMEM WIFI_MID_BMP[];
extern const uint8_t PROGMEM WIFI_WEAK_BMP[];
extern const uint8_t PROGMEM WIFI_OFFLINE_BMP[];

/** WiFi 图标查找表 */
extern const uint8_t* const wifi_icon_table[] PROGMEM;

// ================================================================
//  动画帧 (128x48 px, 768 bytes each)
// ================================================================
extern const uint8_t PROGMEM frame3[];
extern const uint8_t PROGMEM frame5[];

/** 动画帧查找表 */
extern const uint8_t* const frames[] PROGMEM;

#endif // ICONS_H
