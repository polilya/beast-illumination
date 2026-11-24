// Owns every effect and turns (time, settings, audio) into the frame sent to the LEDs.
#pragma once

#include <stdint.h>

#include "audio.h"
#include "effects.h"
#include "frame.h"
#include "output.h"
#include "rng.h"

namespace crabfx {

class Engine {
 public:
  explicit Engine(uint32_t seed = 1);

  int modeCount() const { return kModes; }
  const char* modeName(int i) const { return effects_[i]->name(); }
  int mode() const { return mode_; }
  void setMode(int i, uint32_t now_ms);

  void setSpeed(float s) { speed_ = s < 0.1f ? 0.1f : (s > 4.f ? 4.f : s); }
  float speed() const { return speed_; }
  void setBrightness(uint8_t b) { brightness_ = b; profile_ = -1; }
  uint8_t brightness() const { return brightness_; }
  void setPowerLimits(const PowerLimits& l) { limiter_.setLimits(l); profile_ = -1; }
  const PowerLimits& powerLimits() const { return limiter_.limits(); }
  // Applies a preset from kPowerProfiles: brightness and limits together.
  void setPowerProfile(int i);
  int powerProfile() const { return profile_; }
  void setAudio(const AudioFeatures& a) { audio_ = a; }
  const AudioFeatures& audio() const { return audio_; }

  // Advances the current effect and writes gamma-corrected, power-limited pixels to out.
  const PowerLimiter& tick(uint32_t now_ms, Frame& out);
  const PowerLimiter& power() const { return limiter_; }
  const Frame& raw() const { return raw_; }

 private:
  static constexpr int kModes = 11;

  FireWaves fire_waves_;
  FireSparks fire_sparks_;
  Bioluminescence bioluminescence_;
  BreathingSpasm breathing_;
  Disco disco_;
  Resonance resonance_;
  Heartbeat heartbeat_;
  MuscleWave muscle_red_{false};
  MuscleWave muscle_colours_{true};
  PurpleWave purple_wave_;
  SoundReactive sound_;
  Effect* effects_[kModes];

  Rng rng_;
  Frame raw_;
  AudioFeatures audio_;
  int mode_ = 0;
  bool started_ = false;
  float speed_ = 1.f;
  uint8_t brightness_ = 255;
  int profile_ = -1;  // -1: limits set by hand
  uint32_t last_tick_ms_ = 0;
  PowerLimiter limiter_;
};

}  // namespace crabfx
