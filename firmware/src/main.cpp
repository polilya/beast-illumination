// Beast illumination firmware: 12 legs x 102 WS2812B LEDs on one ESP32.
//
// Core 1 (this loop): render the current effect, push all 12 strips in parallel
// over I2S, serve the control page. Core 0: Wi-Fi and the microphone task.

#define FASTLED_ESP32_I2S 1  // drive all 12 strips at once from the I2S0 peripheral
#include <Arduino.h>
#include <FastLED.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <crabfx.h>

#include "audio_input.h"
#include "config.h"
#include "web_ui.h"

using crabfx::kLegs;
using crabfx::kPixelsPerLeg;

static CRGB leds[kLegs][kPixelsPerLeg];
static crabfx::Engine engine(esp_random());
static crabfx::Frame out;
static WebServer server(80);
static Preferences prefs;
static bool mic_ok = false;

template <int... Pins>
static void addLegs() {
  int leg = 0;
  // FastLED needs each pin as a compile-time constant.
  (FastLED.addLeds<WS2812B, Pins, GRB>(leds[leg++], kPixelsPerLeg), ...);
}

static void saveSettings() {
  prefs.putInt("mode", engine.mode());
  prefs.putFloat("speed", engine.speed());
  prefs.putInt("profile", engine.powerProfile());
  prefs.putUChar("bright", engine.brightness());
  prefs.putUInt("avg", engine.powerLimits().average_ma);
  prefs.putUInt("peak", engine.powerLimits().peak_ma);
}

static void loadSettings() {
  engine.setSpeed(prefs.getFloat("speed", 1.f));
  const int profile = prefs.getInt("profile", kDefaultPowerProfile);
  engine.setPowerProfile(profile >= 0 ? profile : kDefaultPowerProfile);
  if (profile < 0) {  // hand-tuned: brightness and limits on top of the default profile
    crabfx::PowerLimits l = engine.powerLimits();
    l.average_ma = prefs.getUInt("avg", l.average_ma);
    l.peak_ma = prefs.getUInt("peak", l.peak_ma);
    engine.setPowerLimits(l);
    engine.setBrightness(prefs.getUChar("bright", engine.brightness()));
  }
  engine.setMode(prefs.getInt("mode", 0), millis());
}

static void handleState() {
  String json = "{\"modes\":[";
  for (int i = 0; i < engine.modeCount(); ++i) {
    if (i) json += ',';
    json += '"';
    json += engine.modeName(i);
    json += '"';
  }
  json += "],\"profiles\":[";
  for (int i = 0; i < crabfx::kPowerProfileCount; ++i) {
    if (i) json += ',';
    json += '"';
    json += crabfx::kPowerProfiles[i].name;
    json += '"';
  }
  const crabfx::PowerLimiter& p = engine.power();
  const crabfx::AudioFeatures& a = engine.audio();
  json += "],\"mode\":" + String(engine.mode());
  json += ",\"profile\":" + String(engine.powerProfile());
  json += ",\"brightness\":" + String(engine.brightness());
  json += ",\"speed\":" + String(engine.speed(), 2);
  json += ",\"average_limit\":" + String(engine.powerLimits().average_ma);
  json += ",\"peak_limit\":" + String(engine.powerLimits().peak_ma);
  json += ",\"requested\":" + String(p.requested_ma);
  json += ",\"delivered\":" + String(p.delivered_ma);
  json += ",\"average\":" + String(p.average_ma, 0);
  json += ",\"reserve\":" + String(p.reserve, 2);
  json += ",\"scale\":" + String(p.scale, 2);
  json += ",\"white_trim\":" + String(p.white_trim, 2);
  json += ",\"audio\":{\"valid\":" + String(a.valid ? "true" : "false");
  json += ",\"level\":" + String(a.level, 2) + "}}";
  server.send(200, "application/json", json);
}

static void handleSet() {
  if (server.hasArg("mode")) engine.setMode(server.arg("mode").toInt(), millis());
  if (server.hasArg("speed")) engine.setSpeed(server.arg("speed").toFloat());
  if (server.hasArg("profile")) engine.setPowerProfile(server.arg("profile").toInt());
  if (server.hasArg("brightness")) engine.setBrightness(constrain(server.arg("brightness").toInt(), 0, 255));
  if (server.hasArg("average") || server.hasArg("peak")) {
    crabfx::PowerLimits l = engine.powerLimits();
    if (server.hasArg("average")) l.average_ma = constrain(server.arg("average").toInt(), 1500, 80000);
    if (server.hasArg("peak")) l.peak_ma = constrain(server.arg("peak").toInt(), 1500, 80000);
    engine.setPowerLimits(l);
  }
  saveSettings();
  handleState();
}

void setup() {
  Serial.begin(115200);

  addLegs<LEG_PINS>();
  FastLED.setBrightness(255);  // brightness, gamma and the power limit are applied by the engine
  FastLED.setDither(0);
  FastLED.clear(true);

  prefs.begin("crab", false);
  loadSettings();

  mic_ok = audio_input::begin();
  Serial.printf("microphone driver: %s\n", mic_ok ? "ok" : "failed");

  WiFi.softAP(kApSsid, kApPassword);
  server.on("/", [] { server.send_P(200, "text/html", kIndexHtml); });
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/set", HTTP_POST, handleSet);
  server.begin();
  Serial.printf("control page: http://%s\n", WiFi.softAPIP().toString().c_str());
}

void loop() {
  static uint32_t next_frame_ms = 0;
  server.handleClient();

  const uint32_t now = millis();
  if (static_cast<int32_t>(now - next_frame_ms) < 0) return;
  next_frame_ms = now + kFrameIntervalMs;

  if (mic_ok) engine.setAudio(audio_input::latest());
  engine.tick(now, out);
  for (int l = 0; l < kLegs; ++l)
    for (int i = 0; i < kPixelsPerLeg; ++i) {
      const crabfx::RGB& p = out.px[l][i];
      leds[l][i] = CRGB(p.r, p.g, p.b);
    }
  FastLED.show();
}
