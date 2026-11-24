// Draws the frames produced by the WebAssembly build of firmware/lib/crabfx.
(async function () {
  const $ = (id) => document.getElementById(id);
  if (typeof createCrabFx !== "function") {
    $("err").textContent = "build/crabfx.js is missing: run simulator/build.sh first.";
    return;
  }
  const M = await createCrabFx();
  const api = {
    init: M.cwrap("sim_init", null, ["number"]),
    legs: M.cwrap("sim_legs", "number", []),
    perLeg: M.cwrap("sim_pixels_per_leg", "number", []),
    modeCount: M.cwrap("sim_mode_count", "number", []),
    modeName: M.cwrap("sim_mode_name", "string", ["number"]),
    setMode: M.cwrap("sim_set_mode", null, ["number", "number"]),
    setSpeed: M.cwrap("sim_set_speed", null, ["number"]),
    setBrightness: M.cwrap("sim_set_brightness", null, ["number"]),
    profileCount: M.cwrap("sim_profile_count", "number", []),
    profileName: M.cwrap("sim_profile_name", "string", ["number"]),
    setProfile: M.cwrap("sim_set_profile", null, ["number"]),
    brightness: M.cwrap("sim_brightness", "number", []),
    averageLimit: M.cwrap("sim_average_limit", "number", []),
    peakLimit: M.cwrap("sim_peak_limit", "number", []),
    setLimits: M.cwrap("sim_set_limits", null, ["number", "number"]),
    averageMa: M.cwrap("sim_average_ma", "number", []),
    reserve: M.cwrap("sim_reserve", "number", []),
    scale: M.cwrap("sim_scale", "number", []),
    whiteTrim: M.cwrap("sim_white_trim", "number", []),
    audioBuffer: M.cwrap("sim_audio_buffer", "number", []),
    audioBlock: M.cwrap("sim_audio_block", "number", []),
    pushAudio: M.cwrap("sim_push_audio", null, ["number"]),
    tick: M.cwrap("sim_tick", "number", ["number"]),
    requested: M.cwrap("sim_requested_ma", "number", []),
    delivered: M.cwrap("sim_delivered_ma", "number", []),
    audioLevel: M.cwrap("sim_audio_level", "number", []),
  };
  api.init(Math.floor(Math.random() * 1e9) + 1);
  const LEGS = api.legs(), PER_LEG = api.perLeg();
  const t0 = performance.now();
  const now = () => Math.floor(performance.now() - t0);

  // ---- geometry -> canvas coordinates
  const G = window.CRAB_GEOMETRY;
  const local = window.crabLedPositions();
  const place = (leg, [x, y]) => [leg.x + (leg.mirror ? -x : x), leg.y + y];
  const leds = [];
  let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
  for (let l = 0; l < LEGS; l++)
    for (let i = 0; i < PER_LEG; i++) {
      const p = place(G.LEGS[l], local[i]);
      leds.push(p);
      minX = Math.min(minX, p[0]); maxX = Math.max(maxX, p[0]);
      minY = Math.min(minY, p[1]); maxY = Math.max(maxY, p[1]);
    }
  const pad = 60;
  const canvas = $("view"), ctx = canvas.getContext("2d");
  let scale = 1;
  function resize() {
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth;
    const worldW = maxX - minX + 2 * pad, worldH = maxY - minY + 2 * pad;
    canvas.style.height = (w * worldH / worldW) + "px";
    canvas.width = Math.round(w * dpr);
    canvas.height = Math.round(w * worldH / worldW * dpr);
    scale = canvas.width / worldW;
  }
  window.addEventListener("resize", resize);
  resize();
  const sx = (x) => (x - minX + pad) * scale, sy = (y) => (y - minY + pad) * scale;

  // ---- controls
  const modes = $("modes");
  for (let i = 0; i < api.modeCount(); i++) {
    const b = document.createElement("button");
    b.textContent = api.modeName(i);
    b.onclick = () => { api.setMode(i, now()); [...modes.children].forEach((c, j) => c.className = j === i ? "on" : ""); };
    modes.appendChild(b);
  }
  modes.children[0].click();
  const A = (ma) => (ma / 1000).toFixed(1) + " A";
  const sliders = {
    b: { label: "bv", fmt: (v) => Math.round(v / 2.55) + "%", set: (v) => api.setBrightness(v), get: () => api.brightness() },
    s: { label: "sv", fmt: (v) => (v / 100).toFixed(2) + "×", set: (v) => api.setSpeed(v / 100), get: null },
    a: { label: "av", fmt: A, set: (v) => api.setLimits(v, api.peakLimit()), get: () => api.averageLimit() },
    p: { label: "pv", fmt: A, set: (v) => api.setLimits(api.averageLimit(), v), get: () => api.peakLimit() },
  };
  const profiles = $("profiles");
  const markProfile = (k) => [...profiles.children].forEach((c, j) => c.className = j === k ? "on" : "");
  const syncSliders = () => {
    for (const [id, sl] of Object.entries(sliders)) {
      if (sl.get) $(id).value = sl.get();
      $(sl.label).textContent = sl.fmt(Number($(id).value));
    }
  };
  for (const [id, sl] of Object.entries(sliders))
    $(id).oninput = () => { const v = Number($(id).value); sl.set(v); $(sl.label).textContent = sl.fmt(v); markProfile(-1); };
  for (let i = 0; i < api.profileCount(); i++) {
    const b = document.createElement("button");
    b.textContent = api.profileName(i);
    b.onclick = () => { api.setProfile(i); markProfile(i); syncSliders(); };
    profiles.appendChild(b);
  }
  profiles.children[0].click();
  sliders.s.set(100);

  // ---- optional microphone, analysed by the same C++ code as on the ESP32
  $("micbtn").onclick = async () => {
    try {
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
      const ac = new AudioContext({ sampleRate: 16000 });
      const src = ac.createMediaStreamSource(stream);
      const N = api.audioBlock();
      const proc = ac.createScriptProcessor(N, 1, 1);
      proc.onaudioprocess = (e) => {
        new Float32Array(M.HEAPF32.buffer, api.audioBuffer(), N).set(e.inputBuffer.getChannelData(0));
        api.pushAudio(now());
      };
      src.connect(proc);
      proc.connect(ac.destination);
      $("micbtn").disabled = true;
      $("micv").textContent = "on";
    } catch (err) {
      $("micv").textContent = "unavailable";
    }
  };

  // ---- render loop
  // The engine returns gamma-corrected PWM values, as sent to the strips. A screen
  // applies its own gamma, so undo ours to show roughly what the eye sees.
  const toScreen = new Uint8Array(256);
  for (let v = 0; v < 256; v++) toScreen[v] = Math.round(255 * Math.pow(v / 255, 1 / 2.2));

  function drawFrame(rgb) {
    ctx.globalCompositeOperation = "source-over";
    ctx.fillStyle = "#000";
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.strokeStyle = "rgba(255,255,255,0.09)";
    ctx.lineWidth = Math.max(1, 10 * scale);
    ctx.lineCap = "round";
    for (const leg of G.LEGS)
      for (const seg of G.FRAME.concat(G.LEG.map((s) => s.path))) {
        ctx.beginPath();
        seg.forEach((p, k) => { const [x, y] = place(leg, p); k ? ctx.lineTo(sx(x), sy(y)) : ctx.moveTo(sx(x), sy(y)); });
        ctx.stroke();
      }
    ctx.globalCompositeOperation = "lighter";
    const rGlow = Math.max(2, 22 * scale), rCore = Math.max(1, 7 * scale);
    for (let k = 0; k < leds.length; k++) {
      const r = toScreen[rgb[3 * k]], g = toScreen[rgb[3 * k + 1]], b = toScreen[rgb[3 * k + 2]];
      if (r + g + b < 6) continue;
      const x = sx(leds[k][0]), y = sy(leds[k][1]);
      ctx.fillStyle = `rgba(${r},${g},${b},0.18)`;
      ctx.beginPath(); ctx.arc(x, y, rGlow, 0, 6.2832); ctx.fill();
      ctx.fillStyle = `rgb(${r},${g},${b})`;
      ctx.beginPath(); ctx.arc(x, y, rCore, 0, 6.2832); ctx.fill();
    }
  }

  let frames = 0, lastStat = performance.now(), renderMs = 0;
  function loop() {
    const a = performance.now();
    const ptr = api.tick(now());
    drawFrame(new Uint8Array(M.HEAPU8.buffer, ptr, LEGS * PER_LEG * 3));
    renderMs += performance.now() - a;
    frames++;
    const delivered = api.delivered(), requested = api.requested(), average = api.averageMa();
    $("draw").style.width = Math.min(100, (delivered / api.peakLimit()) * 100) + "%";
    $("avg").style.width = Math.min(100, (average / api.averageLimit()) * 100) + "%";
    $("res").style.width = api.reserve() * 100 + "%";
    $("mic").style.width = api.audioLevel() * 100 + "%";
    if (performance.now() - lastStat > 500) {
      $("dv").textContent = A(delivered);
      $("mv").textContent = A(average);
      $("rv").textContent = Math.round(api.reserve() * 100) + "%";
      $("stat").textContent = `Effect asks ${A(requested)}; white trimmed ${Math.round(api.whiteTrim() * 100)}%, ` +
        `dimmed to ${Math.round(api.scale() * 100)}% · ${Math.round(frames * 1000 / (performance.now() - lastStat))} fps, ` +
        `${(renderMs / frames).toFixed(1)} ms/frame`;
      frames = 0; renderMs = 0; lastStat = performance.now();
    }
    requestAnimationFrame(loop);
  }
  requestAnimationFrame(loop);
})();
