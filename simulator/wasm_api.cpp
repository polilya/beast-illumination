// C entry points that expose the firmware's effect engine (firmware/lib/crabfx)
// to the browser simulator through WebAssembly. Built by simulator/build.sh.
#include <emscripten/emscripten.h>

#include "crabfx.h"

using namespace crabfx;

static Engine* engine = nullptr;
static Frame out;
static float samples[AudioAnalyzer::kFftSize];
static AudioAnalyzer analyzer(16000.f);

extern "C" {

EMSCRIPTEN_KEEPALIVE void sim_init(uint32_t seed) {
  delete engine;
  engine = new Engine(seed);
}

EMSCRIPTEN_KEEPALIVE int sim_legs() { return kLegs; }
EMSCRIPTEN_KEEPALIVE int sim_pixels_per_leg() { return kPixelsPerLeg; }
EMSCRIPTEN_KEEPALIVE int sim_mode_count() { return engine->modeCount(); }
EMSCRIPTEN_KEEPALIVE const char* sim_mode_name(int i) { return engine->modeName(i); }
EMSCRIPTEN_KEEPALIVE void sim_set_mode(int i, uint32_t now_ms) { engine->setMode(i, now_ms); }
EMSCRIPTEN_KEEPALIVE void sim_set_speed(float s) { engine->setSpeed(s); }
EMSCRIPTEN_KEEPALIVE void sim_set_brightness(int b) { engine->setBrightness(static_cast<uint8_t>(b)); }
EMSCRIPTEN_KEEPALIVE void sim_set_budget(uint32_t ma) { engine->setPowerBudgetMa(ma); }

// Browser microphone: JS writes kFftSize samples at 16 kHz here, then calls sim_push_audio,
// which runs the same analyzer the ESP32 runs on its INMP441 data.
EMSCRIPTEN_KEEPALIVE float* sim_audio_buffer() { return samples; }
EMSCRIPTEN_KEEPALIVE int sim_audio_block() { return AudioAnalyzer::kFftSize; }
EMSCRIPTEN_KEEPALIVE void sim_push_audio(uint32_t now_ms) {
  AudioFeatures f = analyzer.process(samples, now_ms);
  f.beat = f.beat || engine->audio().beat;  // keep a beat until the next frame consumes it
  engine->setAudio(f);
}
EMSCRIPTEN_KEEPALIVE void sim_clear_audio() { engine->setAudio(AudioFeatures()); }

// Renders one frame; returns kLegs * kPixelsPerLeg RGB triplets (gamma-corrected,
// power-limited: exactly what the firmware sends to the strips).
EMSCRIPTEN_KEEPALIVE const uint8_t* sim_tick(uint32_t now_ms) {
  engine->tick(now_ms, out);
  AudioFeatures a = engine->audio();
  a.beat = false;
  engine->setAudio(a);
  return &out.px[0][0].r;
}

EMSCRIPTEN_KEEPALIVE uint32_t sim_requested_ma() { return engine->power().requested_ma; }
EMSCRIPTEN_KEEPALIVE uint32_t sim_delivered_ma() { return engine->power().delivered_ma; }
EMSCRIPTEN_KEEPALIVE float sim_audio_level() { return engine->audio().valid ? engine->audio().level : 0.f; }
}
