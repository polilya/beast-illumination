// INMP441 microphone on I2S1, analysed on core 0 so the LED loop on core 1 never waits.
#pragma once

#include <crabfx.h>

namespace audio_input {

// Starts the capture task. Returns false if the I2S driver could not be installed.
bool begin();

// Latest features; valid is false when no microphone is answering.
crabfx::AudioFeatures latest();

}  // namespace audio_input
