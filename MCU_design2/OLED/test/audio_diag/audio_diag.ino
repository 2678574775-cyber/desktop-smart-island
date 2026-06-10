/**
 * audio_diag.ino — INMP441 麦克风 + MAX98357A 功放 硬件诊断
 *
 * 接线 (按你的 README):
 *   GPIO5  = BCLK (MAX98357A + INMP441 共用)
 *   GPIO6  = LRC  (MAX98357A + INMP441 共用)
 *   GPIO7  = DIN  → MAX98357A (ESP32 发数据给功放)
 *   GPIO10 = DOUT ← INMP441 SD (麦克风发数据给 ESP32)
 *   GPIO20 = AMP_SD → MAX98357A (功放开关)
 *
 * 串口命令 (115200 bps):
 *   r  — 录音 3 秒, 打印采样统计到串口
 *   p  — 播放 440Hz 正弦波 1 秒
 *   s  — 停止当前操作
 */

#include <Arduino.h>
#include <driver/i2s.h>

// ---- 引脚 (和你的 README 一致) ----
#define I2S_BCLK   5
#define I2S_LRC    6
#define I2S_DIN    7
#define I2S_DOUT  10
#define AMP_SD    20

#define SAMPLE_RATE    16000
#define RECORD_SEC     3
#define RECORD_SAMPLES (SAMPLE_RATE * RECORD_SEC)
#define TONE_HZ        440.0f
#define TONE_DUR_MS    1000

// ---- 录音诊断 ----
void testRecord() {
  Serial.println(F("\n=== MIC RECORD TEST ==="));
  Serial.println(F("Initializing I2S RX mode..."));

  // 关闭功放 (防止啸叫)
  digitalWrite(AMP_SD, LOW);
  delay(10);

  i2s_driver_uninstall(I2S_NUM_0);

  i2s_config_t cfg = {};
  cfg.mode                 = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate          = SAMPLE_RATE;
  cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT;
  cfg.channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count        = 8;
  cfg.dma_buf_len          = 512;
  cfg.use_apll             = false;
  cfg.tx_desc_auto_clear   = false;
  cfg.fixed_mclk           = 0;

  i2s_pin_config_t pin_cfg = {};
  pin_cfg.bck_io_num     = I2S_BCLK;
  pin_cfg.ws_io_num      = I2S_LRC;
  pin_cfg.data_out_num   = I2S_PIN_NO_CHANGE;
  pin_cfg.data_in_num    = I2S_DOUT;

  esp_err_t err = i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("ERROR: i2s_driver_install failed: %d\n", err);
    return;
  }
  err = i2s_set_pin(I2S_NUM_0, &pin_cfg);
  if (err != ESP_OK) {
    Serial.printf("ERROR: i2s_set_pin failed: %d\n", err);
    return;
  }
  // 清空 DMA 缓冲区，丢弃初始化产生的随机数据
  i2s_zero_dma_buffer(I2S_NUM_0);

  Serial.println(F("I2S RX started. Recording 3 seconds..."));
  Serial.println(F("Speak into the mic now! (or tap it to see signal)"));

  int32_t minVal = 0x7FFFFFFF;
  int32_t maxVal = 0x80000000;
  long    sumAbs = 0;
  int     samplesRead = 0;

  unsigned long start = millis();
  while (millis() - start < RECORD_SEC * 1000UL) {
    size_t bytesRead = 0;
    int32_t buf[128];
    esp_err_t r = i2s_read(I2S_NUM_0, buf, sizeof(buf), &bytesRead, 100);
    if (r == ESP_OK && bytesRead > 0) {
      int n = bytesRead / 4;  // 32bit samples
      for (int i = 0; i < n; i++) {
        int32_t s = buf[i];
        if (s < minVal) minVal = s;
        if (s > maxVal) maxVal = s;
        sumAbs += abs(s);
        samplesRead++;
      }
    }
  }

  Serial.printf("\n=== Record Result (%d samples) ===\n", samplesRead);
  Serial.printf("  min  : %d\n", minVal);
  Serial.printf("  max  : %d\n", maxVal);
  Serial.printf("  avgAbs: %ld\n", sumAbs / max(samplesRead, 1));
  Serial.printf("  range : %d\n", maxVal - minVal);

  // 诊断结论
  int range = maxVal - minVal;
  if (range < 1000) {
    Serial.println(F("\n[DIAG] Signal range < 1000 — possible issues:"));
    Serial.println(F("  1. INMP441 not powered (check VDD→3.3V, GND→GND)"));
    Serial.println(F("  2. L/R pin not connected (must be GND or VCC, not floating)"));
    Serial.println(F("  3. Wrong channel format (trying CHANNEL_FMT_ONLY_LEFT vs RIGHT)"));
  } else if (range < 10000) {
    Serial.println(F("\n[DIAG] Weak signal. Mic might work but gain is low. Try speaking closer."));
  } else {
    Serial.println(F("\n[DIAG] Signal looks OK! Microphone is working."));
  }

  i2s_driver_uninstall(I2S_NUM_0);
  Serial.println(F("I2S closed."));
}

// ---- 播放诊断: 生成 440Hz 正弦波 ----
void testPlay() {
  Serial.println(F("\n=== SPEAKER PLAY TEST ==="));
  Serial.println(F("Generating 440Hz sine wave, 1 second..."));

  // 打开功放
  digitalWrite(AMP_SD, HIGH);
  delay(10);

  i2s_driver_uninstall(I2S_NUM_0);

  i2s_config_t cfg = {};
  cfg.mode                 = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate          = SAMPLE_RATE;
  cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count        = 8;
  cfg.dma_buf_len          = 512;
  cfg.use_apll             = false;
  cfg.tx_desc_auto_clear   = true;
  cfg.fixed_mclk           = 0;

  i2s_pin_config_t pin_cfg = {};
  pin_cfg.bck_io_num     = I2S_BCLK;
  pin_cfg.ws_io_num      = I2S_LRC;
  pin_cfg.data_out_num   = I2S_DIN;
  pin_cfg.data_in_num    = I2S_PIN_NO_CHANGE;

  esp_err_t err = i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("ERROR: i2s_driver_install failed: %d\n", err);
    digitalWrite(AMP_SD, LOW);
    return;
  }
  err = i2s_set_pin(I2S_NUM_0, &pin_cfg);
  if (err != ESP_OK) {
    Serial.printf("ERROR: i2s_set_pin failed: %d\n", err);
    digitalWrite(AMP_SD, LOW);
    return;
  }

  // 生成正弦波并播放
  const int totalSamples = SAMPLE_RATE * TONE_DUR_MS / 1000;
  const float amplitude = 8000.0f;  // 16bit 峰值的一半左右 (避免破音)
  const float omega = 2.0f * PI * TONE_HZ / SAMPLE_RATE;

  Serial.println(F("Playing... (you should hear a beep tone)"));
  unsigned long playStart = millis();

  for (int i = 0; i < totalSamples; i += 512) {
    int16_t buf[512];
    int chunk = min(512, totalSamples - i);
    for (int j = 0; j < chunk; j++) {
      buf[j] = (int16_t)(amplitude * sinf(omega * (i + j)));
    }
    size_t written = 0;
    i2s_write(I2S_NUM_0, buf, chunk * sizeof(int16_t), &written, portMAX_DELAY);
    if (written == 0) {
      Serial.println(F("ERROR: i2s_write returned 0 bytes!"));
      break;
    }
  }

  // 等待 DMA 发送完毕
  delay(50);

  Serial.printf("Done. Played %d samples in %lu ms.\n", totalSamples, millis() - playStart);
  Serial.println(F("If you heard a tone: MAX98357A + speaker are working."));
  Serial.println(F("If no sound: check VIN(3.7V), GAIN→GND(12dB), SD→GPIO20, speaker wiring."));

  i2s_driver_uninstall(I2S_NUM_0);
  digitalWrite(AMP_SD, LOW);  // 关功放省电
  Serial.println(F("I2S closed, amp off."));
}

// ---- 处理串口命令 ----
void handleSerial() {
  if (Serial.available() == 0) return;
  char cmd = Serial.read();
  // 清掉换行
  while (Serial.available() > 0) { Serial.read(); }

  switch (cmd) {
    case 'r':
    case 'R':
      testRecord();
      break;
    case 'p':
    case 'P':
      testPlay();
      break;
    default:
      Serial.println(F("Unknown cmd. Use: r=record test, p=play test"));
      break;
  }

  Serial.println(F("\nReady. Send 'r' or 'p':"));
}

// ---- setup & loop ----
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("\n\n===== Audio Diagnostic Tool ====="));
  Serial.println(F("INMP441 Mic + MAX98357A Amp Test"));
  Serial.println();

  // 初始化功放控制引脚
  pinMode(AMP_SD, OUTPUT);
  digitalWrite(AMP_SD, LOW);  // 默认关功放

  // 引脚打印确认
  Serial.println(F("Pin mapping:"));
  Serial.printf("  BCLK  → GPIO%d\n", I2S_BCLK);
  Serial.printf("  LRC   → GPIO%d\n", I2S_LRC);
  Serial.printf("  DIN   → GPIO%d (ESP32 → MAX98357A)\n", I2S_DIN);
  Serial.printf("  DOUT  → GPIO%d (INMP441 → ESP32)\n", I2S_DOUT);
  Serial.printf("  AMP_SD→ GPIO%d\n", AMP_SD);
  Serial.println();

  // 列出诊断要点
  Serial.println(F("Pre-flight checklist:"));
  Serial.println(F("  1. INMP441 VDD→3.3V, GND→GND"));
  Serial.println(F("  2. INMP441 L/R→GND (left channel)"));
  Serial.println(F("  3. MAX98357A VIN→3.7V battery, GND→GND"));
  Serial.println(F("  4. MAX98357A GAIN→GND (12dB)"));
  Serial.println(F("  5. MAX98357A SD→GPIO20 (amp enable)"));
  Serial.println(F("  6. Speaker wires connected to MAX98357A +/-"));
  Serial.println();

  Serial.println(F("Commands:"));
  Serial.println(F("  r = Record 3s, print mic signal stats"));
  Serial.println(F("  p = Play 440Hz tone 1s, test speaker"));
  Serial.println(F("\nReady. Send 'r' or 'p':"));
}

void loop() {
  handleSerial();
  delay(10);
}
