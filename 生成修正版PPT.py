#!/usr/bin/env python3
"""桌面智能灵动岛 - 答辩PPT（修正版）基于实际代码+m4结构"""

from pptx import Presentation
from pptx.util import Inches, Pt
from pptx.dml.color import RGBColor
from pptx.enum.text import PP_ALIGN
from pptx.enum.shapes import MSO_SHAPE
import os

BG = RGBColor(0x1A, 0x1A, 0x2E)
ACCENT = RGBColor(0x00, 0xD2, 0xFF)
WHITE = RGBColor(0xFF, 0xFF, 0xFF)
GRAY = RGBColor(0xAA, 0xAA, 0xAA)
GREEN = RGBColor(0x2E, 0xCC, 0x71)
ORANGE = RGBColor(0xFF, 0x9F, 0x43)
RED = RGBColor(0xFF, 0x6B, 0x6B)
PURPLE = RGBColor(0xA8, 0x55, 0xF7)

prs = Presentation()
prs.slide_width = Inches(13.333)
prs.slide_height = Inches(7.5)

def add_bg(s, c=BG):
    s.background.fill.solid()
    s.background.fill.fore_color.rgb = c

def tb(s, l, t, w, h, txt, sz=18, c=WHITE, bold=False, align=PP_ALIGN.LEFT, name='Microsoft YaHei'):
    b = s.shapes.add_textbox(Inches(l), Inches(t), Inches(w), Inches(h))
    f = b.text_frame; f.word_wrap = True
    p = f.paragraphs[0]; p.text = txt
    p.font.size = Pt(sz); p.font.color.rgb = c; p.font.bold = bold
    p.font.name = name; p.alignment = align
    return f

def title(s, t, y=0.4):
    tb(s, 0.8, y, 11.7, 1.0, t, sz=36, c=ACCENT, bold=True)

def subtitle(s, t, y=1.3):
    tb(s, 0.8, y, 11.7, 0.8, t, sz=20, c=GRAY)

def body(s, t, y=2.0, sz=17, c=WHITE):
    tb(s, 0.8, y, 11.7, 5.0, t, sz=sz, c=c)

def accent_line(s, y=1.15):
    l = s.shapes.add_shape(MSO_SHAPE.RECTANGLE, Inches(0.8), Inches(y), Inches(2.0), Inches(0.04))
    l.fill.solid(); l.fill.fore_color.rgb = ACCENT; l.line.fill.background()

def pn(s, n, total=22):
    tb(s, 11.5, 7.0, 1.5, 0.4, f"{n}/{total}", sz=12, c=GRAY, align=PP_ALIGN.RIGHT)

def ns():
    s = prs.slides.add_slide(prs.slide_layouts[6])
    add_bg(s); return s

def card(s, l, t, w, h, tt, bt, tc=ACCENT, bc=WHITE):
    c = s.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, Inches(l), Inches(t), Inches(w), Inches(h))
    c.fill.solid(); c.fill.fore_color.rgb = RGBColor(0x25, 0x25, 0x3D); c.line.fill.background()
    tb(s, l+0.2, t+0.15, w-0.4, 0.5, tt, sz=18, c=tc, bold=True)
    tb(s, l+0.2, t+0.6, w-0.4, h-0.8, bt, sz=13, c=bc)

def bullets(s, items, ys=2.2, sz=17, c=WHITE, bullet='▸', gap=0.45):
    for i, it in enumerate(items):
        tb(s, 1.2, ys+i*gap, 11.0, 0.5, f"{bullet} {it}", sz=sz, c=c)

# ═══════════════ P1 封面 ═══════════════
sl = ns()
tb(sl, 0, 1.5, 13.33, 1.5, "桌面智能灵动岛", sz=56, c=ACCENT, bold=True, align=PP_ALIGN.CENTER)
tb(sl, 0, 3.0, 13.33, 1.0, "Desktop Smart Island", sz=22, c=GRAY, align=PP_ALIGN.CENTER)
accent_line(sl, 3.9); sl.shapes[-1].left = Inches(5.6); sl.shapes[-1].width = Inches(2.0)
tb(sl, 0, 4.3, 13.33, 0.6, "ESP32-C3 OLED 桌面终端 | 智谱 AI 语音对话 | Web 面板 + MQTT 联动", sz=16, c=GRAY, align=PP_ALIGN.CENTER)
tb(sl, 0, 5.5, 13.33, 0.5, "2025-2026 「人人卓越」工程 · 第三阶段B选题", sz=14, c=GRAY, align=PP_ALIGN.CENTER)
tb(sl, 0, 6.2, 13.33, 0.5, "答辩日期：2026/06/06-07", sz=14, c=GRAY, align=PP_ALIGN.CENTER)

# ═══════════════ P2 项目简介 ═══════════════
sl = ns()
title(sl, "项目简介"); accent_line(sl)
tb(sl, 0.8, 1.6, 11.7, 1.0, "一个放在桌面的小型 OLED 屏幕小狗，会说话、能计时、管任务", sz=28, c=WHITE, bold=True)
body(sl, """桌面智能灵动岛是一台基于 ESP32-C3 SuperMini 的桌面 OLED 智能终端。它通过 I2S 麦克风采集语音、调用智谱大模型进行 AI 对话与语音合成，在 0.96 寸 OLED 上展示 6 种交互模式（时钟/倒计时/待办/天气/设置/AI 对话）。Web 面板通过本地 Mosquitto MQTT Broker 与设备实时双向联动：创建任务、控制番茄钟、同步 AI 对话。还附带一个 Flutter App 骨架用于未来手机端扩展。""", sz=17, c=GRAY, y=2.4)
bullets(sl, [
    "🤖 AI 语音对话 — 长按录音 → 智谱 ASR → GLM-4 → TTS 语音播报",
    "🍅 番茄钟任务 — OLED 大屏倒计时，Web 面板远程控制 + MQTT 同步",
    "📋 任务管理 — Web 创建任务 → MQTT 推 ESP32 → OLED 显示+标记完成",
    "🐕 电子宠物 — OLED 表情包 + 事件动画 + Web 端 MP4 宠物视频",
    "🖥 Web + MQTT — 左宠物+右面板，DeepSeek Agent 驱动 AI 对话",
], ys=4.0, sz=15, bullet='')
pn(sl, 2)

# ═══════════════ P3 目录 ═══════════════
sl = ns()
title(sl, "答辩目录"); accent_line(sl)
secs = [
    ("01","项目背景","痛点与项目定位"),
    ("02","系统总架构","云端服务 + ESP32 + Web/Flutter"),
    ("03","硬件设计","BOM、引脚分配、I2S 总线、电源"),
    ("04","ESP32 固件","6 模式状态机 + 分层架构 + 按键体系"),
    ("05","Web 面板 + Agent","AI Agent 框架 + 4 Tab + Cloudflare Worker"),
    ("06","通信协议","本地 Mosquitto MQTT + 8 主题设计"),
    ("07","技术难点","I2S 复用、GPIO 紧缺、双模 AI、WDT"),
    ("08","演示 & 总结","演示流程 + 代码量 + 团队分工 + 展望"),
]
for i, (num, t, d) in enumerate(secs):
    r, c = i // 2, i % 2
    x, y = 0.8 + c * 6.0, 1.8 + r * 1.15
    tb(sl, x, y, 0.8, 0.5, num, sz=32, c=ACCENT, bold=True)
    tb(sl, x+0.7, y, 5.0, 0.5, t, sz=22, c=WHITE, bold=True)
    tb(sl, x+0.7, y+0.4, 5.0, 0.4, d, sz=14, c=GRAY)
pn(sl, 3)

# ═══════════════ P4 痛点与定位 ═══════════════
sl = ns()
title(sl, "痛点分析与项目定位"); accent_line(sl)
bullets(sl, [
    "桌面效率工具冰冷：番茄钟 App、Todo 列表、闹钟 —— 全是数字，缺乏温度",
    "AI 音箱太大太贵，不适合放桌面做专注伴侣",
    "需要一个能看见、能互动、有'情绪'的桌面实体 — 看得见摸得着的电子宠物",
], ys=1.8, sz=18, bullet='✕')
tb(sl, 0.8, 3.4, 11.7, 0.6, "我们的答案", sz=24, c=GREEN, bold=True)
bullets(sl, [
    "一台 ESP32-C3 驱动的 OLED 屏幕小狗（< ￥80）",
    "语音聊、帮计时、管任务 — 长按录音和它对话",
    "PC Web 面板实时联动 — 左看宠物视频，右控任务番茄钟",
    "Flutter App 骨架已就绪，未来无缝扩展到手机",
], ys=4.0, sz=18, c=GREEN, bullet='✓')
pn(sl, 4)

# ═══════════════ P5 子项目总览 ═══════════════
sl = ns()
title(sl, "子项目总览（3 座代码仓库）"); accent_line(sl)
card(sl, 0.6, 1.8, 3.8, 4.5, "1. ESP32 固件",
    """📁 MCU_design/src/  39 文件 ~3,500 行
C/C++ · PlatformIO · Arduino
6 模式状态机 · 4 键输入
I2S 全双工 · 智谱 ASR/AI/TTS
MQTT Client · ADC 电量检测
完成度: ████████░░ 85%""", GREEN)
card(sl, 4.7, 1.8, 4.0, 4.5, "2. Web 前端+后端",
    """📁 desktop_web/  8 文件 ~3,200 行
HTML/CSS/JS  SPA + Agent
4 Tab · 左栏宠物视频 + 右栏面板
DeepSeek API · 8 Function Calling
Cloudflare Worker 搜索代理
完成度: ████████░░ 80%""", ORANGE)
card(sl, 9.0, 1.8, 3.8, 4.5, "3. Flutter App",
    """📁 appdesign/  25 文件 ~2,500 行
Dart · Provider · Material 3
4 Tab + 开幕动画 · MQTT 完整
⚠ 数据全 Mock · 未接入真实源
完成度: ██░░░░░░░░ 25%""", PURPLE)
pn(sl, 5)

# ═══════════════ P6 系统总架构 ═══════════════
sl = ns()
title(sl, "系统总架构"); accent_line(sl)
body(sl, """┌────────────────── 云端服务 ──────────────────────────────────────────┐
│  智谱 AI (GLM-4-Flash  +  ASR +  TTS)  │  DeepSeek API (Web Agent)  │  Cloudflare Worker (搜索/ASR代理) │
└──────────────────────┬───────────────────────────────────────────────┘
                       │ HTTPS                                HTTPS
┌──────────────────────┴────────────────────────────────────────────────────┐
│                          PC 端 (localhost)                                  │
│  ┌──────────────────────────┐   ┌──────────────┐   ┌───────────────────┐  │
│  │  Web 控制面板 (SPA)      │   │  Mosquitto   │   │  Flutter App      │  │
│  │  HTML + CSS + JS         │←→│  MQTT Broker │←→│  (骨架 25%)       │  │
│  │  DeepSeek Agent + 8 工具 │   │  TCP 1883    │   │  Mock 数据        │  │
│  └──────────────────────────┘   │  WS  8083    │   └───────────────────┘  │
│                                  └──────┬───────┘                          │
└─────────────────────────────────────────┼──────────────────────────────────┘
                                          │ MQTT (WiFi)
┌─────────────────────────────────────────┴──────────────────────────────────┐
│                          ESP32-C3 SuperMini                                 │
│  ┌──────────┐ ┌───────────┐ ┌───────────┐ ┌────────────┐ ┌──────────┐    │
│  │ 0.96 OLED│ │ INMP441   │ │MAX98357A  │ │ 4 按键      │ │ TP4056   │    │
│  │ SSD1306  │ │ I2S 麦克风 │ │ I2S 功放   │ │ (GPIO 1-4) │ │ 充电管理  │    │
│  │ I2C      │ │           │ │ + 喇叭     │ │ + 电池 ADC │ │ 14250    │    │
│  └──────────┘ └───────────┘ └───────────┘ └────────────┘ └──────────┘    │
└────────────────────────────────────────────────────────────────────────────┘""", sz=11, c=WHITE, y=1.8)
pn(sl, 6)

# ═══════════════ P7 数据流 ═══════════════
sl = ns()
title(sl, "核心数据流"); accent_line(sl)
card(sl, 0.6, 1.8, 5.8, 2.5, "语音交互全链路 (智谱全家桶)",
    """长按按键 A(1s) → INMP441 I2S 录音 (16kHz 24→16bit, 最长 3s)
→ HTTP POST PCM → 智谱 GLM-ASR-2512 (流式 base64)
→ 返回文字 → 智谱 GLM-4-Flash AI 对话 → 返回回复文本
→ OLED 显示 + 智谱 glm-tts HTTP 流 → I2S TX → MAX98357A → 喇叭

全程延迟：~3-5s  |  全部走智谱 API，无需百度密钥""")
card(sl, 6.9, 1.8, 5.5, 2.5, "Web Agent 对话流程 (DeepSeek)",
    """用户在 Web 面板输入消息 → Agent.run()
→ think (DeepSeek API + Soul 人设 + Memory)
→ tool_call (识别需要调用 8 个工具之一)
→ observe (工具结果注入上下文)
→ think (继续推理) → respond (最终回复)
→ 自动 MQTT publish → ESP32 OLED 显示

8 工具: createTask/updateTask/deleteTask/listTasks
         toggleTask/getDDLSummary/getWeather/webSearch""")
card(sl, 0.6, 4.6, 12.0, 2.0, "MQTT 双向同步",
    """┌─────────────────────────────────────────────────────────────────────────────────────┐
│  ESP32 ──status──→ Web (在线/电量/状态)  │  Web ──task/add──→ ESP32 (新增任务)            │
│  Web ──pomodoro──→ ESP32 (远程番茄钟)    │  Web ──display──→ ESP32 (OLED 弹通知)          │
│  ESP32 ──voice/input──→ Web (ASR 文本)   │  Web ──chat──→ ESP32 (AI 回复推 OLED)          │
│  ESP32 ←──cmd── Web (切模式/调亮度/关屏) │  ESP32 ←──task/done──→ Web (标记完成, 双向)     │
└─────────────────────────────────────────────────────────────────────────────────────┘""")
pn(sl, 7)

# ═══════════════ P8 硬件总览 + BOM ═══════════════
sl = ns()
title(sl, "硬件总览"); accent_line(sl)
subtitle(sl, "BOM 物料清单  |  总成本 < ￥80  |  全部已购")
body(sl, """┌────────────────────┬─────────────────────┬──────┬──────────┐
│ 模块               │ 型号                │ 数量 │ 价格     │
├────────────────────┼─────────────────────┼──────┼──────────┤
│ 主控               │ ESP32-C3 Super Mini │   1  │  ¥8-12   │
│ 显示屏             │ 0.96" OLED SSD1306  │   1  │  ¥8-12   │
│ 麦克风             │ INMP441 I2S         │   1  │  ¥6-10   │
│ 功放               │ MAX98357A I2S       │   1  │  ¥8-12   │
│ 喇叭               │ 3W 4Ω 圆形          │   1  │  ¥3-5    │
│ 充电模块           │ TP4056 Type-C       │   1  │  ¥2-3    │
│ 电池               │ 14250 3.7V 300mAh   │   1  │  ¥8-12   │
│ 轻触按键           │ 6×6×5mm             │   4  │  ¥2      │
│ 电阻/面包板/杜邦线 │ 10kΩ ×4 / 400孔     │ 若干 │  ¥15     │
├────────────────────┴─────────────────────┴──────┼──────────┤
│                              合计              │  ~¥60-80 │
└───────────────────────────────────────────────┴──────────┘""", sz=14, c=WHITE, y=1.9)
pn(sl, 8)

# ═══════════════ P9 引脚分配 ═══════════════
sl = ns()
title(sl, "GPIO 引脚分配（依据 config.h 实际代码）"); accent_line(sl)
body(sl, """┌──────────┬────────────────────┬──────────────────────────────────┬──────┐
│ GPIO     │ 功能               │ 连接对象                         │ 备注 │
├──────────┼────────────────────┼──────────────────────────────────┼──────┤
│ GPIO0    │ 电池 ADC           │ 分压电路 → ADC1_CH0              │ 原按键 GPIO 损坏 │
│ GPIO1    │ 按键 D (模式中枢)  │ 轻触开关 → GND                   │ 短按/双击/长按   │
│ GPIO2    │ 按键 A (方向/录音) │ 轻触开关 → GND                   │ Strapping, INPUT │
│ GPIO3    │ 按键 C (确认/开始) │ 轻触开关 → GND                   │ INPUT_PULLUP     │
│ GPIO4    │ 按键 B (暂停/返回) │ 轻触开关 → GND                   │ INPUT_PULLUP     │
│ GPIO5    │ I2S BCLK           │ INMP441 SCK + MAX98357A BCLK     │ 共用             │
│ GPIO6    │ I2S LRC            │ INMP441 WS + MAX98357A LRC       │ 共用             │
│ GPIO7    │ I2S DIN            │ MAX98357A DIN (音频输出)         │ ESP32 → 功放     │
│ GPIO8    │ I2C SDA            │ OLED SDA                         │ —                │
│ GPIO9    │ I2C SCL            │ OLED SCL                         │ —                │
│ GPIO10   │ I2S DOUT           │ INMP441 SD (麦克风输入)          │ INMP441 → ESP32  │
│ GPIO20   │ AMP_SD             │ MAX98357A SD 关断控制            │ LOW=关断 HIGH=正常│
├──────────┴────────────────────┴──────────────────────────────────┴──────┤
│ ⚠ 11 个可用 GPIO 全部用完 (GPIO11=Flash, GPIO21=USB CDC)               │
│ ⚠ 原设计 5-6 键 → 实际 4 键 (GPIO0 硬件损坏改为 ADC)                    │
│ ⚠ 按键 10kΩ 上拉 → 改用 ESP32 内部 INPUT_PULLUP (~45kΩ)                │
└─────────────────────────────────────────────────────────────────────────┘""", sz=11, c=WHITE, y=1.8)
pn(sl, 9)

# ═══════════════ P10 I2S 总线 ═══════════════
sl = ns()
title(sl, "I2S 总线共用方案"); accent_line(sl)
body(sl, """INMP441 (录音) 和 MAX98357A (播放) 共用 BCLK(GPIO5) 和 LRC(GPIO6)，软件层面通过 i2sInitRecord() / i2sInitPlay() 动态切换 RX/TX 模式。

           ESP32-C3                          MAX98357A             INMP441
           ────────                          ──────────            ────────
GPIO5  ──→ BCLK ────────────┬──────────────→ BCLK        ┌───────→ SCK
GPIO6  ──→ LRC  ────────────┼──────────────→ LRC         ├───────→ WS
GPIO7  ──→ DIN  ────────────┼──────────────→ DIN         │
GPIO10 ←── DOUT ────────────┼────────────────────────────└─────── SD

                    电源与配置:
                    VIN  → 电池 3.7V (直供, 峰值 0.5A)         VDD  → 3.3V (禁止 5V!)
                    GND  → GND                                  GND  → GND
                    GAIN → GND (12dB 增益)                      L/R  → GND (左声道, 禁悬空)
                    SD   → GPIO20 (关断控制)
                    + /- → 喇叭 4Ω 3W

⚠ 关键约束:  录音前功放先关断 (GPIO20 LOW) 防止自激啸叫，两者互斥工作"""
    , sz=12, c=WHITE, y=1.8)
pn(sl, 10)

# ═══════════════ P11 按键 & 电源 ═══════════════
sl = ns()
title(sl, "按键体系 & 电源架构"); accent_line(sl)
card(sl, 0.6, 1.8, 6.0, 4.5, "4 键直连 GPIO（非设计中 6 键 74HC138D）",
    """按键   GPIO    去抖方案              支持事件
─────────────────────────────────────────────────
A     GPIO2    主循环 1kHz 轮询        短按 / 长按(1s 触发录音)
B     GPIO4    8 次同值确认 (~8ms)    短按 / 双击(500ms 窗口)
C     GPIO3    按下到确认 < 12ms      短按 (确认/开始)
D     GPIO1                           短按(切模式)/双击(配对)/长按(2s 进设置)

• 全用 INPUT_PULLUP 内部上拉 (GPIO2 例外用 INPUT 因 Strapping)
• D 键是模式切换中枢: 短按循环→双击配对→长按进 SETTINGS
• 设计 6 键(含 74HC138D) → 实际简化: GPIO0 硬件损坏 + 代码重构为 4 键""")
card(sl, 6.9, 1.8, 5.5, 4.5, "电源架构",
    """USB 5V → TP4056 → 14250 锂电池 (3.7V 300mAh)
                      │
         ┌────────────┴────────────┐
         │ 3.7V 直供 (大电流)       │ AMS1117 → 3.3V (小电流)
         ▼                         ▼
    MAX98357A VIN              ESP32 + OLED + INMP441
    (峰值 0.5A, 不走 LDO)      (合计 ~100mA)

电池 ADC:  V_bat ─┬─ R1(100k) ─┬─→ GPIO0 ADC1_CH0
                  └─ R2(100k) ─┘
    V_adc = V_bat × 0.5  →  8 次采样取平均 → 0-100%
    10s 间隔读取 (BATT_READ_MS)""")
pn(sl, 11)

# ═══════════════ P12 固件架构 ═══════════════
sl = ns()
title(sl, "ESP32 固件架构（分层设计）"); accent_line(sl)
body(sl, """main.cpp (应用控制器: enterMode / cycleMode / checkTimeout / renderCurrentMode)
      │
      ├── modes/ (6 个应用模式, 每个 = renderXxxMode + handleXxxButtons)
      │      ├ mode_chat_robot.cpp   语音 AI 流水线 + 40 行对话历史
      │      ├ mode_clock.cpp        大字时钟 + 日期星期
      │      ├ mode_countdown.cpp    倒计时 + 番茄钟 (A/B ±5min 连按加速)
      │      ├ mode_todo.cpp         待办 LIST/DETAIL 两态 (MQTT 驱动)
      │      ├ mode_weather.cpp      天气 TODAY/FORECAST (硬编码)
      │      └ mode_settings.cpp     7 项: WiFi/亮度(NVS)/睡眠/AI 模式
      │
      ├── services/ (6 服务)         ├── hardware/ (7 硬件 HAL)
      │   ├ wifi_service.cpp              ├ button_hal.cpp    4 键扫描+去抖
      │   ├ ntp_service.cpp               ├ display_hal.cpp    SSD1306+中文渲染
      │   ├ asr_service.cpp (智谱)        ├ mic_hal.cpp        INMP441 24→16bit 录音
      │   ├ ai_service.cpp (智谱)         ├ i2s_hal.cpp        RX/TX 动态切换
      │   ├ tts_service.cpp (智谱)        ├ batt_hal.cpp       电池 ADC
      │   └ mqtt_service.cpp              ├ buzzer_hal.cpp     LED 反馈状态机
      │                                    └ audio_test.cpp    诊断工具
      └── ui/ (表现层: icons.cpp / status_bar.cpp / animation.cpp)

依赖规则: 上层依赖下层, 同层不互依赖, 全部通过 config.h 共享全局状态""", sz=11, c=WHITE, y=1.8)
pn(sl, 12)

# ═══════════════ P13 模式状态机 ═══════════════
sl = ns()
title(sl, "6 模式状态机 & 按键映射"); accent_line(sl)
body(sl, """┌──────────────────────────────── 模式流转 ────────────────────────────────┐
│  开机 → CHAT-ROBOT (默认中枢)                                                │
│     │ D 键短按循环:  CHAT-ROBOT → CLOCK → TODO → COUNTDOWN → WEATHER → 回   │
│     │ D 键双击配对:  CLOCK↔WEATHER  |  TODO↔COUNTDOWN                       │
│     │ D 键长按 2s:   进 SETTINGS → B 键退出                                  │
│     │ 30s 无操作:     自动回 CHAT-ROBOT (COUNTDOWN 运行中除外)                │
│     │ CHAT-ROBOT:     长按 A 录音 → ASR → AI → TTS  → 回复态(B/C 翻页)       │
└─────────────────────────────────────────────────────────────────────────────┘

    模式            A 键 (GPIO2)        B 键 (GPIO4)         C 键 (GPIO3)       D 键 (GPIO1)
    ──────────     ──────────────     ────────────────     ───────────────    ─────────────────
    CHAT-ROBOT    长按=录音           回复态=返回表情包     回复态=上一页       模式切换中枢
    CLOCK         无操作              无操作               无操作              (全局)
    TODO          下一个任务          无操作              查看详情/开始
    COUNTDOWN     -5min (连按加速)    暂停/继续 (BB=重置)   开始               全局短按/双击/长按
    WEATHER       上一页              下一页               无操作
    SETTINGS      上移菜单            下移菜单             确认                B=返回 CHAT-ROBOT""", sz=11, c=WHITE, y=1.8)
pn(sl, 13)

# ═══════════════ P14 Web 面板 ═══════════════
sl = ns()
title(sl, "Web 控制面板 & AI Agent"); accent_line(sl)
card(sl, 0.6, 1.8, 6.0, 4.5, "Web 面板布局 (左宠物视频 + 右 4 Tab)",
    """┌──────────────┬────────────────────────────────┐
│ 左侧固定(380px) │ 右侧自适应                        │
│ ┌────────────┐  │ ┌ 主页 │ 任务 │ 对话 │ 设置 ┐  │
│ │ 宠物视频    │  │ ├──────────────────────────┤  │
│ │ (MP4 3个)  │  │ │         Tab 内容区         │  │
│ │ 状态标签    │  │ │         (可滚动)           │  │
│ └────────────┘  │ │                            │  │
│ 三指标卡        │ └────────────────────────────┘  │
│ (任务/番茄/电量)│                                   │
└──────────────────────────────────────────────────┘
3 个宠物视频: pet_happy.mp4 / pet_thinking.mp4 / pet_sleeping.mp4
根据设备状态自动切换 (空闲→开心, AI思考中→思考, 休眠→睡觉)""")
card(sl, 6.9, 1.8, 5.5, 4.5, "AI Agent 框架 (DeepSeek)",
    """用户消息 → Agent.run()
    → think (DeepSeek API + Soul 人设 + Memory 记忆)
    → tool_call (识别需要调用工具 → 调用工具)
    → observe (工具返回结果注入上下文)
    → think (继续推理，可循环多轮)
    → respond (最终回复 + 自动 MQTT 推送到 ESP32 OLED)

    Agent 可用的 8 个 Function Calling 工具:
    createTask  updateTask  deleteTask  listTasks
    toggleTask  getDDLSummary  getWeather  webSearch

    联网搜索通过 Cloudflare Worker 中转 (DuckDuckGo)""")
pn(sl, 14)

# ═══════════════ P15 MQTT 通信协议 ═══════════════
sl = ns()
title(sl, "通信协议 — 本地 Mosquitto MQTT"); accent_line(sl)
body(sl, """Broker: Mosquitto (PC 端运行)  |  TCP 1883 + WebSocket 8083  |  配置: config.h MQTT_BROKER_IP

┌──────────────────────────────┬───────────────────┬──────────────────────────────────────────┐
│ MQTT 主题                     │ 方向              │ 说明                                     │
├──────────────────────────────┼───────────────────┼──────────────────────────────────────────┤
│ desk/island/status            │ ESP32 → Web       │ 设备状态 (JSON: online/battery/state/      │
│                              │                   │   pomodoro_remain/wifi_rssi/version/uptime)│
│ desk/island/display           │ Web → ESP32       │ OLED 通知弹窗 (JSON: text/duration)        │
│ desk/island/cmd               │ Web → ESP32       │ 控制: switch_mode/set_brightness/screen_off│
│ desk/island/task/add          │ Web → ESP32       │ 新增任务 (id/title/priority/deadline)       │
│ desk/island/task/done         │ 双向              │ 标记任务完成 (task_id/done)                 │
│ desk/island/pomodoro          │ Web → ESP32       │ 远程番茄钟 (start/pause/resume/reset)       │
│ desk/island/chat              │ Web → ESP32       │ AI 回复推送到 OLED 对话历史                 │
│ desk/island/voice/input       │ ESP32 → Web       │ ASR 识别文本转发给 Web Agent                │
└──────────────────────────────┴───────────────────┴──────────────────────────────────────────┘

ESP32 端: mqtt_service.cpp — 订阅 6 主题 + 每 10s 上报 status
Web 端:   mqtt.js — WebSocket 连接到 Mosquitto 8083""", sz=12, c=WHITE, y=1.8)
pn(sl, 15)

# ═══════════════ P16 Flutter App ═══════════════
sl = ns()
title(sl, "Flutter App 骨架（25% 完成度）"); accent_line(sl)
card(sl, 0.6, 1.8, 5.8, 4.5, "已完成部分",
    """✅ 4 Tab 页面 + 开幕动画 + 弹窗 (页面结构齐全)
├ main.dart         入口 (Provider + MaterialApp + 开幕动画)
├ models/           3 数据模型 (DeviceState / Task / ChatMessage)
├ providers/        3 状态管理 (Device / Task / Chat, ChangeNotifier)
├ screens/          6 页面 (开幕/主页/任务/对话/设置 + 主框架)
├ widgets/          5 组件 (宠物视频/对话气泡/任务卡片/指标卡/日总结)
├ services/         MQTT 完整实现 + mDNS 设备发现
└ mock/             3 套 Mock 数据""", GREEN)
card(sl, 6.9, 1.8, 5.5, 4.5, "未完成 & 待办",
    """❌ 所有 Provider 初始化读 Mock 文件
❌ 没有接入任何真实数据源 (MQTT 连接代码完整但未联调)
❌ 设备发现 (mDNS) 代码完整但未测试

待办:
• 将 Mock 替换为真实 MQTT 连接 (mqtt_service.dart 完整)
• 与 ESP32 联调任务同步 + 番茄钟
• 对接 Cloudflare Worker 做 Web 同款 Agent
• 上架 Google Play / App Store""", RED)
pn(sl, 16)

# ═══════════════ P17 技术难点 ═══════════════
sl = ns()
title(sl, "关键技术难点与解决方案"); accent_line(sl)

# (l, t, w, h, title, body, title_color)
cards17 = [
    (0.6, 1.4, 6.0, 2.8, "难点一：GPIO 引脚资源极度紧缺",
     "问题: ESP32-C3 仅 11 个可用 GPIO · 需驱动 OLED/麦克风/功放/按键/电量检测\n\n"
     "尝试方案: 74HC138 3-8译码器扩展按键 (4脚→6键) → 焊接问题致芯片损坏, 放弃\n"
     "GPIO0 物理损坏 (静电/过温) → 损失 1 键, 改为电池 ADC 分压检测\n\n"
     "退化过程: 6键+LED → HC138损坏 → 5键无LED → GPIO0损坏 → 最终 4 键\n"
     "4 键替代方案: D 键扛 3 事件 (短按切模式/双击配对/长按进设置)\n"
     "A 键按模式变换角色 (CHAT-ROBOT 长按录音 / COUNTDOWN 短按-5min)\n"
     "全用 INPUT_PULLUP 省外部电阻 · GPIO2 因 Strapping 特殊处理为 INPUT\n\n"
     "代码: config.h KEY_PIN_A=2,B=4,C=3,D=1 · button_hal.cpp 1kHz 轮询+8次去抖", RED),

    (6.9, 1.4, 5.5, 2.8, "难点二：语音全链路 (两代架构演进)",
     "第一代 (ESP32 直连智谱 AI→OLED):\n"
     "  • ai_service.cpp 限制 5 个英文单词 → 回复无信息量, 方案失败\n"
     "  • 为什么限制 5 词？OLED 128×64 内容区仅 48px 高, 中文一行 8 字\n\n"
     "第二代 (ESP32→MQTT→Web DeepSeek Agent→MQTT→ESP32):\n"
     "  • agent.js: think→tool_call→observe 多轮推理 + 8 Function Calling\n"
     "  • 回复质量好但延迟长: 2s 录音 + 7-12s 处理, 用户松手等太久\n\n"
     "关键技术创新 — 流式 base64 编码 (asr_service.cpp):\n"
     "  • ESP32-C3 仅 400KB RAM → 无法一次构建 WAV+base64 (128KB)\n"
     "  • streamBase64Concat(): WAV头+PCM→480B 分块边编码边发送\n"
     "  • WAV头与PCM必须连续 → 避免错误'='填充符致 API 失败\n\n"
     "遗留: 15s WEB_AI_TIMEOUT 太短, Agent 多轮 tool_call 偶尔超时", ORANGE),

    (0.6, 4.4, 6.0, 2.8, "难点三：MQTT 通信 — 阻塞重连 + Broker IP 硬编码",
     "问题一: PubSubClient.connect() 同步阻塞\n"
     "  • Broker 不可达时 TCP connect 超时数秒 → 主循环完全卡死\n"
     "  • 按键扫描/OLED 渲染/倒计时/看门狗全部停摆\n"
     "  • ESP32-C3 单核 RISC-V, 无法用 xTaskCreatePinnedToCore 隔离\n"
     "  • 当前仅: 5s 冷却窗口 + 已连接时 mqttClient.loop() 非阻塞\n\n"
     "问题二: Broker IP 写死 (config.h MQTT_BROKER_IP \"172.21.1.146\")\n"
     "  • PC DHCP 换 IP 后失效 → 每 5s 触发一次阻塞式重连\n"
     "  • 两问题耦合: IP 不可达→频繁重连→主循环反复卡顿\n\n"
     "补救: 修 IP 动态配置 (NVS 持久化) 即可大幅减少重连触发\n"
     "根治: 换 AsyncMQTTClient (回调非阻塞) 或 FreeRTOS 任务隔离 MQTT", PURPLE),

    (6.9, 4.4, 5.5, 2.8, "难点四：Flutter APK 构建失败 → Web 退守",
     "问题: flutter build apk 在国内网络环境下全部失败\n"
     "  • 依赖链: Gradle 发行版→插件→Maven→Android SDK→Kotlin→Dart pub\n"
     "  • 配了阿里云/腾讯云/Flutter国内镜像 + Clash 代理 (127.0.0.1:7890)\n"
     "  • 镜像无法覆盖全部环节 (pub.dev/dl.google.com/部分 Kotlin)\n"
     "  • .gradle/ 下四版本目录 → 反复调版本兼容性\n\n"
     "退守: Flutter App (25文件~2500行) UI已完整, 改 Web SPA 方案\n"
     "  • 浏览器即运行时, 无需 Gradle/Android SDK\n"
     "  • Cloudflare Pages: desktoppet.pages.dev (静态前端托管)\n"
     "  • Cloudflare Workers: 搜索代理 + ASR 代理\n"
     "  • 国内演示仍用本地 Live Server (Cloudflare 国内不稳定)\n\n"
     "未来: PWA 包装 Web = 接近原生体验 | 或用完整 Flutter 镜像链重试", GRAY),
]
for l, t, w, h, tt, bt, tc in cards17:
    card(sl, l, t, w, h, tt, bt, tc, WHITE)
pn(sl, 17)

# ═══════════════ P18 演示流程 ═══════════════
sl = ns()
title(sl, "现场演示流程（10 步预设脚本）"); accent_line(sl)
body(sl, """┌──────┬──────────────────────────────────────────────────────────────────────┐
│ 步骤 │ 演示内容                                                              │
├──────┼──────────────────────────────────────────────────────────────────────┤
│  1   │ 上电 → 开机滑动动画 → 进入 CHAT-ROBOT                                  │
│  2   │ 按 D 键 → 循环切换 CLOCK / TODO / COUNTDOWN / WEATHER 各模式            │
│  3   │ 回到 CHAT-ROBOT → 长按 A → OLED 录音提示 → 语音提问                     │
│  4   │ 松开 A → 智谱 ASR 识别 → AI 思考动画 → 回复文字 + TTS 语音播报          │
│  5   │ 打开 PC Web 面板 (localhost) → 左栏宠物视频 + 右栏主页状态              │
│  6   │ Web 对话 Tab → 输入"帮我创建一个明天下午3点的任务"→ Agent 拆解+创建       │
│  7   │ MQTT → ESP32 OLED 弹通知 → TODO 模式可见新任务                          │
│  8   │ Web 番茄钟 → 启动 1 分钟 → ESP32 OLED 同步倒计时 → 完成闪烁              │
│  9   │ ESP32 按键标记任务完成 → MQTT → Web 面板同步更新                         │
│ 10   │ (备) SETTINGS → 亮度调节 / 睡眠模式 / AI 模式查看                        │
└──────┴──────────────────────────────────────────────────────────────────────┘

⏱ 总时长 3~4 分钟 | A 操作 ESP32 + D 展示 Web 面板 | PC 开热点 + Mosquitto 本地 无需外网""", sz=12, c=WHITE, y=1.8)
pn(sl, 18)

# ═══════════════ P19 代码量统计 ═══════════════
sl = ns()
title(sl, "代码量统计 & 技术栈"); accent_line(sl)
body(sl, """┌──────────────────┬──────────┬───────────┬──────────────────────────────────────────────┐
│ 模块             │ 文件数   │ 代码行数   │ 技术栈                                        │
├──────────────────┼──────────┼───────────┼──────────────────────────────────────────────┤
│ ESP32 固件       │   39     │  ~3,500   │ C/C++ · PlatformIO · Arduino · I2S · MQTT    │
│ Web 前端 JS      │    5     │  ~2,200   │ JavaScript ES6 · Fetch · MQTT.js · SPA       │
│ Web HTML + CSS   │    2     │  ~1,200   │ HTML5 · CSS3 · Flexbox · CSS 变量暖光主题     │
│ Cloudflare Worker│    1     │   ~230    │ JavaScript · DuckDuckGo 搜索代理 · ASR 代理   │
│ Flutter App      │   25     │  ~2,500   │ Dart · Provider · Material 3 · MQTT · mDNS   │
├──────────────────┼──────────┼───────────┼──────────────────────────────────────────────┤
│ 合计             │   72     │  ~9,630   │ 5 种语言 · 3 座代码仓库                       │
└──────────────────┴──────────┴───────────┴──────────────────────────────────────────────┘

    硬件: ESP32-C3 · OLED SSD1306 · INMP441 · MAX98357A · TP4056 · 14250 电池
    AI:  智谱 GLM-4-Flash · 智谱 ASR · 智谱 TTS · DeepSeek API (Web Agent)
    通信: 本地 Mosquitto MQTT (TCP 1883 + WebSocket 8083)
    后端: Cloudflare Worker (搜索代理 + ASR 代理)
    固件: Arduino/C++ · PlatformIO · I2S · I2C · WiFi · NTP · JSON
    Web:  HTML/CSS/JS · MQTT.js · Function Calling Agent
    App:  Flutter/Dart · Provider · mDNS""", sz=11, c=WHITE, y=1.8)
pn(sl, 19)

# ═══════════════ P20 团队分工 ═══════════════
sl = ns()
title(sl, "团队分工与协作"); accent_line(sl)
body(sl, """┌──────────────┬─────────────────────────────────┬───────────────────────────────────┐
│ 角色         │ 主要职责                         │ 关键交付物                         │
├──────────────┼─────────────────────────────────┼───────────────────────────────────┤
│ A 硬件&结构  │ BOM 采购 · 面包板搭建 · 焊接     │ 完整可用的硬件原型                  │
│              │ 电池 ADC 分压 · 按键电路验证      │ 嘉立创原理图 · BOM < ￥80           │
│              │ GPIO0 损坏应对 (改为 ADC)         │                                    │
├──────────────┼─────────────────────────────────┼───────────────────────────────────┤
│ B 嵌入式基础 │ ESP32 工程搭建 (PlatformIO)       │ ESP32 基础固件                      │
│              │ 6 模式状态机 · OLED 显示+中文      │ config.h 全局配置 (188 行)          │
│              │ WiFi/NTP · 按键 HAL · 电池 ADC    │ main.cpp 应用控制器 (502 行)         │
│              │ 智谱 ASR/AI/TTS 对接              │ 6 个 Mode 文件 · 7 个 HAL 文件       │
├──────────────┼─────────────────────────────────┼───────────────────────────────────┤
│ C Web+后端   │ Web 控制面板 (SPA + 4 Tab)        │ Web 面板 (8 文件 ~3,200 行)          │
│              │ DeepSeek AI Agent + 8 工具        │ Cloudflare Worker (搜索+ASR代理)     │
│              │ Mosquitto 搭建 · MQTT 联调        │ 4 个 MP4 宠物动画 (Premiere Pro)     │
│              │ Cloudflare Worker 部署            │                                    │
├──────────────┼─────────────────────────────────┼───────────────────────────────────┤
│ D App + 设计 │ Flutter App (骨架, 25%)           │ Flutter App (25 文件 ~2,500 行)      │
│              │ 界面与功能设计文档                 │ 设计文档 (界面/电路/引脚映射)         │
│              │ 答辩 PPT · 流程图 HTML            │                                    │
└──────────────┴─────────────────────────────────┴───────────────────────────────────┘

协作方式: Git 分支协作 · 统一 MQTT 主题约定 (config.h + mqtt.js + mqtt_service.dart 三端一致)""", sz=11, c=WHITE, y=1.8)
pn(sl, 20)

# ═══════════════ P21 项目总结 ═══════════════
sl = ns()
title(sl, "项目总结 & 未来展望"); accent_line(sl)
card(sl, 0.6, 1.8, 5.8, 2.5, "✅ 已完成",
    """• ESP32-C3 固件: 6 模式状态机 + 39 文件 ~3,500 行
• 智谱全家桶: ASR → AI → TTS 全链路语音交互
• I2S 总线共用: INMP441 + MAX98357A 动态切换
• Web 面板: DeepSeek Agent + 8 工具 + MQTT 联动
• 本地 Mosquitto Broker: TCP 1883 + WS 8083
• 4 键直接 GPIO + 电池 ADC 检测
• 软 WDT · NVS 持久化 · WiFi 交互式配网
• Flutter App 骨架 (4 Tab + MQTT 代码就绪)""", GREEN)
card(sl, 6.9, 1.8, 5.5, 2.5, "🔮 未来展望",
    """• 3D 打印外壳 + 成品组装展示
• Flutter App 接入真实数据源 (MQTT 已就绪)
• ESP32-S3 升级 (更多 GPIO + 更强 NPU)
• 语音唤醒词 (免按键触发)
• 环境传感器扩展 (温湿度/光线)
• 多设备 Mesh 联动""", ORANGE)
card(sl, 0.6, 4.6, 11.7, 1.8, "⚙ 软件工程亮点",
    """• 分层架构: main → modes → services/ui/hardware → config.h
• 同层不互依赖，全部通过 config.h extern 共享全局状态
• 3 端 MQTT 主题完全一致 (ESP32 config.h / Web mqtt.js / Flutter mqtt_service.dart)
• Agent 框架: think → tool_call → observe 可循环推理
• 去抖精度: 主循环 1kHz 轮询，8 次同值确认 ≈ 8ms, < 12ms 响应""", ACCENT)
pn(sl, 21)

# ═══════════════ P22 致谢 ═══════════════
sl = ns()
tb(sl, 0, 2.5, 13.33, 1.5, "感谢聆听", sz=56, c=ACCENT, bold=True, align=PP_ALIGN.CENTER)
tb(sl, 0, 4.0, 13.33, 0.8, "桌面智能灵动岛 — Desktop Smart Island", sz=24, c=GRAY, align=PP_ALIGN.CENTER)
accent_line(sl, 4.6); sl.shapes[-1].left = Inches(5.6); sl.shapes[-1].width = Inches(2.0)
tb(sl, 0, 5.2, 13.33, 1.0, "欢迎提问 🙋", sz=18, c=GRAY, align=PP_ALIGN.CENTER)

# ── 保存 ──
out = os.path.expanduser(r"C:\Users\ASUS\OneDrive\Desktop\desktopdog\修正版\桌面智能灵动岛_答辩PPT（修正版）.pptx")
prs.save(out)
print(f"OK — {len(prs.slides)} 页 → {out}")
