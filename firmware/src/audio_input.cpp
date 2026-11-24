#include "audio_input.h"

#include <Arduino.h>
#include <driver/i2s.h>

#include "config.h"

namespace audio_input {

namespace {

constexpr i2s_port_t kPort = I2S_NUM_1;
constexpr int kSampleRate = 16000;
constexpr int kN = crabfx::AudioAnalyzer::kFftSize;

portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
crabfx::AudioFeatures shared;
uint32_t last_ok_ms = 0;
bool pending_beat = false;  // latched until the LED loop reads it, so no beat is lost or doubled

crabfx::AudioAnalyzer analyzer(kSampleRate);
int32_t raw[kN];
float samples[kN];

void task(void*) {
  for (;;) {
    size_t bytes = 0;
    if (i2s_read(kPort, raw, sizeof(raw), &bytes, pdMS_TO_TICKS(100)) != ESP_OK || bytes != sizeof(raw)) continue;

    // INMP441 sends 24-bit samples left-aligned in 32-bit words.
    bool silent = true;
    for (int i = 0; i < kN; ++i) {
      samples[i] = (raw[i] >> 8) / 8388608.f;
      silent = silent && raw[i] == 0;
    }
    if (silent) continue;  // no microphone: the data line floats low

    const crabfx::AudioFeatures f = analyzer.process(samples, millis());
    portENTER_CRITICAL(&lock);
    shared = f;
    pending_beat = pending_beat || f.beat;
    last_ok_ms = millis();
    portEXIT_CRITICAL(&lock);
  }
}

}  // namespace

bool begin() {
  i2s_config_t cfg = {};
  cfg.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate = kSampleRate;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = 256;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = kMicSckPin;
  pins.ws_io_num = kMicWsPin;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = kMicSdPin;

  if (i2s_driver_install(kPort, &cfg, 0, nullptr) != ESP_OK) return false;
  if (i2s_set_pin(kPort, &pins) != ESP_OK) return false;
  xTaskCreatePinnedToCore(task, "audio", 4096, nullptr, 1, nullptr, 0);
  return true;
}

crabfx::AudioFeatures latest() {
  portENTER_CRITICAL(&lock);
  crabfx::AudioFeatures f = shared;
  f.beat = pending_beat;
  pending_beat = false;
  const uint32_t age = millis() - last_ok_ms;
  portEXIT_CRITICAL(&lock);
  if (age > 1000) f.valid = false;
  f.beat = f.valid && f.beat;
  return f;
}

}  // namespace audio_input
