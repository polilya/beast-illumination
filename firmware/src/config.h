// Hardware wiring and defaults. Change these to match the build.
#pragma once

#include <stdint.h>

// One WS2812B strip of 102 LEDs per leg, each on its own GPIO (same pins as the prototypes).
// GPIO 2, 5, 12 and 15 are ESP32 strapping pins: a strip that pulls GPIO 12 high at
// power-up selects 1.8 V flash and the board will not boot. Keep a series resistor on
// every data line and power the strips after the ESP32, or move these legs to other pins.
#define LEG_PINS 2, 4, 5, 12, 13, 14, 15, 18, 19, 21, 22, 23

// INMP441 I2S microphone (L/R tied to GND -> left channel). Runs on I2S1, because the
// parallel LED driver uses I2S0.
constexpr int kMicSckPin = 26;
constexpr int kMicWsPin = 25;
constexpr int kMicSdPin = 33;

// Wi-Fi access point for the control page at http://192.168.4.1
constexpr const char* kApSsid = "beast-crab";
constexpr const char* kApPassword = "change-me-please";  // 8+ characters

// Power profile used until one is chosen on the phone: index into
// crabfx::kPowerProfiles (0 Eco, 1 Normal, 2 Show). Set the profiles' limits in
// firmware/lib/crabfx/src/output.cpp to the real battery and converters.
constexpr int kDefaultPowerProfile = 0;

constexpr uint32_t kFrameIntervalMs = 16;  // ~60 frames per second
