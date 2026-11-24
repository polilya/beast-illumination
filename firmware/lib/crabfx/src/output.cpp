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

const PowerProfile kPowerProfiles[] = {
    // name, brightness, {average mA, peak mA, burst ms, release ms, white trim, legs per supply, supply peak mA}
    {"Eco", 180, {6000, 12000, 3000, 500, 0.4f, 12, 0}},
    {"Normal", 220, {12000, 20000, 3000, 400, 0.3f, 12, 0}},
    {"Show", 255, {20000, 30000, 5000, 300, 0.2f, 12, 0}},
};
const int kPowerProfileCount = sizeof(kPowerProfiles) / sizeof(kPowerProfiles[0]);

void PowerLimiter::setLimits(const PowerLimits& l) {
  limits_ = l;
  if (limits_.peak_ma < limits_.average_ma) limits_.peak_ma = limits_.average_ma;
  if (limits_.legs_per_supply < 1) limits_.legs_per_supply = 1;
  reserve_mams_ = capacity();
}

float PowerLimiter::capacity() const {
  return static_cast<float>(limits_.peak_ma - limits_.average_ma) * limits_.burst_ms;
}

void PowerLimiter::apply(Frame& f, uint32_t dt_ms, const PowerModel& m) {
  const float cap = capacity();
  reserve = cap > 0.f ? reserve_mams_ / cap : 0.f;
  // The ceiling slides from peak (full reserve) down to average (empty reserve),
  // so a long burst fades out instead of hitting a wall.
  allowed_ma = static_cast<uint32_t>(limits_.average_ma + (limits_.peak_ma - limits_.average_ma) * reserve);
  requested_ma = estimateMilliamps(f, m);

  // 1. White trim. A pixel's white part, min(r, g, b), costs all three channels.
  // Removing some of it saves current with little visible change: colours get a
  // touch more saturated before anything gets darker.
  white_trim = 0.f;
  if (requested_ma > allowed_ma && limits_.max_white_trim > 0.f) {
    uint32_t white = 0;
    for (const auto& leg : f.px)
      for (const auto& p : leg) white += p.r < p.g ? (p.r < p.b ? p.r : p.b) : (p.g < p.b ? p.g : p.b);
    const float excess_duty = (requested_ma - allowed_ma) / m.ma_per_channel * 255.f;
    if (white > 0) {
      white_trim = excess_duty / (3.f * white);
      if (white_trim > limits_.max_white_trim) white_trim = limits_.max_white_trim;
      for (auto& leg : f.px)
        for (auto& p : leg) {
          const uint8_t w = p.r < p.g ? (p.r < p.b ? p.r : p.b) : (p.g < p.b ? p.g : p.b);
          const uint8_t cut = static_cast<uint8_t>(w * white_trim);
          p = RGB(p.r - cut, p.g - cut, p.b - cut);
        }
    }
  }

  // 2. Dimming, with instant attack (never above the ceiling) and slow release,
  // so the light does not pump when a mode hovers around the limit.
  const float idle = kPixels * m.idle_ma_per_led;
  const float now_ma = static_cast<float>(estimateMilliamps(f, m));
  float target = 1.f;
  if (now_ma > allowed_ma && now_ma > idle) target = (allowed_ma - idle) / (now_ma - idle);
  if (target < 0.f) target = 0.f;
  if (target < smooth_scale_) {
    smooth_scale_ = target;
  } else {
    const float step = limits_.release_ms ? static_cast<float>(dt_ms) / limits_.release_ms : 1.f;
    smooth_scale_ += (target - smooth_scale_) * (step > 1.f ? 1.f : step);
  }
  scale = smooth_scale_;
  if (scale < 1.f)
    for (auto& leg : f.px)
      for (auto& p : leg) p = RGB(static_cast<uint8_t>(p.r * scale), static_cast<uint8_t>(p.g * scale), static_cast<uint8_t>(p.b * scale));

  // 3. Per-converter limit, when the legs are fed by several supplies.
  if (limits_.supply_peak_ma > 0) {
    const float idle_ma = m.idle_ma_per_led * kPixelsPerLeg;
    for (int first = 0; first < kLegs; first += limits_.legs_per_supply) {
      const int last = first + limits_.legs_per_supply < kLegs ? first + limits_.legs_per_supply : kLegs;
      uint32_t duty = 0;
      for (int l = first; l < last; ++l)
        for (const auto& p : f.px[l]) duty += p.r + p.g + p.b;
      const float group_idle = idle_ma * (last - first);
      const float group_ma = group_idle + duty / 255.f * m.ma_per_channel;
      if (group_ma <= limits_.supply_peak_ma) continue;
      const float k = (limits_.supply_peak_ma - group_idle) / (group_ma - group_idle);
      for (int l = first; l < last; ++l)
        for (auto& p : f.px[l]) p = RGB(static_cast<uint8_t>(p.r * k), static_cast<uint8_t>(p.g * k), static_cast<uint8_t>(p.b * k));
    }
  }

  delivered_ma = estimateMilliamps(f, m);
  reserve_mams_ += (static_cast<float>(limits_.average_ma) - delivered_ma) * dt_ms;
  if (reserve_mams_ > cap) reserve_mams_ = cap;
  if (reserve_mams_ < 0.f) reserve_mams_ = 0.f;
  const float a = dt_ms / 10000.f;
  average_ma += (delivered_ma - average_ma) * (a > 1.f ? 1.f : a);
}

}  // namespace crabfx
