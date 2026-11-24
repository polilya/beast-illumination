#include "audio.h"

#include <math.h>

namespace crabfx {

namespace {
constexpr float kPi = 3.14159265358979f;
}

void fft(float* re, float* im, int n) {
  // bit-reversal permutation
  for (int i = 1, j = 0; i < n; ++i) {
    int bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) {
      float t = re[i]; re[i] = re[j]; re[j] = t;
      t = im[i]; im[i] = im[j]; im[j] = t;
    }
  }
  for (int len = 2; len <= n; len <<= 1) {
    const float ang = -2.f * kPi / len;
    const float wr = cosf(ang), wi = sinf(ang);
    for (int i = 0; i < n; i += len) {
      float cr = 1.f, ci = 0.f;
      for (int k = 0; k < len / 2; ++k) {
        const int a = i + k, b = i + k + len / 2;
        const float xr = re[b] * cr - im[b] * ci;
        const float xi = re[b] * ci + im[b] * cr;
        re[b] = re[a] - xr; im[b] = im[a] - xi;
        re[a] += xr;        im[a] += xi;
        const float nr = cr * wr - ci * wi;
        ci = cr * wi + ci * wr;
        cr = nr;
      }
    }
  }
}

float AutoGain::normalise(float v) {
  peak_ = v > peak_ ? v : peak_ * decay_;
  const float ref = peak_ > floor_ ? peak_ : floor_;
  const float out = v / ref;
  return out > 1.f ? 1.f : out;
}

bool BeatDetector::update(float energy, uint32_t now_ms) {
  if (!primed_) {
    avg_ = energy;
    primed_ = true;
    return false;
  }
  const bool rested = !has_beat_ || now_ms - last_beat_ms_ >= refractory_ms_;
  const bool beat = rested && energy > floor_ && energy > avg_ * ratio_;
  avg_ += 0.05f * (energy - avg_);  // ~1 s memory at ~30 blocks/s
  if (beat) {
    has_beat_ = true;
    last_beat_ms_ = now_ms;
  }
  return beat;
}

AudioAnalyzer::AudioAnalyzer(float sample_rate_hz) : sample_rate_(sample_rate_hz) {
  for (int i = 0; i < kFftSize; ++i) window_[i] = 0.5f * (1.f - cosf(2.f * kPi * i / (kFftSize - 1)));  // Hann
}

float AudioAnalyzer::bandEnergy(float lo_hz, float hi_hz) const {
  const float bin_hz = sample_rate_ / kFftSize;
  int lo = static_cast<int>(lo_hz / bin_hz), hi = static_cast<int>(hi_hz / bin_hz);
  if (lo < 1) lo = 1;
  if (hi > kFftSize / 2) hi = kFftSize / 2;
  float sum = 0.f;
  for (int k = lo; k < hi; ++k) sum += sqrtf(re_[k] * re_[k] + im_[k] * im_[k]);
  return hi > lo ? sum / (hi - lo) / kFftSize : 0.f;
}

AudioFeatures AudioAnalyzer::process(const float* samples, uint32_t now_ms) {
  float sq = 0.f;
  for (int i = 0; i < kFftSize; ++i) {
    sq += samples[i] * samples[i];
    re_[i] = samples[i] * window_[i];
    im_[i] = 0.f;
  }
  fft(re_, im_, kFftSize);

  const float bass = bandEnergy(40.f, 250.f);
  AudioFeatures f;
  f.valid = true;
  f.level = level_gain_.normalise(sqrtf(sq / kFftSize));
  f.bass = bass_gain_.normalise(bass);
  f.mid = mid_gain_.normalise(bandEnergy(250.f, 2000.f));
  f.treble = treble_gain_.normalise(bandEnergy(2000.f, 6000.f));
  f.beat = beat_.update(bass, now_ms);
  return f;
}

}  // namespace crabfx
