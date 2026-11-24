#include "output.h"

#include <math.h>

namespace crabfx {

uint8_t gamma8(uint8_t v) {
  static uint8_t lut[256];
  static bool ready = false;
  if (!ready) {
    for (int i = 0; i < 256; ++i) lut[i] = static_cast<uint8_t>(lroundf(255.f * powf(i / 255.f, 2.2f)));
    ready = true;
  }
  return lut[v];
}

uint32_t estimateMilliamps(const Frame& f, const PowerModel& m) {
  uint32_t duty_sum = 0;  // sum of all channel values, 0..255 each
  for (const auto& leg : f.px)
    for (const auto& p : leg) duty_sum += p.r + p.g + p.b;
  float ma = kPixels * m.idle_ma_per_led + duty_sum / 255.f * m.ma_per_channel;
  return static_cast<uint32_t>(ma + 0.5f);
}

float limitToBudget(Frame& f, uint32_t budget_ma, const PowerModel& m) {
  const float idle = kPixels * m.idle_ma_per_led;
  const float total = static_cast<float>(estimateMilliamps(f, m));
  if (total <= budget_ma || total <= idle) return 1.f;
  float k = (budget_ma - idle) / (total - idle);
  if (k < 0.f) k = 0.f;
  for (auto& leg : f.px)
    for (auto& p : leg) p = RGB(static_cast<uint8_t>(p.r * k), static_cast<uint8_t>(p.g * k), static_cast<uint8_t>(p.b * k));
  return k;
}

void applyBrightnessAndGamma(const Frame& raw, Frame& out, uint8_t brightness) {
  for (int l = 0; l < kLegs; ++l)
    for (int i = 0; i < kPixelsPerLeg; ++i) {
      const RGB& p = raw.px[l][i];
      out.px[l][i] = RGB(gamma8(p.r * brightness / 255), gamma8(p.g * brightness / 255), gamma8(p.b * brightness / 255));
    }
}

}  // namespace crabfx
