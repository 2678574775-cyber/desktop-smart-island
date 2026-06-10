---
name: desktop-pet-dog
description: 桌面智能灵动岛项目概览 — 硬件架构、重构历史、关键设计决策
metadata: 
  node_type: memory
  type: project
  originSessionId: 311920d7-252e-43d0-8882-ab0ae806828b
---

## 项目简介

ESP32-C3 SuperMini 驱动的桌面 OLED 信息终端,6 种应用模式,74HC138 + 6 按键交互。

## 硬件

- MCU: ESP32-C3 SuperMini
- 显示: 0.96" SSD1306 128×64 双色 OLED (I2C: SDA→GPIO8, SCL→GPIO9)
- 按键: 74HC138 3-8 译码器, 地址线 GPIO0/1/2, 感应线 GPIO4 (通过 10k R6)
- 反馈: LED + 蜂鸣器 GPIO10
- Board 配置: `esp32dev` (兼容 S3/S6)

## 项目架构 (重构后)

20 个源文件, 分层结构:
- `src/config.h` — 全局配置、枚举、状态 extern 声明
- `src/hardware/` — display_hal, button_hal, buzzer_hal (硬件抽象)
- `src/ui/` — icons, status_bar, animation (UI 渲染)
- `src/services/` — wifi_service, ntp_service (网络服务)
- `src/modes/` — mode_chat_robot, mode_clock, mode_countdown, mode_todo, mode_weather, mode_settings (6 种模式)
- `test/` — gpio4_test, hc138_diag (诊断工具)

依赖关系: config.h → hardware/ → {ui/, services/, modes/} → main.cpp (无循环依赖)

## 重构历史

**2026-06-02 重构**: 原本 4 个 .ino 文件在根目录乱放, 主程序 maikefeng.ino 达 2027 行单文件巨石。重构后拆分为标准 .cpp/.h 模块化架构:
- 原始文件备份: `maikefeng.ino.bak`
- Board 保持 `esp32dev` (用户要求, 后续推广到 S3/S6)
- 所有代码行为与原版一致

## 6 种模式

| 模式 | 功能 |
|------|------|
| CHAT-ROBOT | 默认模式, 语音交互预留 |
| CLOCK | 大字时钟 + 冒号闪烁 + 日期星期 |
| COUNTDOWN | 倒计时 + 番茄钟 (FOCUS/REST) |
| TODO | 待办列表 / 详情, 可跳转倒计时 |
| WEATHER | 今日天气 + 3 日预报 (占位数据) |
| SETTINGS | 亮度 / 休眠 / AI 模式 |

## 按键操作

- B 短按: 循环切换模式
- B 长按 2s: 进入设置
- B 双击: 回到 CHAT-ROBOT
- 30s 无操作: 自动返回 CHAT-ROBOT
- 按键扫描: HC138 快速轮询 + GPIO4 FALLING ISR + 5 次连续采样去抖

## 后续规划

- MQTT 接入云端同步 TODO / 天气
- 语音输入 (C 键长按)
- 后端对话机器人
- 帧缓冲滑动动画
- 低功耗休眠
- ESP32-S3/S6 适配

**Why:** 项目整体概览，用于后续开发时快速回顾架构和决策。

**How to apply:** 开发新功能时先看此文件了解归属层级；添加新模式参考 `modes/` 中现有模板。
