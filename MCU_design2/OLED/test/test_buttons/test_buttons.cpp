/**
 * test_buttons.ino — 按键硬件最小测试
 * 目的: 排除定时器/ISR/逻辑层干扰，直读 GPIO 确认硬件信号
 *
 * 接线 (per config.h):
 *   GPIO0 = HC138 A0
 *   GPIO1 = HC138 A1
 *   GPIO2 = HC138 A2
 *   GPIO3 = KEY_SENSE (HC138 输出端, 经 10k R6 上拉)
 *
 * HC138 真值: A2A1A0=000→Y0, 001→Y1, 010→Y2, ..., 101→Y5
 * 按键映射: Y0=B, Y1=F, Y2=E, Y3=C, Y4=D, Y5=A
 * 按下 = 低电平 (KEY_SENSE 读到 LOW)
 */

#include <Arduino.h>

const uint8_t ADDR_PINS[] = {0, 1, 2};
const uint8_t SENSE_PIN = 3;

// 扫描地址: A→Y5(101), B→Y0(000), C→Y3(011), D→Y4(100), E→Y2(010), F→Y1(001)
const uint8_t SCAN_ADDR[] = {0b101, 0b000, 0b011, 0b100, 0b010, 0b001};
const char* BTN_NAMES[] = {"A", "B", "C", "D", "E", "F"};

void setup() {
  // 等一下 USB CDC 就绪
  delay(2000);
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("=== Button Hardware Test ==="));
  Serial.println(F("Baseline: all buttons RELEASED"));

  // 初始化 GPIO
  for (int i = 0; i < 3; i++) {
    pinMode(ADDR_PINS[i], OUTPUT);
    digitalWrite(ADDR_PINS[i], HIGH);
  }
  pinMode(SENSE_PIN, INPUT_PULLUP);

  // 先测一次所有地址的基线（无按键按下）
  Serial.println(F("--- Baseline scan ---"));
  for (int btn = 0; btn < 6; btn++) {
    uint8_t addr = SCAN_ADDR[btn];
    digitalWrite(ADDR_PINS[0], addr & 0x01);
    digitalWrite(ADDR_PINS[1], (addr >> 1) & 0x01);
    digitalWrite(ADDR_PINS[2], (addr >> 2) & 0x01);
    delayMicroseconds(50);
    int val = digitalRead(SENSE_PIN);
    Serial.print(F("  "));
    Serial.print(BTN_NAMES[btn]);
    Serial.print(F(" (addr="));
    Serial.print(addr, BIN);
    Serial.print(F("): "));
    Serial.println(val == LOW ? F("LOW(!)") : F("HIGH"));
  }

  Serial.println();
  Serial.println(F("--- Now press and hold any button... ---"));
}

void loop() {
  static unsigned long lastPrint = 0;
  unsigned long now = millis();

  // 连续扫描（不用定时器）
  for (int btn = 0; btn < 6; btn++) {
    uint8_t addr = SCAN_ADDR[btn];
    digitalWrite(ADDR_PINS[0], addr & 0x01);
    digitalWrite(ADDR_PINS[1], (addr >> 1) & 0x01);
    digitalWrite(ADDR_PINS[2], (addr >> 2) & 0x01);
    delayMicroseconds(50);
    int val = digitalRead(SENSE_PIN);

    if (val == LOW) {
      Serial.print(F("[PRESS] "));
      Serial.print(BTN_NAMES[btn]);
      Serial.print(F(" @ "));
      Serial.println(now);
    }
  }

  // 每秒打印一次心跳
  if (now - lastPrint >= 1000) {
    lastPrint = now;
    Serial.print(F("."));  // 证明程序还在跑
  }
}
