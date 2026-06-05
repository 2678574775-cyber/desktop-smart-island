/**
 * ntp_service.cpp — NTP 时间同步服务实现
 */

#include "ntp_service.h"
#include <WiFi.h>
#include <time.h>

// ---- 初始化 NTP ----
void ntpInit() {
  configTime(GMT_OFFSET, DAYLIGHT, NTP_SERVER, "ntp1.aliyun.com", "pool.ntp.org");
}

// ---- 更新时间 ----
void updateTime() {
  unsigned long now = millis();

  // 检查 WiFi 状态并更新图标
  if (WiFi.status() == WL_CONNECTED) {
    currentWifi = WIFI_STRONG;
  } else {
    currentWifi = WIFI_OFFLINE;
    // WiFi 离线时也尝试用已有的系统时间
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
    // 未同步过或超时，重新请求 NTP
    configTime(GMT_OFFSET, DAYLIGHT,
               NTP_SERVER, "ntp1.aliyun.com", "pool.ntp.org");
    lastNtpRetry = now;

    // 等 1.5 秒让 NTP 完成首次同步
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
