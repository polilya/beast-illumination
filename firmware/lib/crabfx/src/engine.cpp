#include "engine.h"

namespace crabfx {

Engine::Engine(uint32_t seed)
    : effects_{&fire_waves_,  &fire_sparks_, &bioluminescence_, &breathing_,     &disco_, &resonance_,
               &heartbeat_,   &muscle_red_,  &muscle_colours_,  &purple_wave_,   &sound_},
      rng_(seed) {}

void Engine::setMode(int i, uint32_t now_ms) {
  if (i < 0 || i >= kModes) return;
  mode_ = i;
  Context c{now_ms, speed_, rng_, audio_};
  effects_[mode_]->begin(raw_, c);
  started_ = true;
}

void Engine::setPowerProfile(int i) {
  if (i < 0 || i >= kPowerProfileCount) return;
  profile_ = i;
  brightness_ = kPowerProfiles[i].brightness;
  limiter_.setLimits(kPowerProfiles[i].limits);
}

const PowerLimiter& Engine::tick(uint32_t now_ms, Frame& out) {
  if (!started_) {
    setMode(mode_, now_ms);
    last_tick_ms_ = now_ms;
  }
  Context c{now_ms, speed_, rng_, audio_};
  effects_[mode_]->render(raw_, c);

  applyBrightnessAndGamma(raw_, out, brightness_);
  limiter_.apply(out, now_ms - last_tick_ms_);
  last_tick_ms_ = now_ms;
  return limiter_;
}

}  // namespace crabfx
