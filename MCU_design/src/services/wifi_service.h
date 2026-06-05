/**
 * wifi_service.h — WiFi 连接管理 + 切换设置
 */

#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include "../config.h"

/** 开机连接 WiFi (用当前 WIFI_SSID / WIFI_PASS) */
void connectWiFi();

/** 重新扫描 + 选择网络 + 输入密码 + 连接 (OLED 交互) */
void wifiSetup();

#endif
