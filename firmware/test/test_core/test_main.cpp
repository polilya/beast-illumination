// Host-side tests for the effect engine: pio test -e native
#include <math.h>
#include <string.h>
#include <unity.h>

#include "crabfx.h"

using namespace crabfx;

void setUp() {}
void tearDown() {}

static Frame frame_a, frame_b;

void test_lerp_truncates_like_python_int() {
  // int(50 + (255 - 50) * 0.5) == 152 in the original scripts
  TEST_ASSERT_EQUAL_UINT8(152, lerp(RGB(50, 0, 0), RGB(255, 50, 0), 0.5f).r);
  TEST_ASSERT_TRUE(lerp(RGB(1, 2, 3), RGB(9, 9, 9), -1.f) == RGB(1, 2, 3));
  TEST_ASSERT_TRUE(lerp(RGB(1, 2, 3), RGB(9, 9, 9), 2.f) == RGB(9, 9, 9));
}

void test_segments_cover_the_leg_exactly() {
  int next = 0;
  for (const Segment& s : kSegments) {
    TEST_ASSERT_EQUAL_INT(next, s.first);
    next += s.count;
  }
  TEST_ASSERT_EQUAL_INT(kPixelsPerLeg, next);
}

void test_gamma_endpoints_and_monotonic() {
  TEST_ASSERT_EQUAL_UINT8(0, gamma8(0));
  TEST_ASSERT_EQUAL_UINT8(255, gamma8(255));
  for (int i = 1; i < 256; ++i) TEST_ASSERT_TRUE(gamma8(i) >= gamma8(i - 1));
  TEST_ASSERT_TRUE(gamma8(128) < 64);  // mid-grey is dark after gamma 2.2
}

void test_full_white_draws_about_75_amps() {
  frame_a.fill(RGB(255, 255, 255));
  // 1224 LEDs x (3 x 20 mA + 1 mA idle)
  TEST_ASSERT_EQUAL_UINT32(1224u * 61u, estimateMilliamps(frame_a));
  frame_a.fill(RGB());
  TEST_ASSERT_EQUAL_UINT32(1224u, estimateMilliamps(frame_a));
}

void test_limiter_fits_budget_and_leaves_small_frames_alone() {
  frame_a.fill(RGB(255, 255, 255));
  const float k = limitToBudget(frame_a, 10000);
  TEST_ASSERT_TRUE(k < 1.f);
  TEST_ASSERT_TRUE(estimateMilliamps(frame_a) <= 10000u);

  frame_a.fill(RGB(10, 0, 0));
  const Frame before = frame_a;
  TEST_ASSERT_EQUAL_FLOAT(1.f, limitToBudget(frame_a, 10000));
  TEST_ASSERT_EQUAL_MEMORY(&before, &frame_a, sizeof(Frame));
}

void test_stepper_scales_with_speed_and_caps_bursts() {
  Stepper s(50);
  s.reset(0);
  TEST_ASSERT_EQUAL_INT(2, s.due(100, 1.f));
  TEST_ASSERT_EQUAL_INT(4, s.due(200, 2.f));
  TEST_ASSERT_EQUAL_INT(0, s.due(220, 1.f));
  TEST_ASSERT_EQUAL_INT(4, s.due(60000, 1.f));  // a long stall does not replay 20 minutes
}

void test_heartbeat_envelope_shape() {
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 2.104f, Heartbeat::cycleSeconds());
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.f, Heartbeat::envelope(0.f));
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 1.f, Heartbeat::envelope(0.6f * 0.21f - 1e-5f));  // top of first beat
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.f, Heartbeat::envelope(2.0f));                  // resting gap
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, Heartbeat::envelope(0.1f), Heartbeat::envelope(0.1f + Heartbeat::cycleSeconds()));
}

void test_fft_finds_a_pure_tone() {
  const int n = 256, bin = 19;
  float re[n], im[n];
  for (int i = 0; i < n; ++i) {
    re[i] = sinf(2.f * 3.14159265f * bin * i / n);
    im[i] = 0.f;
  }
  fft(re, im, n);
  int best = 1;
  for (int k = 1; k < n / 2; ++k)
    if (re[k] * re[k] + im[k] * im[k] > re[best] * re[best] + im[best] * im[best]) best = k;
  TEST_ASSERT_EQUAL_INT(bin, best);
  TEST_ASSERT_FLOAT_WITHIN(0.5f, n / 2.f, sqrtf(re[bin] * re[bin] + im[bin] * im[bin]));
}

void test_beat_detector_fires_on_onsets_only() {
  BeatDetector d;
  uint32_t t = 0;
  for (int i = 0; i < 60; ++i, t += 33) TEST_ASSERT_FALSE(d.update(0.01f, t));  // steady hum
  TEST_ASSERT_TRUE(d.update(0.05f, t));                                         // kick
  TEST_ASSERT_FALSE(d.update(0.08f, t + 100));                                  // inside refractory window
  TEST_ASSERT_TRUE(d.update(0.2f, t + 400));
}

void test_analyzer_puts_a_bass_tone_in_the_bass_band() {
  static AudioAnalyzer a(16000.f);
  float s[AudioAnalyzer::kFftSize];
  AudioFeatures f;
  for (int block = 0; block < 5; ++block) {
    for (int i = 0; i < AudioAnalyzer::kFftSize; ++i) s[i] = 0.5f * sinf(2.f * 3.14159265f * 100.f * i / 16000.f);
    f = a.process(s, block * 32);
  }
  TEST_ASSERT_TRUE(f.valid);
  TEST_ASSERT_TRUE(f.bass > 0.9f);
  TEST_ASSERT_TRUE(f.treble < 0.2f);
}

void test_fire_waves_stay_within_limits_and_palette() {
  static FireWaves fx;
  Rng rng(7);
  AudioFeatures audio;
  Context c{0, 1.f, rng, audio};
  fx.begin(frame_a, c);
  for (uint32_t t = 0; t < 20000; t += 16) {
    c.now_ms = t;
    fx.render(frame_a, c);
    for (int l = 0; l < kLegs; ++l) TEST_ASSERT_TRUE(fx.waveCount(l) <= FireWaves::kMaxWaves);
  }
  const RGB palette[] = {{0, 0, 0}, {50, 0, 0}, {150, 30, 0}, {255, 80, 0}, {255, 150, 0}};
  for (auto& leg : frame_a.px)
    for (auto& p : leg) {
      bool ok = false;
      for (const RGB& q : palette) ok = ok || p == q;
      TEST_ASSERT_TRUE(ok);
    }
}

void test_every_mode_is_deterministic_for_a_seed() {
  static Engine e1(42), e2(42);
  for (int m = 0; m < e1.modeCount(); ++m) {
    e1.setMode(m, 0);
    e2.setMode(m, 0);
    for (uint32_t t = 0; t <= 5000; t += 16) {
      e1.tick(t, frame_a);
      e2.tick(t, frame_b);
    }
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&frame_a, &frame_b, sizeof(Frame), e1.modeName(m));
  }
}

void test_every_mode_lights_something_and_respects_budget() {
  static Engine e(3);
  e.setPowerBudgetMa(8000);
  for (int m = 0; m < e.modeCount(); ++m) {
    e.setMode(m, 0);
    uint32_t max_requested = 0;
    for (uint32_t t = 0; t <= 8000; t += 16) {
      const PowerReport& p = e.tick(t, frame_a);
      if (p.requested_ma > max_requested) max_requested = p.requested_ma;
      TEST_ASSERT_TRUE_MESSAGE(p.delivered_ma <= 8000u + 50u, e.modeName(m));
    }
    TEST_ASSERT_TRUE_MESSAGE(max_requested > 1224u + 500u, e.modeName(m));
  }
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_lerp_truncates_like_python_int);
  RUN_TEST(test_segments_cover_the_leg_exactly);
  RUN_TEST(test_gamma_endpoints_and_monotonic);
  RUN_TEST(test_full_white_draws_about_75_amps);
  RUN_TEST(test_limiter_fits_budget_and_leaves_small_frames_alone);
  RUN_TEST(test_stepper_scales_with_speed_and_caps_bursts);
  RUN_TEST(test_heartbeat_envelope_shape);
  RUN_TEST(test_fft_finds_a_pure_tone);
  RUN_TEST(test_beat_detector_fires_on_onsets_only);
  RUN_TEST(test_analyzer_puts_a_bass_tone_in_the_bass_band);
  RUN_TEST(test_fire_waves_stay_within_limits_and_palette);
  RUN_TEST(test_every_mode_is_deterministic_for_a_seed);
  RUN_TEST(test_every_mode_lights_something_and_respects_budget);
  return UNITY_END();
}
