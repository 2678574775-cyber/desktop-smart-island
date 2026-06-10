#!/usr/bin/env python3
"""桌面智能灵动岛 - 答辩PPT生成脚本"""

from pptx import Presentation
from pptx.util import Inches, Pt, Emu
from pptx.dml.color import RGBColor
from pptx.enum.text import PP_ALIGN, MSO_ANCHOR
from pptx.enum.shapes import MSO_SHAPE
import os

# ── 配色 ──
BG_DARK  = RGBColor(0x1A, 0x1A, 0x2E)    # 深蓝黑背景
ACCENT   = RGBColor(0x00, 0xD2, 0xFF)    # 青色强调
WHITE    = RGBColor(0xFF, 0xFF, 0xFF)
GRAY     = RGBColor(0xAA, 0xAA, 0xAA)
ORANGE   = RGBColor(0xFF, 0x9F, 0x43)
GREEN    = RGBColor(0x2E, 0xCC, 0x71)
RED      = RGBColor(0xFF, 0x6B, 0x6B)

prs = Presentation()
prs.slide_width  = Inches(13.333)
prs.slide_height = Inches(7.5)

# ── 工具函数 ──
def add_bg(slide, color=BG_DARK):
    bg = slide.background
    fill = bg.fill
    fill.solid()
    fill.fore_color.rgb = color

def add_text_box(slide, left, top, width, height, text, font_size=18, color=WHITE, bold=False, align=PP_ALIGN.LEFT, font_name='Microsoft YaHei'):
    txBox = slide.shapes.add_textbox(Inches(left), Inches(top), Inches(width), Inches(height))
    tf = txBox.text_frame
    tf.word_wrap = True
    p = tf.paragraphs[0]
    p.text = text
    p.font.size = Pt(font_size)
    p.font.color.rgb = color
    p.font.bold = bold
    p.font.name = font_name
    p.alignment = align
    return tf

def add_title(slide, text, y=0.4):
    add_text_box(slide, 0.8, y, 11.7, 1.0, text, font_size=36, color=ACCENT, bold=True, align=PP_ALIGN.LEFT)

def add_subtitle(slide, text, y=1.3):
    add_text_box(slide, 0.8, y, 11.7, 0.8, text, font_size=20, color=GRAY)

def add_body(slide, text, y=2.2, font_size=18, color=WHITE):
    add_text_box(slide, 0.8, y, 11.7, 5.0, text, font_size=font_size, color=color)

def add_accent_line(slide, y=1.15):
    line = slide.shapes.add_shape(MSO_SHAPE.RECTANGLE, Inches(0.8), Inches(y), Inches(2.0), Inches(0.04))
    line.fill.solid()
    line.fill.fore_color.rgb = ACCENT
    line.line.fill.background()

def add_page_number(slide, num, total=23):
    add_text_box(slide, 11.5, 7.0, 1.5, 0.4, f"{num}/{total}", font_size=12, color=GRAY, align=PP_ALIGN.RIGHT)

def new_slide():
    layout = prs.slide_layouts[6]  # blank
    slide = prs.slides.add_slide(layout)
    add_bg(slide)
    return slide

def add_card(slide, left, top, width, height, title_text, body_text, title_color=ACCENT, body_color=WHITE):
    """添加卡片式内容块"""
    card = slide.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, Inches(left), Inches(top), Inches(width), Inches(height))
    card.fill.solid()
    card.fill.fore_color.rgb = RGBColor(0x25, 0x25, 0x3D)
    card.line.fill.background()
    add_text_box(slide, left + 0.2, top + 0.15, width - 0.4, 0.5, title_text, font_size=18, color=title_color, bold=True)
    add_text_box(slide, left + 0.2, top + 0.6, width - 0.4, height - 0.8, body_text, font_size=14, color=body_color)

def add_bullet_items(slide, items, y_start=2.2, font_size=17, color=WHITE, bullet='▸', line_spacing=0.45):
    """批量添加带符号的要点"""
    for i, item in enumerate(items):
        y = y_start + i * line_spacing
        add_text_box(slide, 1.2, y, 11.0, 0.5, f"{bullet} {item}", font_size=font_size, color=color)

# ══════════════════════════════════════════
# P1: 封面
# ══════════════════════════════════════════
s = new_slide()
add_bg(s, BG_DARK)
add_text_box(s, 0, 1.5, 13.33, 1.5, "桌面智能灵动岛", font_size=56, color=ACCENT, bold=True, align=PP_ALIGN.CENTER)
add_text_box(s, 0, 3.0, 13.33, 1.0, "Desktop Smart Island — AI Desktop Companion", font_size=22, color=GRAY, align=PP_ALIGN.CENTER)
add_accent_line(s, 3.9); s.shapes[-1].left = Inches(5.6); s.shapes[-1].width = Inches(2.0)
add_text_box(s, 0, 4.3, 13.33, 0.6, "基于 ESP32-C3 的桌面智能助手 | AI 语音对话 | 番茄钟任务管理 | 微信小程序联动", font_size=16, color=GRAY, align=PP_ALIGN.CENTER)
add_text_box(s, 0, 5.5, 13.33, 0.5, "2025-2026 「人人卓越」工程 · 第三阶段B选题", font_size=14, color=GRAY, align=PP_ALIGN.CENTER)
add_text_box(s, 0, 6.2, 13.33, 0.5, "答辩日期：2026/06/06-07", font_size=14, color=GRAY, align=PP_ALIGN.CENTER)

# ══════════════════════════════════════════
# P2: 项目简介
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "项目简介")
add_accent_line(s)
add_text_box(s, 0.8, 1.6, 11.7, 1.0, "一个会说话、能帮你管任务、有表情的桌面电子宠物", font_size=28, color=WHITE, bold=True)
add_body(s, """桌面智能灵动岛是一款集 AI 语音对话、番茄钟任务管理、天气时钟显示、手机小程序联动于一体的桌面智能助手。它以 ESP32-C3 为主控，搭载 OLED 表情显示、I2S 语音交互，通过 MQTT 与微信小程序实时双向同步，成为用户桌面上一个有"人格"的数字化伴侣。""", font_size=18, color=GRAY, y=2.4)
add_bullet_items(s, [
    "🎤 AI 语音对话 —— 长按按键说话，松开AI秒回，支持云端+本地双模",
    "🍅 智能番茄钟 —— OLED 大屏倒计时，手机远程控制，专注统计",
    "📋 任务管理 —— 手机创建任务 → 设备同步提醒 → 一键完成",
    "🤖 电子宠物 —— 8套表情随机切换，事件触发特殊表情动画",
    "🌤 天气时钟 —— NTP校时 + 和风天气实时显示",
], y_start=3.8, font_size=17, bullet='')
add_page_number(s, 2)

# ══════════════════════════════════════════
# P3: 目录
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "答辩目录")
add_accent_line(s)
sections = [
    ("01", "项目背景", "痛点分析与功能全景"),
    ("02", "系统架构", "整体架构与数据流"),
    ("03", "硬件设计", "BOM、电路、I2S音频、按键矩阵"),
    ("04", "嵌入式固件", "状态机、OLED显示、语音全链路、AI双模"),
    ("05", "手机小程序", "5 Tab页面、MQTT双端联动"),
    ("06", "技术难点", "无DAC方案、GPIO紧缺、AI双模切换"),
    ("07", "现场演示", "完整交互流程演示"),
    ("08", "团队总结", "分工与展望"),
]
for i, (num, title, desc) in enumerate(sections):
    row = i // 2
    col = i % 2
    x = 0.8 + col * 6.0
    y = 1.8 + row * 1.2
    add_text_box(s, x, y, 0.8, 0.5, num, font_size=32, color=ACCENT, bold=True)
    add_text_box(s, x + 0.7, y, 5.0, 0.5, title, font_size=22, color=WHITE, bold=True)
    add_text_box(s, x + 0.7, y + 0.4, 5.0, 0.4, desc, font_size=14, color=GRAY)
add_page_number(s, 3)

# ══════════════════════════════════════════
# P4: 痛点分析
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "我们要解决什么？")
add_accent_line(s)
add_subtitle(s, "桌面效率工具大多冷冰冰——一个番茄钟App、一个闹钟、一个Todo列表，没有人味儿")
add_bullet_items(s, [
    "传统番茄钟 / 闹钟只是冷冰冰的数字 + 通知铃声",
    "Todo 列表躺在手机里，极易被忽略，缺少「物理存在感」",
    "AI 音箱太大、太贵，不适合放在桌面上做专注伴侣",
    "需要一个能看见、能互动、有「情绪」的桌面实体",
], y_start=2.4, font_size=18, bullet='✕')
add_text_box(s, 0.8, 4.5, 11.7, 0.6, "我们的答案： 一个低成本（<￥80）、可触摸、有表情、会说话的桌面智能宠物", font_size=22, color=GREEN, bold=True)
add_page_number(s, 4)

# ══════════════════════════════════════════
# P5: 功能全景
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "功能全景")
add_accent_line(s)
features = [
    ("⏰ 时钟天气", "NTP 自动校时\n大字时间 + 日期星期\n和风天气实时显示\n6 套天气图标"),
    ("🍅 番茄钟任务", "25+5 专注模式\n手机远程启动\n倒计时进度条\n完成统计"),
    ("🎤 AI 语音对话", "长按录音 → ASR 识别\n智谱 GLM / 本地 Ollama\nTTS 语音播报回复\n双模自动切换"),
    ("📱 手机联动", "微信小程序控制\n任务双向同步\nMQTT 实时通信\n远程番茄钟 + AI 对话"),
]
for i, (title, body) in enumerate(features):
    add_card(s, 0.6 + i * 3.1, 1.8, 2.8, 4.5, title, body)
add_page_number(s, 5)

# ══════════════════════════════════════════
# P6: 系统架构
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "系统总架构")
add_accent_line(s)
add_body(s, """┌────────────────── 云端服务 ──────────────────┐
│  智谱AI (GLM-4-Flash)  │  百度ASR/TTS  │  和风天气  │  EMQX MQTT  │
└────────────────────┬───────────────────────────┘
                     │ HTTPS / MQTT
      ┌──────────────┴──────────────┐
      │         WiFi 局域网          │
      ├─────────────────────────────┤
      │                             │
      ▼                             ▼
┌──────────────┐          ┌────────────────┐
│ ESP32-C3     │  MQTT    │  微信小程序     │
│ Super Mini   │◄════════►│  5 Tab 页面    │
│              │          │                │
│ OLED 128×64  │          │ 设备状态总览    │
│ INMP441 麦克  │          │ 任务 CRUD      │
│ MAX98357A   │          │ AI 对话        │
│ 按键 × 6    │          │ 番茄钟远程      │
│ TP4056 充电  │          │ WiFi 配网      │
└──────────────┘          └────────────────┘""", font_size=15, color=WHITE, y=1.8)
add_page_number(s, 6)

# ══════════════════════════════════════════
# P7: 数据流 & 语音全链路
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "核心数据流")
add_accent_line(s)
add_card(s, 0.6, 1.8, 5.8, 2.5,
    "语音交互全链路",
    "长按按键C → INMP441 录音(PCM 16kHz) → HTTP POST 百度 ASR → 返回文字\n→ HTTP POST 智谱 AI → 返回回复 → OLED 显示 + 百度 TTS 合成语音\n→ MAX98357A 功放 → 喇叭播放\n\n全程延迟：云端 ~2s  /  本地 Ollama ~4s")
add_card(s, 6.8, 1.8, 5.8, 2.5,
    "MQTT 任务同步",
    "小程序创建任务 → MQTT publish (device/{id}/task/add)\n→ ESP32 收到 → OLED 弹通知动画 → 存入本地列表\n\n小程序标记完成 → MQTT publish (device/{id}/task/done)\n→ ESP32 更新列表 → LED 闪烁确认\n\n番茄钟支持双端互启互停")
add_card(s, 0.6, 4.6, 12.0, 2.0,
    "AI 双模切换策略",
    "开机检测智谱 API 连通性 → 同时扫描局域网 Ollama (GET :11434/api/tags)\n→ 默认云端优先，超时 3s 自动切本地\n→ OLED 状态栏显示 ☁云端 / 🏠本地 图标")
add_page_number(s, 7)

# ══════════════════════════════════════════
# P8: 硬件总览
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "硬件总览")
add_accent_line(s)
add_subtitle(s, "BOM 物料清单  |  总成本 < ￥80")
add_body(s, """┌───────────────────┬────────────────────┬──────┬──────────┐
│ 模块              │ 型号               │ 数量 │ 价格     │
├───────────────────┼────────────────────┼──────┼──────────┤
│ 主控              │ ESP32-C3 Super Mini│   1  │  ¥8-12   │
│ 显示屏            │ 0.96" OLED I2C SSD1306│ 1  │  ¥8-12   │
│ 麦克风            │ INMP441 I2S        │   1  │  ¥6-10   │
│ 功放              │ MAX98357A I2S      │   1  │  ¥8-12   │
│ 按键扫描          │ 74HC138D 3-8译码器 │   1  │  ¥2-3    │
│ 轻触开关          │ 6×6×5mm            │   6  │  ¥3      │
│ 喇叭              │ 3W 4Ω 圆形         │   1  │  ¥3-5    │
│ 充电管理          │ TP4056 Type-C      │   1  │  ¥2-3    │
│ 电池              │ 14250 3.7V 300mAh  │   1  │  ¥8-12   │
│ 电阻/杜邦线/面包板│ —                  │ 若干 │  ¥10     │
├───────────────────┴────────────────────┴──────┼──────────┤
│                              合计             │  ~¥60-80 │
└──────────────────────────────────────────────┴──────────┘""", font_size=14, color=WHITE, y=1.9)
add_page_number(s, 8)

# ══════════════════════════════════════════
# P9: 电路设计 + 引脚分配
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "电路设计 — 引脚分配")
add_accent_line(s)
add_body(s, """┌──────────┬────────────────────────┬──────────────────────────────────┐
│ GPIO     │ 功能                   │ 连接                             │
├──────────┼────────────────────────┼──────────────────────────────────┤
│ GPIO0~2  │ KEY_A/B/C (74HC138)    │ → 74HC138D A/B/C 地址输入         │
│ GPIO3    │ KEY_IN (按键读回)       │ ← 6按键公共端 + 10kΩ上拉 +3V3     │
│ GPIO4    │ I2S_WS / OLED I2C?     │ → INMP441 WS + MAX98357A LRC     │
│ GPIO5    │ I2S_SCK / OLED I2C?    │ → INMP441 SCK + MAX98357A BCLK   │
│ GPIO6    │ I2S_SD (麦克风数据)     │ ← INMP441 SD                     │
│ GPIO7    │ I2S_DOUT (音频输出)     │ → MAX98357A DIN                  │
│ GPIO8    │ I2C_SDA                │ ↔ OLED SDA                       │
│ GPIO9    │ I2C_SCL                │ → OLED SCL                       │
│ GPIO10   │ LED_STATUS             │ → LED (经220Ω) + GND             │
│ 5V       │ VBAT (电池)            │ ← TP4056 BAT+                    │
│ 3V3      │ +3V3 外设供电          │ → OLED/INMP441/74HC138            │
│ GND      │ GND                    │ 全部共地                          │
├──────────┴────────────────────────┴──────────────────────────────────┤
│ GPIO 占用: 11/16  |  剩余: GPIO20/21 (USB串口) + GPIO3 已用         │
└────────────────────────────────────────────────────────────────────┘""", font_size=13, color=WHITE, y=1.8)
add_page_number(s, 9)

# ══════════════════════════════════════════
# P10: I2S 音频子系统
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "I2S 音频子系统设计")
add_accent_line(s)
add_body(s, """共享 I2S 总线架构：

                    ESP32-C3 (I2S Master)
                         │
          ┌──────────────┼──────────────┐
          │ BCLK(GPIO5)  │ LRC(GPIO4)   │
          ▼               ▼              ▼
    ┌──────────┐   ┌──────────┐   ┌──────────┐
    │ INMP441  │   │ MAX98357A│   │          │
    │ 麦克风    │   │ 功放      │   │          │
    │          │   │          │   │          │
    │ SCK◄─────┤   │ BCLK◄────┤   │          │ ← 同一条 BCLK
    │ WS ◄─────┤   │ LRC ◄────┤   │          │ ← 同一条 LRC
    │ SD ──────┼──►│          │   │          │   (ESP32 RX)
    │          │   │ DIN ◄────┼───┤ GPIO10   │   (ESP32 TX)
    └──────────┘   └──────────┘   └──────────┘
         ▲               │
         │               ▼
      声音采集         喇叭播放

⚠️ 关键选型决策：
ESP32-C3 没有内置 DAC → 不能直接用 PAM8403 (模拟输入)
→ 选 MAX98357A I2S 数字功放，纯数字输入，无需 DAC，效率 92%

INMP441 供电只支持 3.3V（禁止接 5V！）
MAX98357A VIN 接电池 3.7V 直供（功放峰值电流 0.5A，不走 LDO）""", font_size=14, color=WHITE, y=1.8)
add_page_number(s, 10)

# ══════════════════════════════════════════
# P11: 按键扫描 — 74HC138D
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "按键方案：74HC138D 3-8 译码器")
add_accent_line(s)
add_card(s, 0.6, 1.8, 6.0, 4.5,
    "为什么用 74HC138？",
    "问题：需要 6 个按键，但 ESP32-C3 仅剩 4 个可用 GPIO\n\n74HC138 方案：\n  • 3 地址线 (GPIO0/1/2) + 1 读回线 (GPIO3)\n  • 4 脚 → 扫 6 键（可扩至 8 键）\n  • Y0~Y5 依次输出低电平，任一按下 KEY_IN 拉低\n\n对比方案：\n  ✕ 直连：6 脚 6 键（GPIO 不够）\n  ✕ Charlieplexing：3 脚 6 键但需二极管防鬼键\n  ✓ 74HC138：4 脚 6 键，硬件可靠，扫描逻辑简单")
add_card(s, 6.9, 1.8, 5.5, 4.5,
    "电路拓扑",
    """        74HC138D              KEY_IN
       ┌──────────┐         ┌──10kΩ── +3V3
GPIO0──│A      Y0│──SW1──┐  │
GPIO1──│B      Y1│──SW2──┤  │
GPIO2──│C      Y2│──SW3──┼──┼── GPIO3
 GND───│G2A    Y3│──SW4──┤  │
 GND───│G2B    Y4│──SW5──┤  │
+3V3───│G1     Y5│──SW6──┘  │
+3V3───│VCC   Y6│── NC     │
 GND───│GND   Y7│── NC     │
       └──────────┘         │
                       读回：0=按下  1=未按

扫描：for(i=0;i<6;i++){地址=i; if(!KEY_IN)按键i被按}
每轮仅 6 次 GPIO 操作，耗时 < 100μs""")
add_page_number(s, 11)

# ══════════════════════════════════════════
# P12: 固件架构
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "嵌入式固件架构")
add_accent_line(s)
add_body(s, """状态机总图：

                      ┌─────────────┐
            ┌────────►│ CHAT-ROBOT  │◄──────────┐
            │ 超时返回 │ (默认中枢)   │           │
            │         └──────┬──────┘           │
            │                │ 按键B循环         │
            │    ┌───────────┼───────────┐       │
            │    ▼           ▼           ▼       │
            │ ┌──────┐ ┌─────────┐ ┌────────┐   │
            │ │CLOCK │ │COUNTDOWN│ │  TODO  │   │
            │ │ 时钟 │ │  倒计时  │ │ 待办   │   │
            │ └──┬───┘ └────┬────┘ └───┬────┘   │
            │    │          │           │        │
            │    ▼          ▼           ▼        │
            │ ┌────────┐                ─────────┘
            │ │WEATHER │              按键B循环
            │ │  天气  │
            │ └────────┘
            │
            │       ┌──────────┐
            └───────│ SETTINGS │ (长按C进入, 按B退出)
                    └──────────┘

主循环: 100Hz 轮询按键 → 驱动状态机 → 更新 OLED → 检查 MQTT → 喂狗""", font_size=14, color=WHITE, y=1.8)
add_page_number(s, 12)

# ══════════════════════════════════════════
# P13: OLED 显示 & 表情系统
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "OLED 显示 & 机器宠表情系统")
add_accent_line(s)
add_card(s, 0.6, 1.8, 6.0, 4.5,
    "6 大核心模式",
    """🕐 CLOCK     — 全屏时间 + 日期 + 天气摘要
⏳ COUNTDOWN — 番茄钟专注/休息倒计时
🤖 CHAT-ROBOT— 空闲表情包 + 语音 + AI回复
📋 TODO      — 手机同步任务列表浏览
🌤 WEATHER   — 当前天气 + 预报详情
⚙ SETTINGS  — WiFi/亮度/音量/AI模式/设备信息

状态栏始终显示：模式图标 | 时间 | WiFi | 电量""")
add_card(s, 6.9, 1.8, 5.5, 4.5,
    "表情系统",
    """8 基础表情（6~10s 随机切换）：
  (◍•ᴗ•◍) 开心  (◍•ᴗ•◍)✧ 眨眼
  (๑•́ ₃ •̀๑) 撒娇  (๑•̀ㅂ•́)و✧ 期待
  (｡•ˇ‸ˇ•｡) 思考  (￣ｑ￣)ｚｚｚ 困倦
  ∑(O_O;) 惊讶    (´▽`ʃ♡ƪ) 爱心

8 事件表情（事件触发 3s）：
  新任务 → (☞ﾟヮﾟ)☞     任务完成 → ヽ(✿ﾟ▽ﾟ)ノ
  番茄钟到 → (๑˃̵ᴗ˂̵)و     低电量 → (´；ω；`)
  AI回复 → (◕‿◕✿)        充电 → (￣▽￣)~*⚡

动画：眨眼(3s周期)、呼吸浮动(±1px)、弹入(scale)""")
add_page_number(s, 13)

# ══════════════════════════════════════════
# P14: 语音全链路
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "语音交互全链路")
add_accent_line(s)
add_body(s, """┌───────────────── 语音输入 ─────────────────────────────────────────────┐
│  长按C键(0.5s) → OLED 显示"聆听中..." + 声波动画                           │
│  → INMP441 I2S 录音 (16kHz 16bit PCM, 最长5s, ~160KB buffer)              │
│  → HTTP POST PCM → 百度语音识别 ASR (免费5万次/天)                         │
│  → 返回识别文字 → 进入 AI 思考态                                            │
└──────────────────────────────────────────────────────────────────────────┘
                                    ↓
┌───────────────── AI 对话 ────────────────────────────────────────────────┐
│  文字 → HTTP POST → 智谱 GLM-4-Flash (云端) / Ollama qwen2.5 (本地)        │
│  → AI 返回回复文本 → OLED 显示 (E/F 键滚动翻页)                             │
└──────────────────────────────────────────────────────────────────────────┘
                                    ↓
┌───────────────── 语音输出 ───────────────────────────────────────────────┐
│  回复文字 → HTTP POST → 百度语音合成 TTS → 返回 MP3 音频                    │
│  → I2S 发送到 MAX98357A → 功放驱动喇叭播放                                  │
└──────────────────────────────────────────────────────────────────────────┘

全部 API 均免费额度，无需付费。全程延迟约 2~3 秒（云端模式）。""", font_size=14, color=WHITE, y=1.8)
add_page_number(s, 14)

# ══════════════════════════════════════════
# P15: AI 双模切换
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "AI 双模切换策略")
add_accent_line(s)
add_body(s, """                  ┌─────────────┐
                  │ 用户提问     │
                  └──────┬──────┘
                         │
                    ┌────▼────┐
                    │ 云端可用？│
                    └──┬───┬──┘
                  是   │   │   否
               ┌───────┘   └───────┐
               ▼                   ▼
        ┌────────────┐     ┌────────────┐
        │ ☁ 云端模式  │     │ 局域网有    │
        │ 智谱 GLM-4  │     │ Ollama？   │
        │ Flash (免费)│     └──┬─────┬───┘
        │ 延迟 1~3s   │   是   │     │ 否
        └────────────┘  ┌──────┘     └──────┐
                        ▼                   ▼
                 ┌────────────┐     ┌────────────┐
                 │ 🏠 本地模式 │     │ 📋 降级模式 │
                 │ Ollama qwen │     │ 预设回复    │
                 │ 0.5B (PC端) │     │ "AI暂时     │
                 │ 延迟 2~5s   │     │  不可用"    │
                 └────────────┘     └────────────┘

优势：答辩现场 WiFi 不稳时，一键切 PC 热点 + Ollama 本地，不受影响
状态栏实时显示当前 AI 模式图标 ☁ / 🏠""", font_size=14, color=WHITE, y=1.8)
add_page_number(s, 15)

# ══════════════════════════════════════════
# P16: 微信小程序
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "微信小程序设计")
add_accent_line(s)
add_body(s, """┌──────────────┬───────────────┬───────────────┬───────────────┬──────────┐
│  设备页       │  任务页        │  AI 对话页     │  番茄钟页      │  我的页   │
│  (首页)       │               │               │               │          │
├──────────────┼───────────────┼───────────────┼───────────────┼──────────┤
│ 设备状态卡片  │ 任务 CRUD     │ AI 文字对话   │ 远程启动/暂停  │ 设备绑定  │
│ • 在线/离线  │ • 创建/编辑    │ • 云端/本地   │ • 时长调节     │ WiFi配网 │
│ • 电量/WiFi  │ • 筛选/排序    │   模式切换     │ • 关联任务     │ MQTT状态 │
│ • 运行时长    │ • 标记完成    │ • 语音输入     │ • 双端同步     │ 消息推送  │
│              │ • 同步状态🟢  │ • 推送结果     │ • 完成通知     │ 关于     │
│ 快捷入口宫格  │               │   到设备       │ • 7日统计图    │          │
│ 6 大功能     │ 进度统计条     │ • 历史记录     │               │          │
│              │               │               │               │          │
├──────────────┴───────────────┴───────────────┴───────────────┴──────────┤
│ 通信方式: MQTT (EMQX Cloud 免费版)                                        │
│ 主题设计: device/{id}/task/add, task/done, pomodoro/start, message 等     │
│ 数据格式: JSON (见技术方案 §5.3)                                          │
└──────────────────────────────────────────────────────────────────────────┘""", font_size=13, color=WHITE, y=1.8)
add_page_number(s, 16)

# ══════════════════════════════════════════
# P17: 双端联动
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "双端联动 — 任务同步 & 番茄钟")
add_accent_line(s)
add_card(s, 0.6, 1.8, 5.8, 4.5,
    "任务同步流程",
    """小程序创建任务"复习模电第三章"
        │ MQTT: device/{id}/task/add
        ▼
ESP32 收到 → OLED 弹通知 "(☞ﾟヮﾟ)☞ 新任务！"
        │ 存入本地任务列表
        ▼
用户在 TODO 模式下 E/F 浏览任务
        │ 按 C 查看详情 + 一键开始番茄钟
        ▼
手机标记完成 → MQTT → OLED 显示 "ヽ(✿ﾟ▽ﾟ)ノ 已完成"
        │
        ▼
双端列表同步更新""")
add_card(s, 6.9, 1.8, 5.5, 4.5,
    "番茄钟双端同步",
    """小程序启动番茄钟 (25分钟)
        │ MQTT: device/{id}/pomodoro/start
        ▼
ESP32 OLED 同步启动倒计时
        │ 每秒 MQTT 上报剩余秒数
        ▼
小程序实时显示进度条 (双向!)
        │
    ┌───┴───┐
    ▼       ▼
 暂停/继续  放弃
    │       │
    └───┬───┘
        ▼
  倒计时 → 双端同时通知 + OLED闪烁
         ESP32 喇叭播放提示音
         OLED 完成表情 (๑˃̵ᴗ˂̵)و""")
add_page_number(s, 17)

# ══════════════════════════════════════════
# P18: 技术难点
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "关键技术难点与解决方案")
add_accent_line(s)
challenges = [
    ("ESP32-C3 无 DAC\n无法接模拟功放", "选型 MAX98357A I2S 数字功放\n纯数字输入，无需 MCLK，效率 92%\nI2S 总线与 INMP441 共享 BCLK/LRC", GREEN),
    ("GPIO 资源紧缺\n仅 11 个可用 IO", "按键用 74HC138D 3-8 译码器扩展\n4 脚 → 6 键（可扩至 8 键）\nI2C 仅 2 脚，I2S 共享总线 4 脚", ORANGE),
    ("ESP32-C3 RAM 有限\n~400KB SRAM", "模块化加载，按需初始化\n录音 buffer 限制 160KB (5s@16kHz)\n不用的功能释放 Heap", ACCENT),
    ("答辩 WiFi 不稳定\nAPI 可能超额/限流", "双模 AI：云端智谱 + 本地 Ollama\nPC 热点 + Ollama 一键切换\n预设降级回复兜底", RED),
    ("INMP441 供电\n禁止接 5V", "统一 3.3V 供电 (AMS1117 LDO)\nL/R 引脚必须接死 (GND=左声道)\n功放 VIN 走电池直供", GRAY),
]
for i, (problem, solution, color) in enumerate(challenges):
    y = 1.8 + i * 1.05
    add_card(s, 0.6, y, 3.5, 0.9, "", problem, title_color=color, body_color=WHITE)
    add_card(s, 4.3, y, 8.5, 0.9, "→ 解决方案", solution, title_color=GREEN, body_color=WHITE)
add_page_number(s, 18)

# ══════════════════════════════════════════
# P19: 演示流程
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "现场演示流程（预设脚本）")
add_accent_line(s)
add_body(s, """┌──────┬─────────────────────────────────────────────────────────────┐
│ 步骤 │ 演示内容                                                     │
├──────┼─────────────────────────────────────────────────────────────┤
│  1   │ 上电 → 开机动画 → 进入 CHAT-ROBOT 表情包（眨眼+呼吸浮动）      │
│  2   │ 按 B 键 → 循环切换 CLOCK/WEATHER/COUNTDOWN 各模式              │
│  3   │ 回到 CHAT-ROBOT → 长按 C → 显示"聆听中"+ 声波动画              │
│  4   │ 语音提问 "今天天气怎么样" → 松开 C → ASR 识别 → AI 思考动画    │
│  5   │ AI 返回回复 → OLED 显示 + TTS 语音播报                         │
│  6   │ 打开手机微信小程序 → 创建任务 "复习模电" → 发送到设备            │
│  7   │ ESP32 OLED 弹出通知动画 "(☞ﾟヮﾟ)☞ 新任务！"                    │
│  8   │ 切换到 TODO 模式 → E/F 浏览任务 → 手机标记完成 → OLED 同步更新  │
│  9   │ 手机启动 1 分钟番茄钟 → ESP32 同步倒计时 → 完成闪烁提示          │
│ 10   │ (备用) 切 AI 到本地 Ollama 模式，演示离线可用                    │
└──────┴─────────────────────────────────────────────────────────────┘

⏱ 总时长控制在 3 分钟以内，发言人操作设备，另一人持手机展示小程序端""", font_size=13, color=WHITE, y=1.8)
add_page_number(s, 19)

# ══════════════════════════════════════════
# P20: 团队分工
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "团队分工与协作")
add_accent_line(s)
add_body(s, """┌────────────┬──────────────────────────┬────────────────────────────────┐
│ 角色       │ 主要职责                  │ 关键交付物                      │
├────────────┼──────────────────────────┼────────────────────────────────┤
│ A 硬件&结构 │ BOM采购、电路焊接          │ 完整可用的硬件原型              │
│            │ 面包板搭建、硬件调试        │ 3D打印外壳 (待)                │
│            │ 74HC138 按键矩阵验证        │ 嘉立创原理图                   │
├────────────┼──────────────────────────┼────────────────────────────────┤
│ B 嵌入式    │ WiFi/OLED/时间/天气        │ ESP32 基础固件                  │
│   基础     │ 番茄钟/按键/状态机          │ 6 模式 OLED 显示+表情系统       │
│            │ PlatformIO 工程搭建         │ main.cpp + config.h            │
├────────────┼──────────────────────────┼────────────────────────────────┤
│ C 嵌入式    │ I2S 语音采集+播放          │ ESP32 高级固件                  │
│   高级+AI  │ 百度 ASR/TTS 对接          │ AI 双模切换                    │
│            │ 智谱 AI / Ollama 对接       │ MQTT Client                    │
│            │ MAX98357A 驱动              │                                │
├────────────┼──────────────────────────┼────────────────────────────────┤
│ D 小程序    │ 微信小程序开发              │ 5 Tab 页面小程序                │
│   +云端    │ MQTT 通信联调               │ 双端联动全流程可演示            │
│            │ Ollama 桥接服务             │ Cloudflare Worker 后端          │
│            │ Premiere Pro 动画制作       │ 4 组宠物动画视频                │
└────────────┴──────────────────────────┴────────────────────────────────┘

协作方式：Git 分支协作 + 每日站会 + MQTT 接口提前约定死""", font_size=12, color=WHITE, y=1.8)
add_page_number(s, 20)

# ══════════════════════════════════════════
# P21: 项目总结
# ══════════════════════════════════════════
s = new_slide()
add_title(s, "项目总结")
add_accent_line(s)
add_card(s, 0.6, 1.8, 5.8, 2.2,
    "✅ 已完成",
    "• ESP32-C3 固件：6 模式状态机 + 表情系统\n• OLED 128×64 全界面实现\n• I2S 语音全链路 (录音→ASR→AI→TTS→播放)\n• AI 双模切换 (智谱GLM + Ollama本地)\n• 微信小程序 5 Tab 页面\n• MQTT 任务同步 & 番茄钟远程\n• 74HC138D 按键矩阵 (4脚→6键)\n• MAX98357A I2S 数字功放\n• 电路原理图 (嘉立创) + BOM (< ￥80)")
add_card(s, 6.9, 1.8, 5.5, 2.2,
    "🔮 未来展望",
    "• 3D 打印外壳 + 成品组装\n• ESP32-S3 升级 (更多 GPIO + 更强 AI)\n• 增加环境传感器 (温湿度/光线)\n• 语音唤醒词 (免按键)\n• 多设备联动 (房间级智能)\n• 小程序上架微信审核")
add_card(s, 0.6, 4.3, 11.7, 2.2,
    "📊 技术栈总结",
    "硬件: ESP32-C3 · OLED SSD1306 · INMP441 · MAX98357A · 74HC138D · TP4056\n固件: Arduino/C++ · PlatformIO · I2S · I2C · WiFi · MQTT · JSON · NTP\nAI: 智谱 GLM-4-Flash · 百度 ASR + TTS · Ollama qwen2.5 · 和风天气\n软件: 微信小程序 (WXML+WXSS+JS) · MQTT.js · Flutter (App端) · Cloudflare Worker · Python Flask\n设计: Adobe Premiere Pro (动画) · 嘉立创EDA (原理图) · U8g2 (OLED UI)")
add_page_number(s, 21)

# ══════════════════════════════════════════
# P22: 致谢
# ══════════════════════════════════════════
s = new_slide()
add_bg(s, BG_DARK)
add_text_box(s, 0, 2.5, 13.33, 1.5, "感谢聆听", font_size=56, color=ACCENT, bold=True, align=PP_ALIGN.CENTER)
add_text_box(s, 0, 4.0, 13.33, 0.8, "桌面智能灵动岛 — Desktop Smart Island", font_size=24, color=GRAY, align=PP_ALIGN.CENTER)
add_accent_line(s, 4.6); s.shapes[-1].left = Inches(5.6); s.shapes[-1].width = Inches(2.0)
add_text_box(s, 0, 5.2, 13.33, 1.0, "欢迎提问 🙋\nGitHub / 小程序体验码", font_size=18, color=GRAY, align=PP_ALIGN.CENTER)

# ── 保存 ──
output_path = os.path.expanduser(r"C:\Users\ASUS\OneDrive\Desktop\desktopdog\桌面智能灵动岛_答辩PPT.pptx")
prs.save(output_path)
print(f"PPT 已生成: {output_path}")
print(f"共 {len(prs.slides)} 页")
