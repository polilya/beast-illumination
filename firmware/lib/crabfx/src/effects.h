// The lighting effects. Each one is a port of a prototype in legacy/, rewritten
// to be non-blocking: render() is called every frame with the current time and
// advances the animation by however many of its own steps are due, so all 12
// legs update in the same frame and the main loop never sleeps.
#pragma once

#include <stdint.h>

#include "audio.h"
#include "frame.h"
#include "rng.h"

namespace crabfx {

struct Context {
  uint32_t now_ms;
  float speed;  // 1 = original timing
  Rng& rng;
  const AudioFeatures& audio;
};

// Converts elapsed time into whole animation steps of a fixed period.
class Stepper {
 public:
  explicit Stepper(float period_ms) : period_ms_(period_ms) {}
  void reset(uint32_t now_ms) { last_ms_ = now_ms; acc_ms_ = 0.f; }
  // Steps due since the last call; capped so a stall never causes a burst.
  int due(uint32_t now_ms, float speed);

 private:
  float period_ms_;
  uint32_t last_ms_ = 0;
  float acc_ms_ = 0.f;
};

class Effect {
 public:
  virtual ~Effect() = default;
  virtual const char* name() const = 0;
  virtual void begin(Frame& f, Context& c);
  virtual void render(Frame& f, Context& c) = 0;
};

// legacy/micropython/fire_waves.py: flames that flare up at random points and spread.
class FireWaves : public Effect {
 public:
  const char* name() const override { return "Fire waves"; }
  void begin(Frame& f, Context& c) override;
  void render(Frame& f, Context& c) override;

  static constexpr int kMaxWaves = 19;
  int waveCount(int leg) const { return count_[leg]; }

 private:
  struct Wave { int center, radius, max_radius, intensity; };
  Wave waves_[kLegs][kMaxWaves];
  int count_[kLegs] = {};
  Stepper step_{80};
};

// legacy/micropython/fire_sparks.py: sparks plus waves that run along one segment.
class FireSparks : public Effect {
 public:
  const char* name() const override { return "Fire sparks"; }
  void begin(Frame& f, Context& c) override;
  void render(Frame& f, Context& c) override;

 private:
  struct Wave { int leg, start, step; };
  static constexpr int kMaxWaves = 16;
  Wave waves_[kMaxWaves];
  int count_ = 0;
  Stepper step_{50};
};

// legacy/micropython/bioluminescence.py: blue-green plankton sparkle.
class Bioluminescence : public Effect {
 public:
  const char* name() const override { return "Bioluminescence"; }
  void begin(Frame& f, Context& c) override;
  void render(Frame& f, Context& c) override;

 private:
  float level_[kLegs][kPixelsPerLeg];
  Stepper step_{50};
};

// legacy/micropython/breathing_spasm.py: slow breathing, neuron flickers, red spasms.
class BreathingSpasm : public Effect {
 public:
  const char* name() const override { return "Breathing"; }
  void begin(Frame& f, Context& c) override;
  void render(Frame& f, Context& c) override;

 private:
  float t_s_ = 0.f;
  Stepper step_{50};
};

// legacy/micropython/disco.py
class Disco : public Effect {
 public:
  const char* name() const override { return "Disco"; }
  void render(Frame& f, Context& c) override;

 private:
  Stepper step_{100};
};

// legacy/micropython/resonance.py: legs glow one after another in random order.
class Resonance : public Effect {
 public:
  const char* name() const override { return "Resonance"; }
  void begin(Frame& f, Context& c) override;
  void render(Frame& f, Context& c) override;

 private:
  void shuffle(Rng& rng);
  int order_[kLegs];
  int index_ = 0, phase_ = 0;
  Stepper step_{20};
};

// legacy/micropython/heartbeat_1leg.py: two quick beats and a slow one, on all legs.
class Heartbeat : public Effect {
 public:
  const char* name() const override { return "Heartbeat"; }
  void begin(Frame& f, Context& c) override;
  void render(Frame& f, Context& c) override;

  // Brightness envelope 0..1 at time t within the repeating pattern.
  static float envelope(float t_s);
  static float cycleSeconds();

 private:
  float t_s_ = 0.f;
  uint32_t last_ms_ = 0;
};

// legacy/micropython/muscle_wave_{red,colours}_1leg.py: a contraction running
// along each segment, offset leg to leg like a walking gait.
class MuscleWave : public Effect {
 public:
  explicit MuscleWave(bool colours) : colours_(colours) {}
  const char* name() const override { return colours_ ? "Muscle wave (colours)" : "Muscle wave"; }
  void begin(Frame& f, Context& c) override;
  void render(Frame& f, Context& c) override;

 private:
  void drawStep(Frame& f, int leg, int step);
  bool colours_;
  RGB wave_colour_[kLegs];
  int step_index_ = 0;
  Stepper step_{20};
};

// legacy/micropython/purple_wave_4legs.py: purple-to-cyan wave spreads from the
// knee, cyan sparkles, then the wave retracts.
class PurpleWave : public Effect {
 public:
  const char* name() const override { return "Purple wave"; }
  void begin(Frame& f, Context& c) override;
  void render(Frame& f, Context& c) override;

 private:
  enum class Phase { On, Sparkle, Off };
  Phase phase_ = Phase::On;
  int i_ = 0;
  Stepper step_{50};
};

// New: driven by the microphone. Bass sets the ember glow, each beat sends a pulse
// up every leg, treble adds sparks. Without a microphone it runs on a 120 BPM clock.
class SoundReactive : public Effect {
 public:
  const char* name() const override { return "Sound reactive"; }
  void begin(Frame& f, Context& c) override;
  void render(Frame& f, Context& c) override;

 private:
  struct Pulse { float pos; RGB colour; };
  static constexpr int kMaxPulses = 6;
  Pulse pulses_[kMaxPulses];
  int count_ = 0;
  uint8_t hue_ = 0;
  float bass_ = 0.f;
  uint32_t last_ms_ = 0, next_fake_beat_ms_ = 0;
};

}  // namespace crabfx
