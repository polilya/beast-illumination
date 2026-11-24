// Where every LED sits, so the simulator shows the effects on the real leg shape.
//
// LEG: the LED runs of one leg, traced from a photo of the built leg (pixel
// coordinates, origin at the top crank pivot, y pointing down). Address ranges
// follow the strip as wired: data enters at the bottom of the triangle's vertical
// edge (address 0). The split of 67-91 between the foot's right edge and the
// curved link is estimated from their lengths.
//
// LEGS: where each of the 12 strips (in firmware pin order) is placed. This
// layout - two sides of three mirrored pairs, as in the photo - is an assumption;
// edit x, y and mirror to match the crab.
window.CRAB_GEOMETRY = {
  // Joints, from the photo: A top crank pivot, B middle joint, C triangle corner,
  // D knee, E foot joint, F foot tip.
  LEG: [
    { name: "top triangle, B to A", first: 0, count: 10, path: [[-60, 270], [0, 0]] },
    { name: "top triangle, A to C", first: 10, count: 13, path: [[0, 0], [-325, 210]] },
    { name: "top triangle, C to B", first: 23, count: 7, path: [[-325, 210], [-60, 270]] },
    { name: "upper link C to D (main line down)", first: 30, count: 12, path: [[-325, 210], [-510, 425]] },
    { name: "foot outer edge D to F (main line down)", first: 42, count: 25, path: [[-510, 425], [-290, 840]] },
    { name: "foot inner edge F to E (main line up)", first: 67, count: 14, path: [[-290, 840], [-290, 465]] },
    { name: "curved link E to B (main line up)", first: 81, count: 11, path: [[-290, 465], [-190, 360], [-60, 270]] },
    { name: "foot top edge E to D (parallel run)", first: 92, count: 10, path: [[-290, 465], [-510, 425]] },
  ],
  // Frame members without LEDs, drawn faintly for context (B to the central crank).
  FRAME: [
    [[-60, 270], [135, 270]],
  ],
  LEGS: [
    { x: 0, y: 0, mirror: false }, { x: 330, y: 0, mirror: true },
    { x: 1400, y: 0, mirror: false }, { x: 1730, y: 0, mirror: true },
    { x: 2800, y: 0, mirror: false }, { x: 3130, y: 0, mirror: true },
    { x: 0, y: 1000, mirror: false }, { x: 330, y: 1000, mirror: true },
    { x: 1400, y: 1000, mirror: false }, { x: 1730, y: 1000, mirror: true },
    { x: 2800, y: 1000, mirror: false }, { x: 3130, y: 1000, mirror: true },
  ],
};

// Position of LED i (0..101) along the leg, in leg coordinates.
window.crabLedPositions = function () {
  const pos = new Array(102);
  for (const seg of window.CRAB_GEOMETRY.LEG) {
    const pts = seg.path;
    const lens = [];
    let total = 0;
    for (let k = 1; k < pts.length; k++) {
      const l = Math.hypot(pts[k][0] - pts[k - 1][0], pts[k][1] - pts[k - 1][1]);
      lens.push(l);
      total += l;
    }
    for (let j = 0; j < seg.count; j++) {
      let d = ((j + 0.5) / seg.count) * total;
      let k = 0;
      while (k < lens.length - 1 && d > lens[k]) d -= lens[k++];
      const t = d / lens[k];
      pos[seg.first + j] = [pts[k][0] + (pts[k + 1][0] - pts[k][0]) * t, pts[k][1] + (pts[k + 1][1] - pts[k][1]) * t];
    }
  }
  return pos;
};
