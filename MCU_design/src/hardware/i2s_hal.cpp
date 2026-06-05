/**
 * i2s_hal.cpp — I2S 总线硬件抽象层实现
 */

#include "i2s_hal.h"

// ---- RX 录音 (16kHz, INMP441) ----
void i2sInitRecord() {
  i2s_driver_uninstall(I2S_NUM_0);

  i2s_config_t cfg = {};
  cfg.mode            = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate     = SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  cfg.channel_format  = I2S_CHANNEL_FMT_ONLY_RIGHT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count   = 8;
  cfg.dma_buf_len     = 512;
  cfg.use_apll        = false;
  cfg.tx_desc_auto_clear = false;
  cfg.fixed_mclk      = 0;

  i2s_pin_config_t p = {};
  p.bck_io_num   = I2S_BCLK;
  p.ws_io_num    = I2S_LRC;
  p.data_out_num = I2S_PIN_NO_CHANGE;
  p.data_in_num  = I2S_DOUT;

  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &p);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

// ---- TX 播放 (默认 16kHz) ----
void i2sInitPlay() {
  i2sInitPlayAt(SAMPLE_RATE);
}

// ---- TX 播放 (可指定采样率) ----
void i2sInitPlayAt(int sampleRate) {
  i2s_driver_uninstall(I2S_NUM_0);

  i2s_config_t cfg = {};
  cfg.mode            = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate     = sampleRate;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format  = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count   = 8;
  cfg.dma_buf_len     = 512;
  cfg.use_apll        = false;
  cfg.tx_desc_auto_clear = true;
  cfg.fixed_mclk      = 0;

  i2s_pin_config_t p = {};
  p.bck_io_num   = I2S_BCLK;
  p.ws_io_num    = I2S_LRC;
  p.data_out_num = I2S_DIN;
  p.data_in_num  = I2S_PIN_NO_CHANGE;

  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &p);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

void i2sDeinit() {
  i2s_driver_uninstall(I2S_NUM_0);
}
