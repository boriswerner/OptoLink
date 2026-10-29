#pragma once
#include <Arduino.h>

// Startseite der Weboberfläche: statisches HTML/CSS/JS ohne externe Abhängigkeiten.
// Daten kommen von /api/status (JSON) und /api/logs (Text), Neustart per POST /api/restart.
const char WEB_UI_PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>OptoLink</title>
<style>
:root{--bg:#f4f5f7;--card:#fff;--text:#1d2129;--muted:#6b7280;--line:#e3e5e8;--accent:#2563eb;
--ok:#15803d;--okbg:#dcfce7;--bad:#b91c1c;--badbg:#fee2e2;--warn:#a16207;--warnbg:#fef9c3}
@media (prefers-color-scheme:dark){:root{--bg:#111318;--card:#1b1e25;--text:#e6e8eb;--muted:#9aa1ad;
--line:#2a2e37;--accent:#60a5fa;--ok:#4ade80;--okbg:#14321f;--bad:#f87171;--badbg:#3b1717;--warn:#facc15;--warnbg:#3a3112}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);font:15px/1.4 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
header{position:sticky;top:0;z-index:1;background:var(--card);border-bottom:1px solid var(--line);padding:10px 16px}
.top{display:flex;align-items:center;gap:12px;flex-wrap:wrap;max-width:1200px;margin:0 auto}
h1{font-size:18px;margin:0;font-weight:650}
#clock{font-variant-numeric:tabular-nums;color:var(--muted)}
.spacer{flex:1}
.pill{padding:3px 10px;border-radius:999px;font-size:13px;font-weight:600;white-space:nowrap}
.pill.ok{color:var(--ok);background:var(--okbg)}.pill.bad{color:var(--bad);background:var(--badbg)}
.pill.unk{color:var(--muted);background:var(--bg)}
button{font:inherit;border:1px solid var(--line);background:var(--bg);color:var(--text);border-radius:8px;padding:6px 12px;cursor:pointer}
button:hover{border-color:var(--accent)}
.meta{max-width:1200px;margin:6px auto 0;display:flex;gap:16px;flex-wrap:wrap;font-size:13px;color:var(--muted)}
main{max-width:1200px;margin:0 auto;padding:16px}
h2{font-size:14px;text-transform:uppercase;letter-spacing:.04em;color:var(--muted);margin:22px 0 8px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(175px,1fr));gap:10px}
.card{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:10px 12px;min-width:0}
.card .n{font-size:13px;color:var(--muted);overflow-wrap:anywhere}
.card .v{font-size:22px;font-weight:600;font-variant-numeric:tabular-nums;margin:2px 0}
.card .u{font-size:14px;font-weight:400;color:var(--muted);margin-left:3px}
.card .t{font-size:12px;color:var(--muted)}
.card.stale{border-color:var(--warn);background:var(--warnbg)}
.card.stale .t{color:var(--warn);font-weight:600}
.err{display:grid;gap:10px}
.err .card .v{font-size:16px}
details{margin-top:22px;background:var(--card);border:1px solid var(--line);border-radius:10px}
summary{cursor:pointer;padding:10px 12px;font-weight:600}
pre{margin:0;padding:10px 12px;max-height:420px;overflow:auto;font:12px/1.45 ui-monospace,Consolas,monospace;border-top:1px solid var(--line);white-space:pre-wrap;overflow-wrap:anywhere}
#banner{display:none;max-width:1200px;margin:10px auto 0;padding:8px 12px;border-radius:8px;background:var(--warnbg);color:var(--warn);font-weight:600}
</style>
</head>
<body>
<header>
<div class="top">
<h1>OptoLink</h1><span id="clock">–</span>
<span class="spacer"></span>
<span id="errpill" class="pill unk">Heizung: –</span>
<button id="restart" type="button">Neustart</button>
</div>
<div class="meta">
<span>Letzte Abfrage: <b id="last">–</b></span>
<span>Nächste: <b id="next">–</b></span>
<span>MQTT: <b id="mqtt">–</b></span>
<span>WLAN: <b id="rssi">–</b></span>
<span>Freier Heap: <b id="heap">–</b></span>
</div>
<div id="banner"></div>
</header>
<main>
<div id="groups"></div>
<details id="logbox"><summary>Log</summary><pre id="log">wird geladen …</pre></details>
</main>
<script>
const GROUPS=[["temperature","Temperaturen"],["state","Zustände & Betriebsarten"],["counter","Zähler"],["error","Störmeldungen"]];
const UNITS={temperature:"°C",hours:"Std."};
let offset=0,interval=60,timer=null,nextAt=0;
const $=id=>document.getElementById(id);
const valid=t=>t>1600000000;
function fmt(t,withDate){if(!valid(t))return"–";const d=new Date(t*1000),o={hour:"2-digit",minute:"2-digit",second:"2-digit"};
if(withDate){o.day="2-digit";o.month="2-digit"}return d.toLocaleString("de-DE",o)}
function esc(s){return String(s).replace(/[&<>"]/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"}[c]))}
function banner(text){const b=$("banner");b.textContent=text||"";b.style.display=text?"block":"none"}
function tick(){const now=Date.now()+offset;$("clock").textContent=offset?fmt(now/1000,true):"–";
if(nextAt){const s=Math.max(0,Math.round((nextAt-Date.now())/1000));$("next").textContent="in "+s+" s"}}
function card(v,now){const stale=!v.t||(valid(now)&&now-v.t>2*interval);
const unit=UNITS[v.g]?'<span class="u">'+UNITS[v.g]+'</span>':"";
let val=v.v===""?"–":esc(v.v)+unit,sub="";
if(v.g==="error"&&v.v){const p=v.v.split("#");val=esc(p[1]||v.v);sub=esc(p[0]||"")+(p[2]?" · Code "+esc(p[2]):"")+"<br>"}
return '<div class="card'+(stale?" stale":"")+'"><div class="n">'+esc(v.n)+'</div><div class="v">'+val+'</div><div class="t">'+sub+
(v.t?"aktualisiert "+fmt(v.t,stale):"noch nicht gelesen")+'</div></div>'}
function render(d){interval=d.interval;offset=valid(d.time)?d.time*1000-Date.now():0;
const e=$("errpill");e.className="pill "+(d.error===1?"bad":d.error===0?"ok":"unk");
e.textContent=d.error===1?"Heizung antwortet nicht":d.error===0?"Heizung verbunden":"Heizung: noch kein Zyklus";
$("last").textContent=fmt(d.lastCycle,false);$("mqtt").textContent=d.mqtt?"verbunden":"getrennt";
$("rssi").textContent=d.rssi+" dBm";$("heap").textContent=(d.heap/1024).toFixed(1)+" KB";
let html="";for(const[g,label]of GROUPS){const items=d.values.filter(v=>v.g===g||(g==="counter"&&v.g==="hours"));
if(!items.length)continue;html+="<h2>"+label+'</h2><div class="'+(g==="error"?"err":"grid")+'">'+items.map(v=>card(v,d.time)).join("")+"</div>"}
$("groups").innerHTML=html}
async function loadLogs(){try{const r=await fetch("/api/logs");const p=$("log");p.textContent=await r.text();p.scrollTop=p.scrollHeight}catch(e){}}
async function load(){clearTimeout(timer);let wait=10000;
try{const r=await fetch("/api/status",{cache:"no-store"});if(!r.ok)throw 0;const d=await r.json();render(d);banner("");
wait=Math.max(d.nextCycleIn+3000,5000);nextAt=Date.now()+Math.max(d.nextCycleIn,0);
if($("logbox").open)loadLogs()}catch(e){banner("Board nicht erreichbar – neuer Versuch in 10 s");nextAt=0}
timer=setTimeout(load,wait)}
$("logbox").addEventListener("toggle",()=>{if($("logbox").open)loadLogs()});
$("restart").addEventListener("click",async()=>{if(!confirm("Board wirklich neu starten?"))return;
try{await fetch("/api/restart",{method:"POST"})}catch(e){}banner("Neustart läuft … Seite lädt in 20 s neu");
clearTimeout(timer);nextAt=0;timer=setTimeout(load,20000)});
setInterval(tick,1000);load();
</script>
</body>
</html>
)rawliteral";
