// Phone control page served by the ESP32 (mode, brightness, speed, power budget).
#pragma once

static const char kIndexHtml[] = R"HTML(<!doctype html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Beast crab</title>
<style>
body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:16px;max-width:480px}
h1{font-size:20px;margin:0 0 12px}
#modes{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:16px}
#profiles{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px}
h2{font-size:15px;margin:20px 0 8px;color:#ccc}
button{padding:14px 8px;border:1px solid #444;border-radius:10px;background:#222;color:#eee;font-size:15px}
button.on{background:#e8590c;border-color:#e8590c;color:#fff}
label{display:block;margin:14px 0 4px;font-size:14px;color:#aaa}
input[type=range]{width:100%}
.meter{height:8px;background:#333;border-radius:4px;overflow:hidden}
.meter div{height:100%;background:#e8590c;width:0}
.stat{font-size:13px;color:#aaa;margin-top:12px}
</style></head><body>
<h1>Beast crab</h1>
<div id="modes"></div>
<label>Speed <span id="sv"></span></label><input id="s" type="range" min="10" max="400">
<h2>Power</h2>
<div id="profiles"></div>
<label>Brightness <span id="bv"></span></label><input id="b" type="range" min="0" max="255">
<label>Average limit <span id="av"></span></label><input id="a" type="range" min="1500" max="40000" step="500">
<label>Peak limit <span id="pv"></span></label><input id="p" type="range" min="1500" max="60000" step="500">
<label>Current now (of peak limit) <span id="dv"></span></label><div class="meter"><div id="draw"></div></div>
<label>Average, 10 s (of average limit) <span id="mv"></span></label><div class="meter"><div id="avg"></div></div>
<label>Burst reserve <span id="rv"></span></label><div class="meter"><div id="res"></div></div>
<label>Microphone</label><div class="meter"><div id="mic"></div></div>
<div class="stat" id="stat"></div>
<script>
const $=id=>document.getElementById(id);
let busy=false;
function set(k,v){fetch('/api/set?'+k+'='+v,{method:'POST'}).then(poll)}
function render(st){
  const m=$('modes');
  if(m.children.length!==st.modes.length){m.innerHTML='';st.modes.forEach((n,i)=>{const b=document.createElement('button');b.textContent=n;b.onclick=()=>set('mode',i);m.appendChild(b)})}
  [...m.children].forEach((b,i)=>b.className=i===st.mode?'on':'');
  const pr=$('profiles');
  if(pr.children.length!==st.profiles.length){pr.innerHTML='';st.profiles.forEach((n,i)=>{const b=document.createElement('button');b.textContent=n;b.onclick=()=>set('profile',i);pr.appendChild(b)})}
  [...pr.children].forEach((b,i)=>b.className=i===st.profile?'on':'');
  if(!busy){$('b').value=st.brightness;$('s').value=Math.round(st.speed*100);$('a').value=st.average_limit;$('p').value=st.peak_limit}
  const A=v=>(v/1000).toFixed(1)+' A';
  $('bv').textContent=Math.round(st.brightness/2.55)+'%';
  $('sv').textContent=st.speed.toFixed(2)+'x';
  $('av').textContent=A(st.average_limit);
  $('pv').textContent=A(st.peak_limit);
  $('dv').textContent=A(st.delivered);
  $('mv').textContent=A(st.average);
  $('rv').textContent=Math.round(st.reserve*100)+'%';
  $('draw').style.width=Math.min(100,st.delivered/st.peak_limit*100)+'%';
  $('avg').style.width=Math.min(100,st.average/st.average_limit*100)+'%';
  $('res').style.width=st.reserve*100+'%';
  $('mic').style.width=(st.audio.valid?st.audio.level*100:0)+'%';
  $('stat').textContent='Effect asks '+A(st.requested)+'; white trimmed '+Math.round(st.white_trim*100)+'%, dimmed to '+Math.round(st.scale*100)+'%'+(st.audio.valid?'':'. No microphone: sound mode runs at 120 BPM');
}
function poll(){fetch('/api/state').then(r=>r.json()).then(render).catch(()=>{})}
for(const [id,k,f] of [['b','brightness',v=>v],['s','speed',v=>v/100],['a','average',v=>v],['p','peak',v=>v]]){
  $(id).oninput=()=>{busy=true};
  $(id).onchange=e=>{busy=false;set(k,f(e.target.value))};
}
poll();setInterval(poll,500);
</script></body></html>)HTML";
