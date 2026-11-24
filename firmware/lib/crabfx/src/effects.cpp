#include "effects.h"

#include <math.h>

namespace crabfx {

namespace {

constexpr float kPi = 3.14159265358979f;

RGB wheel(uint8_t h) {  // fully saturated hue, 0..255 around the colour circle
  const uint8_t region = h / 43, rem = (h - region * 43) * 6;
  const uint8_t up = rem, down = 255 - rem;
  switch (region) {
    case 0: return RGB(255, up, 0);
    case 1: return RGB(down, 255, 0);
    case 2: return RGB(0, 255, up);
    case 3: return RGB(0, down, 255);
    case 4: return RGB(up, 0, 255);
    default: return RGB(255, 0, down);
  }
}

void setIfOnLeg(RGB* leg, int i, RGB c) {
  if (i >= 0 && i < kPixelsPerLeg) leg[i] = c;
}

}  // namespace

int Stepper::due(uint32_t now_ms, float speed) {
  acc_ms_ += (now_ms - last_ms_) * speed;
  last_ms_ = now_ms;
  int n = static_cast<int>(acc_ms_ / period_ms_);
  acc_ms_ -= n * period_ms_;
  if (n > 4) {
    n = 4;
    acc_ms_ = 0.f;
  }
  return n;
}

void Effect::begin(Frame& f, Context&) { f.fill(RGB()); }

// ---- Fire waves -------------------------------------------------------------

static const RGB kFireLevels[] = {
    {0, 0, 0},      // no fire
    {50, 0, 0},     // embers
    {150, 30, 0},   // catching
    {255, 80, 0},   // peak
    {255, 150, 0},  // flaring out
};

void FireWaves::begin(Frame& f, Context& c) {
  f.fill(RGB());
  for (int& n : count_) n = 0;
  step_.reset(c.now_ms);
}

void FireWaves::render(Frame& f, Context& c) {
  for (int n = step_.due(c.now_ms, c.speed); n > 0; --n) {
    for (int l = 0; l < kLegs; ++l) {
      if (count_[l] < kMaxWaves && c.rng.chance(0.82f))
        waves_[l][count_[l]++] = {c.rng.range(0, kPixelsPerLeg - 1), 0, c.rng.range(6, 18), 4};
      int kept = 0;
      for (int w = 0; w < count_[l]; ++w) {
        Wave& wave = waves_[l][w];
        wave.radius++;
        if (wave.intensity > 1) wave.intensity--;
        if (wave.radius <= wave.max_radius) waves_[l][kept++] = wave;
      }
      count_[l] = kept;
    }
  }
  for (int l = 0; l < kLegs; ++l) {
    int heat[kPixelsPerLeg] = {};
    for (int w = 0; w < count_[l]; ++w) {
      const Wave& wave = waves_[l][w];
      for (int i = 0; i < kPixelsPerLeg; ++i) {
        const int d = i > wave.center ? i - wave.center : wave.center - i;
        if (d <= wave.radius && wave.intensity - d / 2 > heat[i]) heat[i] = wave.intensity - d / 2;
      }
    }
    for (int i = 0; i < kPixelsPerLeg; ++i) f.px[l][i] = kFireLevels[heat[i] > 4 ? 4 : heat[i]];
  }
}

// ---- Fire sparks ------------------------------------------------------------

static const RGB kSparkColours[] = {{255, 50, 0}, {255, 100, 0}, {255, 150, 50}, {255, 200, 100}};

void FireSparks::begin(Frame& f, Context& c) {
  f.fill(RGB());
  count_ = 0;
  step_.reset(c.now_ms);
}

void FireSparks::render(Frame& f, Context& c) {
  for (int n = step_.due(c.now_ms, c.speed); n > 0; --n) {
    for (auto& leg : f.px)
      for (auto& p : leg)
        if (c.rng.chance(0.02f)) p = kSparkColours[c.rng.range(0, 3)];

    if (count_ < kMaxWaves && c.rng.chance(0.005f)) {
      const Segment& s = kSegments[c.rng.range(0, kSegmentCount - 1)];
      waves_[count_++] = {c.rng.range(0, kLegs - 1), s.first + c.rng.range(0, s.count - 1), 0};
    }
    // The wave front sits at every pixel whose distance from the start has the
    // same parity as the step, as in the original neighbour-set expansion.
    int kept = 0;
    for (int w = 0; w < count_; ++w) {
      Wave& wave = waves_[w];
      const RGB colour = kSparkColours[wave.step < 3 ? wave.step : 3];
      for (int d = wave.step; d >= 0; d -= 2) {
        setIfOnLeg(f.px[wave.leg], wave.start - d, colour);
        setIfOnLeg(f.px[wave.leg], wave.start + d, colour);
      }
      if (++wave.step < 4) waves_[kept++] = wave;
    }
    count_ = kept;
  }
}

// ---- Bioluminescence --------------------------------------------------------

void Bioluminescence::begin(Frame& f, Context& c) {
  for (auto& leg : level_)
    for (float& v : leg) v = 0.f;
  f.fill(RGB(0, 30, 50));
  step_.reset(c.now_ms);
}

void Bioluminescence::render(Frame& f, Context& c) {
  for (int n = step_.due(c.now_ms, c.speed); n > 0; --n)
    for (int l = 0; l < kLegs; ++l)
      for (int i = 0; i < kPixelsPerLeg; ++i) {
        float& v = level_[l][i];
        if (c.rng.chance(0.08f)) v = 1.f;
        v = v > 0.05f ? v - 0.05f : 0.f;
        f.px[l][i] = RGB(0, static_cast<uint8_t>(30 + 225 * v), static_cast<uint8_t>(50 + 205 * v));
      }
}

// ---- Breathing --------------------------------------------------------------

void BreathingSpasm::begin(Frame& f, Context& c) {
  t_s_ = 0.f;
  f.fill(RGB(0, 50, 100));
  step_.reset(c.now_ms);
}

void BreathingSpasm::render(Frame& f, Context& c) {
  constexpr float kBreathPeriodS = 6.f;
  for (int n = step_.due(c.now_ms, c.speed); n > 0; --n) {
    t_s_ += 0.05f;
    const float breath = (sinf(2.f * kPi * t_s_ / kBreathPeriodS) + 1.f) / 2.f;
    const RGB colour = lerp(RGB(0, 50, 100), RGB(0, 255, 255), breath);
    for (int l = 0; l < kLegs; ++l) {
      for (auto& p : f.px[l]) p = c.rng.chance(0.02f) ? RGB(255, 255, 255) : colour;  // neuron flicker
      if (c.rng.chance(0.005f)) f.fillLeg(l, RGB(255, 0, 0));                         // spasm
    }
  }
}

// ---- Disco ------------------------------------------------------------------

void Disco::render(Frame& f, Context& c) {
  for (int n = step_.due(c.now_ms, c.speed); n > 0; --n)
    for (auto& leg : f.px)
      for (auto& p : leg)
        p = RGB(c.rng.range(50, 150), c.rng.range(50, 150), c.rng.range(50, 150));
}

// ---- Resonance --------------------------------------------------------------

static constexpr RGB kResonanceBase(10, 0, 30), kResonanceGlow(0, 100, 255);

void Resonance::shuffle(Rng& rng) {
  for (int i = 0; i < kLegs; ++i) order_[i] = i;
  for (int i = kLegs - 1; i > 0; --i) {
    const int j = rng.range(0, i);
    const int t = order_[i];
    order_[i] = order_[j];
    order_[j] = t;
  }
}

void Resonance::begin(Frame& f, Context& c) {
  f.fill(kResonanceBase);
  shuffle(c.rng);
  index_ = phase_ = 0;
  step_.reset(c.now_ms);
}

void Resonance::render(Frame& f, Context& c) {
  constexpr int kSteps = 20;  // up 0..19, down 20..0
  for (int n = step_.due(c.now_ms, c.speed); n > 0; --n) {
    const int level = phase_ < kSteps ? phase_ : 2 * kSteps - phase_;
    f.fillLeg(order_[index_], lerp(kResonanceBase, kResonanceGlow, static_cast<float>(level) / kSteps));
    if (++phase_ > 2 * kSteps) {
      phase_ = 0;
      if (++index_ == kLegs) {
        shuffle(c.rng);
        index_ = 0;
      }
    }
  }
}

// ---- Heartbeat --------------------------------------------------------------

static const float kBeatScales[] = {0.6f, 0.6f, 1.2f};  // two quick beats, one slow
constexpr float kRiseS = 0.21f, kFallS = 0.42f, kGapS = 0.08f, kRestS = 0.4f;

float Heartbeat::cycleSeconds() {
  float total = kRestS;
  for (float k : kBeatScales) total += k * (kRiseS + kFallS + kGapS);
  return total;
}

float Heartbeat::envelope(float t_s) {
  float t = fmodf(t_s, cycleSeconds());
  for (float k : kBeatScales) {
    if (t < k * kRiseS) return t / (k * kRiseS);
    t -= k * kRiseS;
    if (t < k * kFallS) return 1.f - t / (k * kFallS);
    t -= k * kFallS;
    if (t < k * kGapS) return 0.f;
    t -= k * kGapS;
  }
  return 0.f;
}

void Heartbeat::begin(Frame& f, Context& c) {
  t_s_ = 0.f;
  last_ms_ = c.now_ms;
  f.fill(RGB(50, 0, 0));
}

void Heartbeat::render(Frame& f, Context& c) {
  t_s_ += (c.now_ms - last_ms_) * c.speed / 1000.f;
  last_ms_ = c.now_ms;
  f.fill(lerp(RGB(50, 0, 0), RGB(255, 50, 0), envelope(t_s_)));
}

// ---- Muscle wave ------------------------------------------------------------

static constexpr RGB kRelax(50, 0, 0), kStretch(255, 50, 0);
constexpr int kStretchLength = 10, kSegmentDelay = 3, kLegDelay = 2;
constexpr int kWaveSteps = 37 + kStretchLength;  // longest segment + tail
constexpr int kCycleSteps = kWaveSteps + 5;      // 0.1 s rest between contractions

void MuscleWave::begin(Frame& f, Context& c) {
  f.fill(colours_ ? RGB() : kRelax);
  for (auto& w : wave_colour_) w = RGB(c.rng.range(100, 255), c.rng.range(100, 255), c.rng.range(100, 255));
  step_index_ = 0;
  step_.reset(c.now_ms);
}

void MuscleWave::drawStep(Frame& f, int leg, int step) {
  for (int g = 0; g < kSegmentCount; ++g)
    for (int j = 0; j < kSegments[g].count; ++j) {
      RGB& p = f.px[leg][kSegments[g].first + j];
      int d = step - g * kSegmentDelay - j;
      if (d < 0) d = -d;
      if (d < kStretchLength) {
        const float scale = 1.f - static_cast<float>(d) / kStretchLength;
        p = colours_ ? lerp(p, wave_colour_[leg], scale) : lerp(kRelax, kStretch, scale);
      } else if (!colours_) {
        p = kRelax;
      }
    }
}

void MuscleWave::render(Frame& f, Context& c) {
  for (int n = step_.due(c.now_ms, c.speed); n > 0; --n) {
    ++step_index_;
    for (int l = 0; l < kLegs; ++l) {
      const int s = ((step_index_ - l * kLegDelay) % kCycleSteps + kCycleSteps) % kCycleSteps;
      if (colours_ && s == 0)
        wave_colour_[l] = RGB(c.rng.range(100, 255), c.rng.range(100, 255), c.rng.range(100, 255));
      if (s < kWaveSteps) drawStep(f, l, s);
    }
  }
}

// ---- Purple wave ------------------------------------------------------------

static RGB purpleGradient(int step) { return lerp(RGB(148, 0, 211), RGB(0, 255, 255), step * 0.1f); }

void PurpleWave::begin(Frame& f, Context& c) {
  f.fill(RGB());
  phase_ = Phase::On;
  i_ = 0;
  step_.reset(c.now_ms);
}

void PurpleWave::render(Frame& f, Context& c) {
  constexpr int kKneeSteps = 16, kTipSteps = 10, kSparkleSteps = 60;  // 60 x 50 ms = 3 s
  for (int n = step_.due(c.now_ms, c.speed); n > 0; --n) {
    for (auto& leg : f.px) {
      switch (phase_) {
        case Phase::On:
          if (i_ < kKneeSteps) {
            setIfOnLeg(leg, 67 - i_, purpleGradient(i_));
            setIfOnLeg(leg, 67 + i_, purpleGradient(i_));
          } else {
            const int k = i_ - kKneeSteps;
            setIfOnLeg(leg, 83 + k, purpleGradient(k + 20));
            setIfOnLeg(leg, 93 + k, purpleGradient(k + 20));
            setIfOnLeg(leg, 51 - k, purpleGradient(k + 20));
          }
          break;
        case Phase::Sparkle:
          if (c.rng.chance(0.67f)) leg[c.rng.range(0, 41)] = RGB(0, 255, c.rng.range(128, 255));
          break;
        case Phase::Off:
          if (i_ < kTipSteps) {
            const int k = kTipSteps - 1 - i_;
            setIfOnLeg(leg, 83 + k, RGB());
            setIfOnLeg(leg, 93 + k, RGB());
            setIfOnLeg(leg, 51 - k, RGB());
          } else {
            const int k = kKneeSteps - 1 - (i_ - kTipSteps);
            setIfOnLeg(leg, 67 - k, RGB());
            setIfOnLeg(leg, 67 + k, RGB());
          }
          // The original never cleared the sparkle zone (0-41); clear it here too.
          setIfOnLeg(leg, 2 * i_, RGB());
          setIfOnLeg(leg, 2 * i_ + 1, RGB());
          break;
      }
    }
    ++i_;
    if (phase_ == Phase::On && i_ == kKneeSteps + kTipSteps) {
      phase_ = Phase::Sparkle;
      i_ = 0;
    } else if (phase_ == Phase::Sparkle && i_ == kSparkleSteps) {
      phase_ = Phase::Off;
      i_ = 0;
    } else if (phase_ == Phase::Off && i_ == kKneeSteps + kTipSteps) {
      phase_ = Phase::On;
      i_ = 0;
    }
  }
}

// ---- Sound reactive ---------------------------------------------------------

void SoundReactive::begin(Frame& f, Context& c) {
  f.fill(RGB());
  count_ = 0;
  bass_ = 0.f;
  last_ms_ = c.now_ms;
  next_fake_beat_ms_ = c.now_ms;
}

void SoundReactive::render(Frame& f, Context& c) {
  const float dt_ms = (c.now_ms - last_ms_) * c.speed;
  last_ms_ = c.now_ms;

  float bass_target, treble;
  bool beat;
  if (c.audio.valid) {
    bass_target = c.audio.bass;
    treble = c.audio.treble;
    beat = c.audio.beat;
  } else {  // no microphone: a steady 120 BPM pulse
    bass_target = 0.4f + 0.3f * sinf(c.now_ms * 0.002f);
    treble = 0.2f;
    beat = c.now_ms >= next_fake_beat_ms_;
    if (beat) next_fake_beat_ms_ = c.now_ms + 500;
  }
  bass_ += (bass_target - bass_) * 0.3f;

  if (beat && count_ < kMaxPulses) {
    pulses_[count_++] = {0.f, wheel(hue_)};
    hue_ += 37;
  }
  int kept = 0;
  for (int p = 0; p < count_; ++p) {
    pulses_[p].pos += dt_ms * 0.06f;  // 60 LEDs per second
    if (pulses_[p].pos < kPixelsPerLeg + kLegs * 1.5f + 10) pulses_[kept++] = pulses_[p];
  }
  count_ = kept;

  for (int l = 0; l < kLegs; ++l)
    for (int i = 0; i < kPixelsPerLeg; ++i) {
      RGB px = lerp(RGB(), RGB(140, 25, 0), bass_ * (0.7f + 0.3f * c.rng.uniform()));  // embers
      for (int p = 0; p < count_; ++p) {
        const float d = pulses_[p].pos - l * 1.5f - i;  // legs fire slightly one after another
        if (d >= 0.f && d < 10.f) px = lerp(px, pulses_[p].colour, 1.f - d / 10.f);
      }
      if (c.rng.chance(treble * 0.02f)) px = RGB(255, 255, 255);
      f.px[l][i] = px;
    }
}

}  // namespace crabfx
