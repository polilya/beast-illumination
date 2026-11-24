// Runs the firmware effect engine on the host and writes raw frames to stdout,
// for tools/render_gif.py. Usage: render_frames <mode> <seconds> <fps>
#include <stdio.h>
#include <stdlib.h>

#include "crabfx.h"

int main(int argc, char** argv) {
  if (argc != 4) {
    fprintf(stderr, "usage: %s <mode> <seconds> <fps>\n", argv[0]);
    return 2;
  }
  const int mode = atoi(argv[1]);
  const float seconds = static_cast<float>(atof(argv[2]));
  const int fps = atoi(argv[3]);

  static crabfx::Engine engine(2024);
  static crabfx::Frame out;
  if (mode < 0 || mode >= engine.modeCount()) {
    for (int i = 0; i < engine.modeCount(); ++i) fprintf(stderr, "%d %s\n", i, engine.modeName(i));
    return 2;
  }
  engine.setBrightness(255);
  crabfx::PowerLimits unlimited;  // show the effect as designed, not as a supply would cap it
  unlimited.average_ma = unlimited.peak_ma = 1000000;
  engine.setPowerLimits(unlimited);
  engine.setMode(mode, 0);

  // Warm up so persistent effects (sparks, fire) are already going in the first frame.
  for (uint32_t t = 0; t < 3000; t += 16) engine.tick(t, out);
  const int frames = static_cast<int>(seconds * fps);
  for (int k = 0; k < frames; ++k) {
    const uint32_t t0 = 3000 + k * 1000 / fps, t1 = 3000 + (k + 1) * 1000 / fps;
    for (uint32_t t = t0; t < t1; t += 8) engine.tick(t, out);
    fwrite(&out, sizeof(out), 1, stdout);
  }
  return 0;
}
