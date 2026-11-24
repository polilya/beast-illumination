// Output stage: brightness, gamma correction and the power budget.
#pragma once

#include <stdint.h>

#include "frame.h"

namespace crabfx {

// Gamma 2.2 lookup, so a fade that is linear in code also looks linear to the eye.
uint8_t gamma8(uint8_t v);

// WS2812B current model: each colour channel draws up to ~20 mA at full duty,
// plus ~1 mA quiescent per LED even when dark.
struct PowerModel {
  float ma_per_channel = 20.f;
  float idle_ma_per_led = 1.f;
};

uint32_t estimateMilliamps(const Frame& f, const PowerModel& m = PowerModel());

// Scales the whole frame down evenly so its estimated draw fits the budget.
// Returns the scale applied (1 when the frame already fits).
float limitToBudget(Frame& f, uint32_t budget_ma, const PowerModel& m = PowerModel());

// raw -> out: master brightness, then gamma.
void applyBrightnessAndGamma(const Frame& raw, Frame& out, uint8_t brightness);

}  // namespace crabfx
