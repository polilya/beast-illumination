"""Renders README preview GIFs from the firmware effect engine.

Builds tools/render_frames.cpp against firmware/lib/crabfx, runs it for each
mode, and draws the frames on the leg geometry from simulator/geometry.js.
Needs a C++17 compiler, Node.js (to read geometry.js) and Pillow.

    python3 tools/render_gif.py            # the README previews -> docs/gifs/
    python3 tools/render_gif.py 0 3        # selected modes
    python3 tools/render_gif.py all        # every mode
"""
import json
import pathlib
import re
import subprocess
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = pathlib.Path(__file__).resolve().parent.parent
LIB = ROOT / "firmware/lib/crabfx/src"
BIN = ROOT / "tools/build/render_frames"
OUT = ROOT / "docs/gifs"
LEGS, PER_LEG = 12, 102
SECONDS, FPS, WIDTH = 3, 12, 560


def build():
    BIN.parent.mkdir(parents=True, exist_ok=True)
    sources = [str(p) for p in LIB.glob("*.cpp")] + [str(ROOT / "tools/render_frames.cpp")]
    subprocess.run(["c++", "-std=c++17", "-O2", "-I", str(LIB), *sources, "-o", str(BIN)], check=True)


def geometry():
    js = (
        "global.window={};require(process.argv[1]);"
        "const G=window.CRAB_GEOMETRY;"
        "console.log(JSON.stringify({legs:G.LEGS,frame:G.FRAME.concat(G.LEG.map(s=>s.path)),pos:window.crabLedPositions()}))"
    )
    out = subprocess.run(["node", "-e", js, str(ROOT / "simulator/geometry.js")], check=True, capture_output=True, text=True)
    return json.loads(out.stdout)


def mode_names():
    res = subprocess.run([str(BIN), "-1", "0", "0"], capture_output=True, text=True)
    return [line.split(" ", 1)[1] for line in res.stderr.splitlines()]


def render(mode, name, geo):
    raw = subprocess.run([str(BIN), str(mode), str(SECONDS), str(FPS)], check=True, capture_output=True).stdout
    frame_bytes = LEGS * PER_LEG * 3

    def place(leg, p):
        return (leg["x"] + (-p[0] if leg["mirror"] else p[0]), leg["y"] + p[1])

    pts = [place(geo["legs"][l], geo["pos"][i]) for l in range(LEGS) for i in range(PER_LEG)]
    pad = 60
    min_x, min_y = min(p[0] for p in pts) - pad, min(p[1] for p in pts) - pad
    max_x, max_y = max(p[0] for p in pts) + pad, max(p[1] for p in pts) + pad
    s = WIDTH / (max_x - min_x)
    size = (WIDTH, int((max_y - min_y) * s))
    xy = lambda p: ((p[0] - min_x) * s, (p[1] - min_y) * s)

    base = Image.new("RGB", size, "black")
    d = ImageDraw.Draw(base)
    for leg in geo["legs"]:
        for seg in geo["frame"]:
            d.line([xy(place(leg, p)) for p in seg], fill=(28, 28, 32), width=max(1, int(10 * s)))

    to_screen = [round(255 * (v / 255) ** (1 / 2.2)) for v in range(256)]
    images = []
    for k in range(len(raw) // frame_bytes):
        f = raw[k * frame_bytes:(k + 1) * frame_bytes]
        glow = Image.new("RGB", size, "black")
        g = ImageDraw.Draw(glow)
        core = base.copy()
        c = ImageDraw.Draw(core)
        for j, p in enumerate(pts):
            col = tuple(to_screen[v] for v in f[3 * j:3 * j + 3])
            if sum(col) < 6:
                continue
            x, y = xy(p)
            g.ellipse([x - 4, y - 4, x + 4, y + 4], fill=col)
            c.ellipse([x - 1.6, y - 1.6, x + 1.6, y + 1.6], fill=col)
        glow = glow.filter(ImageFilter.GaussianBlur(4))
        images.append(_add(core, glow).quantize(colors=64, dither=Image.Dither.NONE))
    slug = re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-")
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / f"{mode:02d}-{slug}.gif"
    images[0].save(path, save_all=True, append_images=images[1:], duration=1000 // FPS, loop=0, optimize=True)
    print(path.relative_to(ROOT))


def _add(core, glow):
    """Additive light: the dots plus a doubled blurred halo, like LEDs behind diffuser."""
    return ImageChops.add(core, ImageChops.add(glow, glow))


def main():
    build()
    geo = geometry()
    names = mode_names()
    readme = [0, 8, 9, 10]  # fire waves, muscle wave (colours), purple wave, sound reactive
    if sys.argv[1:] == ["all"]:
        modes = range(len(names))
    else:
        modes = [int(a) for a in sys.argv[1:]] or readme
    for m in modes:
        render(m, names[m], geo)


if __name__ == "__main__":
    main()
