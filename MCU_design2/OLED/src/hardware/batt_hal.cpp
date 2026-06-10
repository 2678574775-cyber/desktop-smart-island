/**
 * batt_hal.cpp — 电池电量检测实现
 * ESP32-C3 ADC1_CH0 (GPIO0), 12-bit, 11dB 衰减量程 ~3.9V
 */

#include "batt_hal.h"
#include <Arduino.h>

// ================================================================
//  静态变量
// ================================================================

static int  battMv      = 4200;    // 电池电压 (mV), 初始假设满电
static int  battPercent = 100;     // 电量百分比
static bool battInited  = false;

// 分压比: Vbat = V_adc * (R1+R2) / R2
static const float DIV_RATIO = (float)(BATT_R1 + BATT_R2) / (float)BATT_R2;

// 锂电池电压 → 电量映射 (3.3V=0%, 4.2V=100%)
static const int BATT_MV_MIN = 3300;
static const int BATT_MV_MAX = 4200;

// ADC 读取采样次数 (取平均抗噪)
static const int ADC_SAMPLES = 8;

// ================================================================
//  初始化
// ================================================================

void battInit() {
  if (battInited) return;

  pinMode(BATT_ADC_PIN, INPUT);
  analogSetAttenuation(ADC_11db);       // 11dB 衰减, 量程 ~3.9V
  battInited = true;

  Serial.printf("[BATT] ADC pin=GPIO%d, divider=%.1f:1\n", BATT_ADC_PIN, DIV_RATIO);
}

// ================================================================
//  更新电池状态
// ================================================================

void updateBattery() {
  if (!battInited) battInit();

  // 多次采样取平均
  long sum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += analogReadMilliVolts(BATT_ADC_PIN);
    delay(2);
  }
  int adcMv = sum / ADC_SAMPLES;

  // 计算实际电池电压
  battMv = (int)(adcMv * DIV_RATIO);

  // 计算百分比
  battPercent = constrain(
    map(battMv, BATT_MV_MIN, BATT_MV_MAX, 0, 100),
    0, 100
  );

  // 更新全局电池状态
  if (battPercent >= 80) {
    currentBatt = BATT_FULL;
  } else if (battPercent >= 30) {
    currentBatt = BATT_MID;
  } else {
    currentBatt = BATT_LOW;
  }

#if BTN_DEBUG
  Serial.printf("[BATT] adc=%dmV bat=%dmV %d%% state=%d\n",
                adcMv, battMv, battPercent, currentBatt);
#endif
}

// ================================================================
//  Getter
// ================================================================

int getBatteryVoltage() {
  return battMv;
}

int getBatteryPercent() {
  return battPercent;
}
