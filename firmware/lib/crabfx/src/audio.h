// Audio features for the sound-reactive mode: FFT, band energies, beat detection.
// Hardware-independent: the firmware feeds it samples from an INMP441 I2S microphone.
#pragma once

#include <stdint.h>

namespace crabfx {

struct AudioFeatures {
  float level = 0.f;   // overall loudness, 0..1 after auto-gain
  float bass = 0.f;    // 40-250 Hz, 0..1
  float mid = 0.f;     // 250-2000 Hz, 0..1
  float treble = 0.f;  // 2-6 kHz, 0..1
  bool beat = false;   // a bass onset in this block
  bool valid = false;  // false when no microphone data is coming in
};

// In-place iterative radix-2 FFT; n must be a power of two.
void fft(float* re, float* im, int n);

// Tracks a decaying peak so a band reads 0..1 whatever the microphone gain.
class AutoGain {
 public:
  AutoGain(float floor, float decay) : floor_(floor), decay_(decay) {}
  float normalise(float v);

 private:
  float floor_, decay_, peak_ = 0.f;
};

// Flags an onset when energy jumps above its running average.
class BeatDetector {
 public:
  explicit BeatDetector(float ratio = 1.5f, float floor = 1e-4f, uint32_t refractory_ms = 250)
      : ratio_(ratio), floor_(floor), refractory_ms_(refractory_ms) {}
  bool update(float energy, uint32_t now_ms);

 private:
  float ratio_, floor_;
  uint32_t refractory_ms_;
  float avg_ = 0.f;
  bool primed_ = false;
  bool has_beat_ = false;
  uint32_t last_beat_ms_ = 0;
};

class AudioAnalyzer {
 public:
  static constexpr int kFftSize = 512;
  explicit AudioAnalyzer(float sample_rate_hz = 16000.f);

  // samples: kFftSize values in [-1, 1].
  AudioFeatures process(const float* samples, uint32_t now_ms);

 private:
  float bandEnergy(float lo_hz, float hi_hz) const;

  float sample_rate_;
  float re_[kFftSize], im_[kFftSize], window_[kFftSize];
  AutoGain level_gain_{0.01f, 0.995f}, bass_gain_{0.02f, 0.995f}, mid_gain_{0.01f, 0.995f}, treble_gain_{0.005f, 0.995f};
  BeatDetector beat_;
};

}  // namespace crabfx
