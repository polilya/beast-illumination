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
    setBudget: M.cwrap("sim_set_budget", null, ["number"]),
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
  const bind = (id, label, fmt, set) => {
    const el = $(id);
    const apply = () => { const v = Number(el.value); $(label).textContent = fmt(v); set(v); };
    el.oninput = apply;
    apply();
  };
  bind("b", "bv", (v) => Math.round(v / 2.55) + "%", api.setBrightness);
  bind("s", "sv", (v) => (v / 100).toFixed(2) + "×", (v) => api.setSpeed(v / 100));
  let budget = 20000;
  bind("p", "pv", (v) => (v / 1000).toFixed(1) + " A", (v) => { budget = v; api.setBudget(v); });

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
      $("mv").textContent = "on";
    } catch (err) {
      $("mv").textContent = "unavailable";
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
    const delivered = api.delivered(), requested = api.requested();
    $("draw").style.width = Math.min(100, (delivered / budget) * 100) + "%";
    $("mic").style.width = api.audioLevel() * 100 + "%";
    if (performance.now() - lastStat > 500) {
      $("stat").textContent = `Requested ${(requested / 1000).toFixed(1)} A, delivered ${(delivered / 1000).toFixed(1)} A · ` +
        `${Math.round(frames * 1000 / (performance.now() - lastStat))} fps, ${(renderMs / frames).toFixed(1)} ms/frame`;
      frames = 0; renderMs = 0; lastStat = performance.now();
    }
    requestAnimationFrame(loop);
  }
  requestAnimationFrame(loop);
})();
