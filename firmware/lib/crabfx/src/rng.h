// Small seedable PRNG so effects are deterministic in tests and the renderer.
#pragma once

#include <stdint.h>

namespace crabfx {

class Rng {
 public:
  explicit Rng(uint32_t seed = 0x1234567u) { reseed(seed); }
  void reseed(uint32_t seed) { state_ = seed ? seed : 0x1234567u; }

  // xorshift32
  uint32_t next() {
    uint32_t x = state_;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return state_ = x;
  }
  // Uniform in [0, 1).
  float uniform() { return (next() >> 8) * (1.0f / 16777216.0f); }
  // Uniform integer in [lo, hi], inclusive like Python's random.randint.
  int range(int lo, int hi) { return lo + static_cast<int>(next() % static_cast<uint32_t>(hi - lo + 1)); }
  bool chance(float p) { return uniform() < p; }

 private:
  uint32_t state_;
};

}  // namespace crabfx
