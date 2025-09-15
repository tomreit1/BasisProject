import { byId, setText, setWidth, setDot, on } from "../../utils/dom.js";
import { fmtTime, fmtBytes, fmtSec } from "../../utils/format.js";
import * as sys from "../../api/sys.js";
import store from "../../state/store.js";
import { drawSparkline } from "../components/sparkline.js";
import { applySignalQuality } from "../components/meter.js";
import { AUTO_REFRESH_MS } from "../../config.js";

export function initDashboard(){
  console.log("[app] main loaded");

  // ------- refs -------
  const el = {
    hostname: byId("hostname"),
    ip: byId("ip"),
    ssid: byId("ssid"),
    rssiDbm: byId("rssiDbm"),
    rssiBar: byId("rssiBar"),
    rssiQ: byId("rssiQualityText"),
    modeTag: byId("modeTag"),

    dot: byId("dot"),
    healthText: byId("healthText"),
    latency: byId("latency"),
    lastCheck: byId("lastCheck"),
    btnPing: byId("btnPing"),

    sparkLine: byId("sparkLine"),
    rssiMin: byId("rssiMin"),
    rssiMax: byId("rssiMax"),

    autoRefresh: byId("autoRefresh"),
    year: byId("year"),
    lastUpdated: byId("lastUpdated"),
    toggleTheme: byId("toggle-theme"),

    // System card
    sysUptime: byId("sysUptime"),
    sysHeap: byId("sysHeap"),
    cpu0: byId("cpu0"),
    cpu1: byId("cpu1"),
    cpu0bar: byId("cpu0bar"),
    cpu1bar: byId("cpu1bar"),
    taskTbody: byId("taskTbody"),
  };

  // ------- init -------
  setText(el.hostname, location.hostname || "esp32.local");
  setText(el.year, new Date().getFullYear());

  try{
    const savedTheme = localStorage.getItem("theme");
    if (savedTheme){
      document.documentElement.setAttribute("data-theme", savedTheme);
      if (el.toggleTheme) el.toggleTheme.checked = savedTheme === "dark";
    }
    on(el.toggleTheme, "change", () => {
      const theme = el.toggleTheme.checked ? "dark" : "light";
      document.documentElement.setAttribute("data-theme", theme);
      localStorage.setItem("theme", theme);
    });
  }catch{}

  // ------- logic -------
  function updateSparkMeta(){
    const hist = store.rssiHistory().slice(-60);
    if (!hist.length){
      setText(el.rssiMin, "–"); setText(el.rssiMax, "–");
    } else {
      const min = Math.min(...hist), max = Math.max(...hist);
      setText(el.rssiMin, String(min)); setText(el.rssiMax, String(max));
    }
    drawSparkline(el.sparkLine, store.rssiHistory());
  }

  async function updateInfo(){
    try{
      const info = await sys.getInfo();
      const { ip, ssid, rssi, mode } = info;

      setText(el.ip, ip || "–");
      setText(el.ssid, ssid || "–");
      setText(el.rssiDbm, (typeof rssi === "number" ? rssi : "–"));

      if (el.modeTag){
        if (mode){ setText(el.modeTag, mode); el.modeTag.classList.remove("hidden"); }
        else { el.modeTag.classList.add("hidden"); }
      }

      if (typeof rssi === "number"){
        applySignalQuality(el.rssiBar, el.rssiQ, rssi);
        store.pushRssi(rssi);
      } else {
        applySignalQuality(el.rssiBar, el.rssiQ, undefined);
        store.clearRssi();
      }

      updateSparkMeta();
      setText(el.lastUpdated, `Last update: ${fmtTime()}`);
    }catch(e){
      applySignalQuality(el.rssiBar, el.rssiQ, undefined);
      store.clearRssi();
      updateSparkMeta();
      setText(el.lastUpdated, `Last update: ${fmtTime()} (error)`);
      console.warn("[app] /info failed:", e?.message || e);
    }
  }

  async function updateHealth(){
    try{
      const res = await sys.ping();
      setDot(el.dot, true);
      setText(el.healthText, "OK");
      setText(el.latency, `${res._rt} ms`);
      setText(el.lastCheck, fmtTime());
    }catch(e){
      setDot(el.dot, false);
      setText(el.healthText, "Unavailable");
      setText(el.latency, "–");
      setText(el.lastCheck, fmtTime());
      console.warn("[app] /health failed:", e?.message || e);
    }
  }

  async function refreshSystem(){
    try{
      const info = await sys.sysInfo();
      setText(el.sysUptime, fmtSec(info.uptime_s));
      setText(el.sysHeap,   fmtBytes(info.heap_free));

      const c0 = Number(info.cpu_load?.[0] ?? 0);
      const c1 = Number(info.cpu_load?.[1] ?? 0);
      setText(el.cpu0, c0.toFixed(1));
      setText(el.cpu1, c1.toFixed(1));
      setWidth(el.cpu0bar, `${Math.min(100, Math.max(0, c0))}%`);
      setWidth(el.cpu1bar, `${Math.min(100, Math.max(0, c1))}%`);
    }catch{}

    try{
      const act = await sys.sysActive();
      if (!el.taskTbody) return;
      if (!act.tasks || !act.tasks.length){
        el.taskTbody.innerHTML = `<tr><td colspan="5" class="muted">No data</td></tr>`;
        return;
      }
      el.taskTbody.innerHTML = act.tasks.map(t=>{
        const core = t.core===0?'0': t.core===1?'1':'?';
        return `<tr>
          <td class="mono">${t.name}</td>
          <td class="mono center">${core}</td>
          <td class="mono right">${t.prio}</td>
          <td class="mono right">${t.stack_min}</td>
          <td class="mono right">${t.share.toFixed(2)}</td>
        </tr>`;
      }).join("");
    }catch{}
  }

  // ------- auto loop -------
  let timer = null;
  function startAuto(){
    if (timer) return;
    updateInfo(); updateHealth(); refreshSystem();
    timer = setInterval(()=>{ updateInfo(); updateHealth(); refreshSystem(); }, AUTO_REFRESH_MS);
  }
  function stopAuto(){
    if (!timer) return;
    clearInterval(timer); timer = null;
  }

  // ------- bindings -------
  on(el.btnPing, "click", (e)=>{ e.preventDefault(); updateHealth(); });
  on(el.autoRefresh, "change", (e)=> e.target.checked ? startAuto() : stopAuto());

  // Debug hooks (handig tijdens testen)
  window.__ping = updateHealth;
  window.__info = updateInfo;
  window.__sys  = refreshSystem;

  // go
  if (!el.autoRefresh || el.autoRefresh.checked) startAuto();
}
