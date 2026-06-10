# 桌面智能灵动岛 (Desktop Pet Dog)

简介：这是Desktop Pet Dog项目的桌面智能终端的代码设计，ESP32-C3 SuperMini 驱动的桌面 OLED 信息终端，支持 6 种应用模式，5 按键直连 GPIO 交互，通过 I2S 总线连接 MAX98357A 功放与 INMP441 麦克风实现语音输入与输出，接入通过蓝牙与手机进行连接，接入Deepseek大模型，采用简易的Agent架构对AI对话中的上下文进行处理。

---

## 技术规格和任务清单

### 任务清单

| TAsk               | 完成度 |
| ------------------ | ------ |
| OLED开幕动画显示   | ✓      |
| 模式切换与按键监测 | ✓      |
| OLED内容显示       | ✓      |
| 电量的监控         | ✕      |
| Wi-Fi信号的监控    | ✕      |
| 语音输入识别模块   | ✕      |
| 语音输出模块       | ✕      |
| MQTT，与手机互联   | ✕      |
| 设置模块的显示     | ✕      |
| 表情包动画         | ✕      |
| 看门狗/省电模式    | ✕      |
|                    |        |

### 硬件支持

| 组件 | 型号 / 连接 |
|------|------------|
| MCU | ESP32-C3 SuperMini |
| 显示 | 0.96" SSD1306 双色 OLED (128×64, I2C, 地址 0x3C) |
| 按键 | 5 个轻触开关, 直连 GPIO0-4, 内部上拉 |
| 音频输入 | INMP441 I2S 麦克风 (I2S 总线, 与功放共用 BCLK/LRC) |
| 音频输出 | MAX98357A I2S 功放 + 4Ω 3W 喇叭 (I2S 总线, 与麦克风共用 BCLK/LRC) |
| 网页端 | 前后端在Cloudflare上 |

### 引脚连接

```
ESP32-C3 SuperMini 引脚分配总览:

       左侧                         右侧
    ┌──────────────────────────────┐
    │ 5V   — TP4056 5V 输出        │
    │ GND  — 共地                  │
    │ 3V3  — AMS1117 稳压输出       │
    │ GPIO4 — 按键A (辅助/方向)      │
    │ GPIO3 — 按键B (返回/取消)      │
    │ GPIO2 — 按键C (确认/录音) ⚠️   │
    │ GPIO1 — 按键D (模式切换)       │
    │ GPIO0 — 按键E (电源)          │
    │                              │
    │               GPIO5  — I2S BCLK (MAX98357A + INMP441 共用)
    │               GPIO6  — I2S LRC  (MAX98357A + INMP441 共用)
    │               GPIO7  — I2S DIN  → MAX98357A
    │               GPIO8  — I2C SDA  → OLED
    │               GPIO9  — I2C SCL  → OLED
    │               GPIO10 — I2S DOUT ← INMP441
    │               GPIO20 — USB D+
    │               GPIO21 — USB D-
    └──────────────────────────────┘  
```

> ⚠️ **GPIO2 为 Strapping 引脚**，开机时请勿按住按键 C，否则可能影响启动。

按键 (5 颗，直连 GPIO，`INPUT_PULLUP`，按下 = LOW):

```
GPIO4 ─┬─ 按键A ─ GND    短按-模式切换 双击-
GPIO3 ─┬─ 按键B ─ GND    返回 / 暂停
GPIO2 ─┬─ 按键C ─ GND    短按 → 确认 / 长按 → 语音
GPIO1 ─┬─ 按键D ─ GND    短按 → 模式切换 / 长按 → 设置
GPIO0 ─┬─ 按键E ─ GND    长按 2s → 开关机
```

I2S 音频 (MAX98357A + INMP441 共用 BCLK/LRC):

```
           ESP32-C3                 MAX98357A           INMP441
           ────────                ──────────          ────────
  GPIO5 ──→ BCLK ──────────┬────→ BCLK         ┌────→ SCK
  GPIO6 ──→ LRC  ──────────┼────→ LRC          ├────→ WS
  GPIO7 ──→ DIN  ──────────┼────→ DIN          │
  GPIO10 ←── DOUT ─────────┼────────────────── └──── SD
                            │
                            │    电源与配置:
                            │    VIN  → 电池 3.7V (直供, 不经过 LDO)
                            │    GND  → GND
                            │    GAIN → GND (12dB)
                            │    SD   → 3.3V / 悬空
                            │    + / -→ 喇叭 4Ω 3W
                            │
                            │    VDD  → 3.3V (禁止 5V!)
                            │    GND  → GND
                            │    L/R  → GND (左声道, 禁止悬空)
```

OLED (I2C):

```
OLED:  VCC → 3.3V    GND → GND    SDA → GPIO8    SCL → GPIO9
```

### 电源架构

```
USB 5V → TP4056 → 14250 电池 (3.7V)
                    │
                    ├──→ AMS1117-3.3V → ESP32 / OLED / INMP441
                    │
                    └──→ MAX98357A VIN (功放直供, 峰值电流大)
```

### 按键映射

| 按键 | GPIO | 功能定位 |
|:----:|:----:|---------|
| A | GPIO4 | 方向键 (上 / 下翻页) |
| B | GPIO3 | 副操作 (暂停 / 返回) |
| C | GPIO2 | 主操作 (确认 / 语音) ⚠️ Strapping 引脚 |
| D | GPIO1 | 模式切换 / 设置入口 |
| E | GPIO0 | 电源 (长按关机) |

---

## 界面：

### 按键

按键在全局中的功能

| KEY        | 单击     | 双击 | 长按     |
| ---------- | -------- | ---- | -------- |
| A（GPIO2） |          |      |          |
| B（GPIO4） |          |      |          |
| C（GPIO3） |          |      |          |
| D（GPIO1） | 切换模式 |      | 进入设置 |

### 界面切换：

####  CLOCK 时钟界面-Weather天气界面

按键：双击D切换CLOCK/WEATHER

#### Chatbot 界面

按键：长按A进行声音的录制识别，松手时上传，经过AI回答后显示到屏幕上，，短按B显示文字/隐藏文字

#### 待办任务界面 -倒计时界面

双击D切换代办界面和倒计时界面

单击按C同步手机平台的更新

单击A下一个

单击B显示细节-->转到Detail界面



Detail界面：

A：标记为已完成

B 开始执行倒计时

C 返回待办任务界面



倒计时界面

未开始状态：

B：单击 +5min，长按：1分钟1分钟的加，按多久加多久，松开就停止

A：单击 -5min，长按：1分钟一分钟的减，……

C：短按开始计时

开始倒计时状态：

B：短按暂停/继续，双击取消返回未开始界面

#### Setting界面

按键：A单击下一个，B单击上一个

C选择



## 项目架构

```
src/
├── config.h                    全局配置中心 (引脚、枚举、结构体、常量)
├── main.cpp                    主入口 — 状态定义、setup/loop、模式路由
│
├── hardware/                   硬件抽象层 (HAL) — 对硬件资源的封装
│   ├── button_hal.h / .cpp     5 键直连扫描 — 去抖、长按/双击
│   ├── button_events.h / .cpp  按键事件管理 — 全局分发 + 模式路由
│   └── display_hal.h / .cpp    SSD1306 OLED — 初始化、居中打印、图标、帧绘制
│
├── ui/                         UI 表现层 — 只依赖 hardware + config
│   ├── icons.h / .cpp          全部 PROGMEM 位图 (模式/电池/WiFi/动画帧)
│   ├── status_bar.h / .cpp     顶部 9px 状态栏 (图标、时间、标签)
│   └── animation.h / .cpp      开机动画 (滑入/停留/滑出)
│
├── services/                   服务层 — WiFi 与时间基础设施
│   ├── wifi_service.h / .cpp   WiFi 扫描 + 连接 (2.4G only)
│   └── ntp_service.h / .cpp    NTP 北京时间同步 + 自动重试
│
└── modes/                      应用模式层 — 6 种模式，每个实现 render + handle
    ├── mode_chat_robot.h / .cpp  默认模式 — 语音交互入口 (预留)
    ├── mode_clock.h / .cpp       大字时钟 + 日期星期
    ├── mode_countdown.h / .cpp   倒计时 + 番茄钟 (FOCUS→REST)
    ├── mode_todo.h / .cpp        待办列表/详情 → 可跳转倒计时
    ├── mode_weather.h / .cpp     今日天气 + 3 日预报
    └── mode_settings.h / .cpp    亮度 / 休眠 / AI 模式设置
```

### 分层架构 (面向对象视角)

```
┌─────────────────────────────────────────────────┐
│  main.cpp           应用控制器                    │
│  ┌─────────────┐  ┌─────────────┐               │
│  │ enterMode() │  │checkTimeout│  超时管理      │
│  │ cycleMode() │  │renderMode  │  渲染分发      │
│  └─────────────┘  └─────────────┘               │
├─────────────────────────────────────────────────┤
│  modes/             6 个应用模式                  │
│  每个模式 = render() + handleButtons()           │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐           │
│  │CHAT- │ │CLOCK │ │COUNT-│ │TODO  │ ...       │
│  │ROBOT │ │      │ │DOWN  │ │      │           │
│  └──────┘ └──────┘ └──────┘ └──────┘           │
├─────────────────────────────────────────────────┤
│  services/          基础设施服务                  │
│  ┌──────────┐  ┌──────────┐                     │
│  │WiFi 连接  │  │NTP 时间   │                     │
│  └──────────┘  └──────────┘                     │
├─────────────────────────────────────────────────┤
│  ui/                表现层                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐      │
│  │状态栏    │  │图标位图   │  │开机动画   │      │
│  └──────────┘  └──────────┘  └──────────┘      │
├─────────────────────────────────────────────────┤
│  hardware/          硬件抽象层 (HAL)             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐      │
│  │按键扫描   │  │按键事件   │  │OLED 显示 │      │
│  │5键直连   │  │管理分发   │  │SSD1306   │      │
│  └──────────┘  └──────────┘  └──────────┘      │
├─────────────────────────────────────────────────┤
│  config.h           全局配置中心                  │
│  引脚 / 枚举 / 结构体 / 常量 / extern 声明        │
└─────────────────────────────────────────────────┘
```

**依赖规则**：上层依赖下层，同层不互依赖。所有模块通过 `config.h` 共享全局状态，无循环依赖。

---

## 文件详解

### `config.h` — 全局配置中心

**职责**：项目的"单一数据源"。定义所有引脚、枚举、结构体、可调参数，以及跨模块共享的 `extern` 全局变量。

| 分类 | 内容 |
|------|------|
| **引脚定义** | `OLED_SDA/SCL` (GPIO8/9)、`KEY_A/B/C/D/E` (GPIO4/3/2/1/0)、`I2S_BCLK/LRC/DIN/DOUT` (GPIO5/6/7/10) |
| **屏幕常量** | `SCREEN_W=128`, `SCREEN_H=64`, `STATUSBAR_H=9`, `CONTENT_Y=9`, `CONTENT_H=55` |
| **按键参数** | `BTN_COUNT=5`, `DEBOUNCE_CNT=8` (~8ms @1kHz), `LONG_PRESS_MS_A/B=2000`, `LONG_PRESS_MS_C=1000`, `DOUBLE_TAP_MS=500` |
| **超时/刷新** | `TIMEOUT_MS=30000` (30s 无操作回主页), `TIME_REFRESH_MS=60000`, `NTP_RETRY_MS=120000` |
| **枚举** | `AppMode`(6 种模式), `ModeIcon`, `BattState`, `WifiState`, `CdState`, `CrSubState`, `TodoSubState`, `WxPage`, `SettingsSub` |
| **结构体** | `Button` — 按键状态机 (raw/last/pressed/released/longPress/doubleTap/pressTime/releaseTime/lastClickTime/longPressActive) |
| **extern 声明** | 所有跨模块全局变量: `buttons[]`, `btnRaw[]`, `currentMode`, 各模式子状态等 |

### `main.cpp` — 应用控制器

**职责**：定义所有全局状态实例，实现 `setup()` / `loop()` 主循环，以及模式路由、超时管理。按键事件处理已提取到 `hardware/button_events`。

#### 全局变量 (所有 `extern` 声明的定义)
```
buttons[5]        — 5 个 Button 结构体 (去抖后的按键状态)
btnRaw[5]         — 原始读数 (主循环读取)
debounceCnt[5]    — 去抖计数器
debounceState[5]  — 去抖稳定值
currentMode       — 当前应用模式
beforeSettings    — 进入设置前所在模式 (用于返回)
currentModeIcon   — 状态栏模式图标索引
currentBatt/Wifi  — 电池/WiFi 状态 (图标驱动)
lastUserAction    — 最后用户操作时刻 (超时检测)
...               — 各模式子状态变量
```

#### 核心函数
| 函数 | 签名 | 职责 |
|------|------|------|
| **enterMode** | `void enterMode(AppMode newMode)` | 切换到指定模式，重置子状态，清屏。若已在目标模式则直接返回 |
| **cycleMode** | `void cycleMode()` | D 键短按：循环切换 CHAT-ROBOT → CLOCK → COUNTDOWN → TODO → WEATHER → ...，跳过 SETTINGS |
| **checkTimeout** | `void checkTimeout()` | 30 秒无操作自动返回 CHAT-ROBOT（倒计时运行/暂停中除外） |
| **renderCurrentMode** | `void renderCurrentMode()` | 按 currentMode 路由到各 renderXxxMode() |

#### 主循环 `loop()` 时序
```
循环 (~1kHz):
  1. processButtonEvents()      ← button_events 封装: 去抖 + 全局分发 + 模式路由
  2. NTP 时间刷新 (每 60s)
  3. 倒计时 tick (CD_RUNNING 时)
  4. checkTimeout()             ← 超时回主页
  5. 渲染 (200ms 脏标记)        ← 状态栏 + 内容区
  6. delay(1)                   ← 让出 CPU
```

### `hardware/button_hal` — 按键扫描 HAL

**职责**：5 键直连 GPIO 的按键扫描。主循环中轮询 GPIO0-4 电平，做连续采样去抖和事件检测。

#### 函数清单
| 函数 | 可见性 | 职责 |
|------|--------|------|
| **initButtons** | public | 初始化 GPIO0-4 为 `INPUT_PULLUP`，初始化 `buttons[]` 等数组 |
| **readButtons** | public | 主循环调用。读取 5 个 GPIO，做连续采样去抖 (DEBOUNCE_CNT=8 次同值确认)，边沿检测生成 pressed/released/longPress/doubleTap 事件 |
| **debugPrintButtons** | public (条件编译) | 每 1s 通过串口打印 5 键原始值/去抖值/计数器/事件 |

#### 按键扫描数据流
```
[主循环: ~1kHz]

 readButtons()
   │
   ├─ 读 GPIO0-4 (digitalRead) ×5
   ├─ 连续 8 次同值 → 确认
   ├─ 边沿: HIGH→LOW = pressed
   ├─ 边沿: LOW→HIGH = released
   ├─ 长按计时: 超阈值 → longPress
   └─ 双击: B/D 500ms内两次 → doubleTap
              │
              ▼
      buttons[] → processButtonEvents()
```

#### 去抖算法
```
连续采样计数器:
  if raw == debounceState[i]:  cnt = 0     ← 一致, 确认稳定
  else:                        cnt++        ← 不一致, 累加
  if cnt >= DEBOUNCE_CNT(8):  state = raw  ← 连续 8 次同值, 切换状态
```
在 1kHz 采样率下，8 次 = **8 ms 去抖窗口**。

### `hardware/button_events` — 按键事件管理层

**职责**：位于硬件层之上、模式层之下。封装去抖读取、全局按键分发、模式路由。替代原 `main.cpp` 中的 `dispatchButtons()`。

#### 调用链
```
main.cpp loop()
  └─ processButtonEvents()                  ← 唯一对外入口
       ├─ readButtons()                      ← button_hal: 去抖 + 边沿检测
       ├─ debugPrintButtons()                ← button_hal: 调试输出
       ├─ dispatchGlobalButtons() [static]   ← D/E 跨模式: 电源/模式切换/设置入口
       └─ dispatchModeButtons()   [static]   ← A/B/C 按 currentMode 路由
            └─ handleXxxButtons()            ← mode_xxx.cpp
```

#### 函数清单
| 函数 | 可见性 | 职责 |
|------|--------|------|
| **processButtonEvents** | public | 主循环每帧调用。串联去抖→全局分发→模式路由 |
| dispatchGlobalButtons | static | E 长按→关机, D 双击→主页, D 长按→设置, D 短按→切模式 |
| dispatchModeButtons | static | `switch(currentMode)` 路由到 6 个 `handleXxxButtons()` |

### `hardware/display_hal` — OLED 显示 HAL

**职责**：封装 Adafruit_SSD1306 库，提供项目专用的绘制辅助函数。

#### 函数清单
| 函数 | 职责 |
|------|------|
| **displayInit** | `display.begin(SSD1306_SWITCHCAPVCC, 0x3C)`，失败返回 false |
| **printCentred** | 在指定 y 坐标居中打印字符串 (字符宽 6px) |
| **drawIcon** | 从 PROGMEM 逐 bit 绘制位图图标 (跳过 0 bit 加速) |
| **printLine** | 在指定行打印文本，支持 `hi=true` 反色高亮 (白底黑字) |
| **drawContentFrame** | 绘制 128×55 内容区域位图帧，支持 X 方向偏移 (用于滑动动画) |
| **drawFullScreen** | 绘制内容帧 → `display.display()` 提交 |

#### 全局对象
```cpp
Adafruit_SSD1306 display(128, 64, &Wire, -1);  // -1 = 无 RESET 引脚
```

### `ui/icons` — 图标位图库

**职责**：所有图标的 PROGMEM 位图数据定义 + 查找表。

| 数据 | 格式 | 用途 |
|------|------|------|
| `MODE_*_BMP[6]` | 12×8 px (16B) | 6 个模式图标: 时钟/沙漏/机器人/TODO/天气/齿轮 |
| `BATT_*_BMP[4]` | 12×8 px (16B) | 4 种电池状态: 满/中/低/充电中 |
| `WIFI_*_BMP[4]` | 12×8 px (16B) | 4 种 WiFi 状态: 强/中/弱/离线 |
| `frames[2]` | 128×48 px (768B) | 开机动画帧 frame3, frame5 |

每个类别都有对应的 `*_icon_table[]` 查找表，通过枚举值索引。

### `ui/status_bar` — 状态栏

**职责**：渲染屏幕顶部 9px 状态栏区域。

| 函数 | 职责 |
|------|------|
| **drawStatusBar** | 清空 → 绘模式图标(12×8) → 标签(3字符) → 时间(HH:MM) → WiFi图标 → 电池图标 → 底部分隔线 |
| **updateStatusBarTag** | 根据 `currentMode` 和子状态更新 `statusBarTag` (倒计时模式: RUN/PAU/DON/RST/RPA) |
| **updateModeIcon** | `currentMode` → `currentModeIcon` 枚举映射 |

### `ui/animation` — 开机动画

**职责**：开机时的 Logo 动画播放。

| 函数 | 职责 |
|------|------|
| **playAnimation** | frame3 滑入→停留→滑出 → frame5 滑入→保持 3s (期间刷新 NTP)→滑出。每帧 16 steps, 90ms/step |
| **transitSlide** | 模式切换滑动动画 (预留，当前直接清屏重绘) |

### `services/wifi_service` — WiFi 连接

| 函数 | 职责 |
|------|------|
| **connectWiFi** | ① 扫描 2.4G AP 列表 → ② 确认目标 SSID 存在 → ③ `WiFi.begin()` 连接 (15s 超时) → ④ 显示 IP 或失败信息。全程在 OLED 上显示进度 |

### `services/ntp_service` — NTP 时间

| 函数 | 职责 |
|------|------|
| **ntpInit** | `configTime(UTC+8, 无夏令时, ntp.aliyun.com)` |
| **updateTime** | ① 检查 WiFi 状态更新图标 → ② `getLocalTime()` 获取北京时间 → ③ 写入 `timeStr` (HH:MM 格式) → ④ 失败时每 2 分钟自动重新请求 NTP |

### `modes/` — 6 种应用模式

每个模式遵循**统一接口契约**：

```cpp
// 每个 mode_xxx.h 对外暴露的两个函数:
void renderXxxMode();        // 绘制内容区 (CONTENT_Y ~ 63)
void handleXxxButtons();     // 处理本模式 A/B/C 键事件
```

#### `mode_chat_robot` (模式 0) — 默认主界面

| 函数 | 职责 |
|------|------|
| `renderChatRobotIdle` | 显示 "CHAT-ROBOT" + "Ready" + 底部提示 "Hold C:Voice B:Mode" |
| `handleChatRobotButtons` | C 长按→语音输入(预留), C 短按→反馈, A/B→反馈 |

子状态枚举: `CrSubState { CR_EXPR, CR_LISTEN, CR_THINK, CR_REPLY }`

#### `mode_clock` (模式 1) — 大字时钟

| 函数 | 职责 |
|------|------|
| `renderClockMode` | 大字 HH:MM (冒号每秒闪烁) → 日期+星期 → 分隔线 → 底部信息栏 (温度/TODO计数) → 按键提示 |
| `handleClockButtons` | C→任务弹窗(预留), B→跳转 TODO 模式 |

#### `mode_countdown` (模式 2) — 倒计时 & 番茄钟

| 函数 | 职责 |
|------|------|
| `renderCountdownMode` | 按 cdState 渲染 4 种界面: **IDLE** (预设值+进度条), **RUNNING/PAUSED** (MM:SS+进度条+结束时间), **DONE** (笑脸+时长+D:再次) |
| `handleCountdownButtons` | 完整状态机: C→开始/取消/再次, B→暂停/继续, BB→重置, A→±1min/循环预设 |
| **drawCdProgressBar** | 在指定 y 绘制 120×4px 进度条，支持闪烁 (<1min 剩余时闪) |
| **getCdEndTime** | 计算当前时间 + 剩余秒数 → "End: HH:MM" |

番茄钟流程: 专注阶段完成 → 自动进入 5min 休息 → 休息完成弹 DONE

| 按键 | CD_IDLE | CD_RUNNING | CD_PAUSED | CD_DONE |
|------|---------|-----------|----------|---------|
| C | 开始倒计时 | 长按 1s 取消 | — | 重新开始 |
| B | — | 暂停 | 继续 | — |
| B 双击 | — | 重置 | 重置 | — |
| A | ±1 分钟 / 循环预设 | — | — | — |

#### `mode_todo` (模式 3) — 待办事项

| 函数 | 职责 |
|------|------|
| `renderTodoMode` | 按 todoSub 渲染: **LIST** (标题+4行列表, 选中反色), **DETAIL** (标题+优先级+截止+预计), **空态** |
| `handleTodoButtons` | LIST: A→滚动, C→进详情, B→标记完成; DETAIL: C→以此任务时间启动倒计时, B→返回 |

子状态枚举: `TodoSubState { TODO_LIST, TODO_DETAIL }`

#### `mode_weather` (模式 4) — 天气

| 函数 | 职责 |
|------|------|
| `renderWeatherMode` | 按 wxPage 渲染: **TODAY** (城市+大温度+描述+湿度/风力+体感), **FORECAST** (3天列表, 含选中高亮) |
| `handleWeatherButtons` | C→切换今日/预报, B→回今日, A→滚动预报日期 |

子状态枚举: `WxPage { WX_TODAY, WX_FORECAST }`

#### `mode_settings` (模式 5) — 系统设置

| 函数 | 职责 |
|------|------|
| `renderSettingsMode` | 按 settingsSub 渲染: **MAIN** (7项菜单), **BRIGHTNESS** (百分比+滑条), **SLEEP** (5档超时), **AI** (3种引擎) |
| `handleSettingsButtons` | 统一 A→选择, C→确认/进入, B→返回。亮度子页面 C 键写入 SSD1306 对比度寄存器 |

子状态枚举: `SettingsSub { SETTINGS_MAIN, SETTINGS_BRIGHTNESS, SETTINGS_SLEEP, SETTINGS_AI }`

---

## 具体机制详解

### 按键机制详解

#### 架构

```
主循环 ~1kHz 轮询

 readButtons()                                      ← 每帧调用
  ├─ digitalRead(GPIO0-4) ×5                        ← 读取 5 键电平
  ├─ 连续采样去抖: 8 次同值确认 (~8ms)
  ├─ 边沿检测 → pressed / released (单帧有效)
  ├─ 长按检测 → longPress (超阈值触发一次)
  ├─ 双击检测 → doubleTap (B/D 键 500ms 内两次)
  └─ 写入 buttons[]
         │
         ▼
 processButtonEvents()                              ← ~1kHz 调用
  ├─ dispatchGlobalButtons() → D/E 跨模式全局处理
  └─ dispatchModeButtons()   → A/B/C 按模式分发
```

#### 性能指标

| 指标 | 数值 |
|------|------|
| 采样率 | ~1,000 Hz (每帧扫描全部 5 键) |
| 去抖窗口 | ~8 ms |
| 按下→确认延迟 | < 12 ms |
| CPU 占用 | 5 次 digitalRead / 帧，几乎可忽略 |

---

### 应用模式总览

#### 模式切换

- **D 短按**：循环切换 CHAT-ROBOT → CLOCK → COUNTDOWN → TODO → WEATHER → ...
- **D 长按 (2s)**：进入 SETTINGS，记录 `beforeSettings` 用于返回
- **D 双击**：任意模式直接跳转 CHAT-ROBOT 主页
- **30s 无操作**：自动返回 CHAT-ROBOT（倒计时运行/暂停中除外）

#### 按键角色分配

| 按键 | GPIO | 全局角色 | 各模式角色 |
|:----:|:----:|---------|-----------|
| **A** | GPIO4 | — | 方向: 上/下/± |
| **B** | GPIO3 | — | 副操作: 暂停/返回/取消 |
| **C** | GPIO2 | — | 主操作: 开始/确认/进入 |
| **D** | GPIO1 | 模式切换 (短按/长按/双击) | — |
| **E** | GPIO0 | 电源键 (长按关机) | — |

---

## 构建与烧录

### 环境

- PlatformIO (VS Code 扩展 或 CLI)
- ESP32 Arduino Core v3.0+

### 依赖

```ini
lib_deps =
    adafruit/Adafruit SSD1306 @ ^2.5.7
    adafruit/Adafruit GFX Library @ ^1.11.9
```

### 命令

```bash
pio run               # 编译
pio run -t upload     # 编译 + 烧录
pio device monitor    # 串口监视 (115200 bps)
```

### 调试开关

`config.h` 中：

```cpp
#define BTN_DEBUG      1   // 1=串口按键调试输出 (每 1s), 0=关闭
```

调试输出格式：
```
BTN:[00100] deb:[00100] cnt:[00005] fire:[1,]
    原始5键   稳定5键   去抖计数   按键事件
```

---

## 诊断工具



|      |      |
|------|------|
|      |      |
|      |      |

---

## 问题修复

### P0 — 阻塞性 Bug
（已全部修复）

### P1 — 功能 Bug
（已全部修复）

### P2 — 半成品 / 代码质量问题

| # | 文件 | 问题 | 状态 |
|---|------|------|:----:|
| 1 | `mode_chat_robot.cpp` | **CrSubState 状态机闲置**: `config.h` 定义了 `CR_EXPR/LISTEN/THINK/REPLY` 4 态，`main.cpp` 初始化了 `crSub`，但 `renderChatRobotIdle()` 完全不读 `crSub`，永远画同一界面 | ✕ |
| 2 | `mode_weather.cpp`<br>`mode_todo.cpp`<br>`mode_clock.cpp` | **模式数据硬编码**: 天气(城市/温度/预报)、TODO(3条假任务)、时钟底部信息栏("Sun 26C"/"TODO:3") 全为静态占位。依赖 MQTT/BLE 接入后替换为真实数据 | ✕ |

### P3 — 次要问题

| # | 文件 | 问题 | 状态 |
|---|------|------|:----:|
| 3 | `main.cpp:290-293` | **OLED 初始化失败死循环无反馈**: `for(;;)` 永久挂起，未接串口的用户无法知道发生了什么。应加蜂鸣器提示或屏幕闪烁报错 | ✕ |
| 5 | `mode_settings.cpp:122-131` | **主菜单 4 个选项为空壳**: "WiFi Setup"(0)、"Volume"(2)、"Sound Mode"(3)、"Device Info"(6) 按 C 键无任何响应，`handleSettingsButtons()` 的 switch 未处理这些 case | ✕ |
| 6 | `main.cpp:43-44` | **WiFi 密码明文硬编码**: SSID 和密码写在源码中。如需公开仓库，应移到独立 `secrets.h` 并加入 `.gitignore` | ✕ |
| 7 | `mode_weather.cpp` | **预报页 B 键未重置光标**: `handleWeatherButtons` 中 B 键回今日时未重置 `wxDayCursor=0`，下次进入预报页光标停在旧位置 | ✕ |
