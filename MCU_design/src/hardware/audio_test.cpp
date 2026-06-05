/**
 * audio_test.cpp — 音频硬件诊断实现 (OLED 显示版)
 * 无需串口, 在 OLED 128×64 屏幕上直接查看麦克风/功放测试结果
 */

#include "audio_test.h"
#include "display_hal.h"

// ---- 初始化 ----
void audioTestInit() {
  pinMode(AMP_SD_PIN, OUTPUT);
  digitalWrite(AMP_SD_PIN, LOW);
}

// ---- 麦克风录音诊断 (OLED 界面) ----
void audioTestMic() {
  display.clearDisplay();

  // ── 标题栏 ──
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("MIC TEST"));
  display.drawFastHLine(0, 9, SCREEN_W, SSD1306_WHITE);

  // ── 初始化 I2S RX ──
  digitalWrite(AMP_SD_PIN, LOW);
  i2s_driver_uninstall(I2S_NUM_0);

  // INMP441 实际输出: MSB-justified, 数据出现在右声道 (即使 L/R 接地)
  i2s_config_t cfg = {};
  cfg.mode            = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate     = 16000;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  cfg.channel_format  = I2S_CHANNEL_FMT_ONLY_RIGHT;   // INMP441 数据在右声道!
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;  // I2S 标准格式
  cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count   = 4;
  cfg.dma_buf_len     = 256;
  cfg.use_apll        = false;
  cfg.tx_desc_auto_clear = false;
  cfg.fixed_mclk      = 0;

  i2s_pin_config_t pin = {};
  pin.bck_io_num   = I2S_BCLK;
  pin.ws_io_num    = I2S_LRC;
  pin.data_out_num = I2S_PIN_NO_CHANGE;
  pin.data_in_num  = I2S_DOUT;

  if (i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL) != ESP_OK) { goto fail; }
  if (i2s_set_pin(I2S_NUM_0, &pin) != ESP_OK)                 { goto fail; }
  i2s_zero_dma_buffer(I2S_NUM_0);

  // ── 实时录音 + VU 表 (10 秒) ──
  #define MIC_TEST_SEC  10
  {
    int32_t liveMin = 0x7FFFFFFF, liveMax = 0x80000000;
    int     samplesRead = 0;
    int     readsFailed = 0;
    int     peak = 0;                  // 当前帧峰值 (用于 VU 表)
    unsigned long t0 = millis();

    while (millis() - t0 < MIC_TEST_SEC * 1000UL) {
      size_t bytesRead = 0;
      int32_t raw[128];
      esp_err_t err = i2s_read(I2S_NUM_0, raw, sizeof(raw), &bytesRead, 50);
      if (err == ESP_OK && bytesRead > 0) {
        int n = bytesRead / 4;
        int framePeak = 0;
        for (int i = 0; i < n; i++) {
          int32_t s = raw[i] >> 8;     // INMP441 24bit → 16bit
          int absVal = abs(s);
          if (absVal > framePeak) framePeak = absVal;
          if (s < liveMin) liveMin = s;
          if (s > liveMax) liveMax = s;
          samplesRead++;
        }
        // 峰值平滑: 快速上升, 慢速衰减
        if (framePeak > peak) peak = framePeak;
        else peak = peak * 7 / 8;      // 衰减 12.5%
      } else {
        readsFailed++;
      }

      // 每 50ms 刷新一次界面 (smoother VU)
      static unsigned long lastDraw = 0;
      if (millis() - lastDraw >= 50) {
        lastDraw = millis();

        int elapsed  = (millis() - t0) / 1000;
        int progress = map(elapsed, 0, MIC_TEST_SEC, 0, 120);

        display.fillRect(0, 10, SCREEN_W, 54, SSD1306_BLACK);

        // ── 上半: 大 VU 柱 (40px 宽区域) ──
        int vuY = 12;
        int vuH = 16;
        // 映射 peak 到 0~120px (peakMax ≈ 32767, 映射 0~30000)
        int barW = constrain(map(peak, 0, 28000, 0, 120), 0, 120);

        display.drawRect(3, vuY, 122, vuH, SSD1306_WHITE);
        if (barW > 0) {
          display.fillRect(4, vuY + 1, barW, vuH - 2, SSD1306_WHITE);
        }
        // 峰值读数
        display.setCursor(4, vuY + vuH + 3);
        display.print(F("Peak:")); display.print(peak);

        // ── 下半: 进度条 (底部) ──
        int progY = 45;
        int progH = 4;
        display.drawRect(3, progY, 122, progH, SSD1306_WHITE);
        if (progress > 0) {
          display.fillRect(4, progY + 1, progress, progH - 2, SSD1306_WHITE);
        }
        // 倒计时文字
        display.setCursor(78, progY + 5);
        display.print(elapsed); display.print(F("/")); display.print(MIC_TEST_SEC); display.print('s');

        // ── 中间: 实时数值 ──
        int range = liveMax - liveMin;
        display.setCursor(0, 36);
        display.print(F("Rng:")); display.print(range);
        if (readsFailed > 0) {
          display.print(F(" err")); display.print(readsFailed);
        }

        display.display();
      }
    }

    // ── 录音结束, 显示统计 ──
    i2s_driver_uninstall(I2S_NUM_0);
    display.fillRect(0, 10, SCREEN_W, 54, SSD1306_BLACK);

    int range = liveMax - liveMin;

    if (samplesRead == 0 || readsFailed > 50) {
      // i2s_read 根本没读到数据
      display.setCursor(0, 10);
      display.print(F("NO DATA!"));
      display.setCursor(0, 22);
      display.print(F("i2s_read failed x"));
      display.print(readsFailed);
      display.setCursor(0, 34);
      display.print(F("Chk: VDD/SCK/WS/SD"));
      display.setCursor(0, 46);
      display.print(F("L/R must be GND/VCC"));
    } else {
      display.setCursor(0, 10);
      display.print(F("Samples:")); display.print(samplesRead);
      display.setCursor(0, 20);
      display.print(F("Min:")); display.print(liveMin);
      display.print(F(" Max:")); display.print(liveMax);
      display.setCursor(0, 30);
      display.print(F("Range:")); display.println(range);

      // 诊断结论
      display.setCursor(0, 50);
      if (range < 1000) {
        display.print(F("FAIL: no signal"));
      } else if (range < 8000) {
        display.print(F("WEAK: speak closer"));
      } else {
        display.print(F("OK! Mic working"));
      }
    }

    display.display();
    return;
  }

fail:
  i2s_driver_uninstall(I2S_NUM_0);
  display.fillRect(0, 10, SCREEN_W, 54, SSD1306_BLACK);
  display.setCursor(0, 20);
  display.print(F("I2S ERROR!"));
  display.setCursor(0, 32);
  display.print(F("Check wiring"));
  display.display();
}

// ---- 功放播放诊断 (OLED 界面) ----
void audioTestSpk() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("SPEAKER TEST"));
  display.drawFastHLine(0, 9, SCREEN_W, SSD1306_WHITE);

  // ── 初始化 I2S TX ──
  digitalWrite(AMP_SD_PIN, HIGH);  // 打开功放
  delay(10);

  i2s_driver_uninstall(I2S_NUM_0);

  i2s_config_t cfg = {};
  cfg.mode            = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate     = 16000;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format  = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count   = 8;
  cfg.dma_buf_len     = 512;
  cfg.use_apll        = false;
  cfg.tx_desc_auto_clear = true;
  cfg.fixed_mclk      = 0;

  i2s_pin_config_t pin = {};
  pin.bck_io_num   = I2S_BCLK;
  pin.ws_io_num    = I2S_LRC;
  pin.data_out_num = I2S_DIN;
  pin.data_in_num  = I2S_PIN_NO_CHANGE;

  if (i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL) != ESP_OK) { goto fail2; }
  if (i2s_set_pin(I2S_NUM_0, &pin) != ESP_OK)                 { goto fail2; }

  // ── 播放 440Hz 正弦波 ──
  {
    const int total = 32000;  // 2 秒 @ 16kHz
    const float amp = 6000.0f;
    const float w   = 2.0f * PI * 440.0f / 16000.0f;

    display.fillRect(0, 10, SCREEN_W, 54, SSD1306_BLACK);
    display.setCursor(0, 25);
    display.print(F("Playing 440Hz..."));
    display.setCursor(0, 40);
    display.print(F("Listen for beep"));
    display.display();

    for (int i = 0; i < total; i += 256) {
      int16_t buf[256];
      int chunk = min(256, total - i);
      for (int j = 0; j < chunk; j++) {
        buf[j] = (int16_t)(amp * sinf(w * (i + j)));
      }
      size_t wr = 0;
      i2s_write(I2S_NUM_0, buf, chunk * 2, &wr, portMAX_DELAY);
    }
    delay(50);  // 等 DMA 发完
  }

  i2s_driver_uninstall(I2S_NUM_0);
  digitalWrite(AMP_SD_PIN, LOW);

  display.fillRect(0, 10, SCREEN_W, 54, SSD1306_BLACK);
  display.setCursor(0, 20);
  display.print(F("Playback done!"));
  display.setCursor(0, 35);
  display.print(F("If you heard beep:"));
  display.setCursor(0, 47);
  display.print(F("MAX98357A OK!"));
  display.display();
  return;

fail2:
  i2s_driver_uninstall(I2S_NUM_0);
  digitalWrite(AMP_SD_PIN, LOW);
  display.fillRect(0, 10, SCREEN_W, 54, SSD1306_BLACK);
  display.setCursor(0, 20);
  display.print(F("I2S TX ERROR!"));
  display.setCursor(0, 32);
  display.print(F("Check DIN wiring"));
  display.display();
}
