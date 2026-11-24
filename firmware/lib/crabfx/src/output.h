// Output stage: brightness, gamma correction and power management.
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

// Two limits instead of one. A supply and its wiring care about the instantaneous
// current; a battery and the heat in the strips care about the average. So short
// bright moments (a heartbeat, a flash) may go up to peak_ma, paid for from a
// reserve that refills whenever the draw is below average_ma, and the long-run
// average can never exceed average_ma plus one reserve.
struct PowerLimits {
  uint32_t average_ma = 6000;    // sustained draw: battery life, heat
  uint32_t peak_ma = 12000;      // instantaneous draw: converter and wiring rating
  uint32_t burst_ms = 3000;      // how long a full-peak burst the reserve holds
  uint32_t release_ms = 500;     // how fast brightness comes back after a dip
  float max_white_trim = 0.4f;   // share of a colour's white part removed before dimming
  uint8_t legs_per_supply = 12;  // legs fed by one converter (12 = a single supply)
  uint32_t supply_peak_ma = 0;   // per-converter limit; 0 = only the global peak
};

struct PowerProfile {
  const char* name;
  uint8_t brightness;
  PowerLimits limits;
};
// Starting points; match them to the real battery and converters.
extern const PowerProfile kPowerProfiles[];
extern const int kPowerProfileCount;

class PowerLimiter {
 public:
  PowerLimiter() { setLimits(PowerLimits()); }
  void setLimits(const PowerLimits& l);
  const PowerLimits& limits() const { return limits_; }

  // Limits f in place. dt_ms is the time since the previous frame.
  void apply(Frame& f, uint32_t dt_ms, const PowerModel& m = PowerModel());

  uint32_t requested_ma = 0;  // what the frame would draw unlimited
  uint32_t delivered_ma = 0;  // after limiting
  uint32_t allowed_ma = 0;    // ceiling for this frame
  float average_ma = 0.f;     // delivered, averaged over ~10 s
  float scale = 1.f;          // dimming applied (1 = none)
  float white_trim = 0.f;     // share of white removed this frame
  float reserve = 1.f;        // burst reserve left, 0..1

 private:
  float capacity() const;
  PowerLimits limits_;
  float reserve_mams_ = 0.f;  // mA x ms above average still allowed
  float smooth_scale_ = 1.f;
};

}  // namespace crabfx
