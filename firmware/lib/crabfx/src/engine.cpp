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

const PowerReport& Engine::tick(uint32_t now_ms, Frame& out) {
  if (!started_) setMode(mode_, now_ms);
  Context c{now_ms, speed_, rng_, audio_};
  effects_[mode_]->render(raw_, c);

  applyBrightnessAndGamma(raw_, out, brightness_);
  power_.requested_ma = estimateMilliamps(out);
  power_.scale = limitToBudget(out, budget_ma_);
  power_.delivered_ma = estimateMilliamps(out);
  return power_;
}

}  // namespace crabfx
