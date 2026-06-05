/**
 * ntp_service.h — NTP 时间同步服务
 * 管理北京时间 (UTC+8) 的同步与更新
 */

#ifndef NTP_SERVICE_H
#define NTP_SERVICE_H

#include "../config.h"

/** 初始化 NTP 配置 */
void ntpInit();

/** 更新北京时间到 timeStr，含自动重试逻辑 */
void updateTime();

#endif // NTP_SERVICE_H
