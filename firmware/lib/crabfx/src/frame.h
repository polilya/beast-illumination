// Pixel buffer and leg geometry shared by every effect.
#pragma once

#include <stdint.h>

namespace crabfx {

constexpr int kLegs = 12;
constexpr int kPixelsPerLeg = 102;
constexpr int kPixels = kLegs * kPixelsPerLeg;

struct RGB {
  uint8_t r = 0, g = 0, b = 0;
  constexpr RGB() = default;
  constexpr RGB(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
  bool operator==(const RGB& o) const { return r == o.r && g == o.g && b == o.b; }
  bool operator!=(const RGB& o) const { return !(*this == o); }
};

// Linear blend from a to b; t is clamped to [0, 1]. Channels are truncated
// towards zero, like int() in the original MicroPython scripts.
inline RGB lerp(RGB a, RGB b, float t) {
  if (t < 0.f) t = 0.f;
  if (t > 1.f) t = 1.f;
  return RGB(static_cast<uint8_t>(a.r + (b.r - a.r) * t),
             static_cast<uint8_t>(a.g + (b.g - a.g) * t),
             static_cast<uint8_t>(a.b + (b.b - a.b) * t));
}

// One strip of 102 LEDs is wired as four runs along the leg.
struct Segment {
  int first;
  int count;
};
constexpr Segment kSegments[] = {
    {0, 30},   // top triangle
    {30, 37},  // main line, down
    {67, 25},  // main line, up
    {92, 10},  // parallel run
};
constexpr int kSegmentCount = sizeof(kSegments) / sizeof(kSegments[0]);

struct Frame {
  RGB px[kLegs][kPixelsPerLeg];

  void fill(RGB c) {
    for (auto& leg : px)
      for (auto& p : leg) p = c;
  }
  void fillLeg(int leg, RGB c) {
    for (auto& p : px[leg]) p = c;
  }
};

}  // namespace crabfx
