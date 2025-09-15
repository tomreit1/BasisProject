import { byId, setText, on } from "../../utils/dom.js";
import { fmtTime } from "../../utils/format.js";
import * as fs from "../../api/fs.js";

function getQuery(key, fallback = ""){
  const u = new URL(location.href);
  return u.searchParams.get(key) ?? fallback;
}

function rowHtml(entry, basePath){
  const isDir = !!entry.dir;
  const name  = entry.name;
  const pfx   = basePath.endsWith("/") ? basePath : basePath + "/";
  const full  = pfx + name;
  const nav   = isDir ? `<a class="btn btn-small" href="/pages/files.html?path=${encodeURIComponent(full)}">Open</a>` : "";
  const dl    = !isDir ? `<a class="btn btn-small" href="${fs.fsDownloadUrl(full)}">Download</a>` : "";
  return `<tr>
    <td class="mono">${name}</td>
    <td class="mono">${isDir ? "—" : (entry.size || 0)}</td>
    <td>${isDir ? "yes" : "no"}</td>
    <td class="actions">${nav}${dl}</td>
  </tr>`;
}

export function initFilesPage(){
  const el = {
    menuBtn: byId("menuBtn"),
    drawer:  byId("drawer"),
    hostname: byId("hostname"),
    year: byId("year"),

    fsTotal: byId("fsTotal"),
    fsUsed:  byId("fsUsed"),
    path:    byId("path"),
    tbody:   byId("tbody"),

    uploadForm: byId("uploadForm"),
    fileInput:  byId("fileInput"),
    overwrite:  byId("overwrite"),

    renameForm: byId("renameForm"),
    fromName:   byId("fromName"),
    toName:     byId("toName"),

    lastUpdated: byId("lastUpdated"),
  };

  setText(el.hostname, location.hostname || "esp32.local");
  setText(el.year, new Date().getFullYear());

  // Drawer toggle (klein & lokaal)
  on(el.menuBtn, "click", () => el.drawer?.classList.toggle("open"));
  document.addEventListener("keydown", e => { if (e.key === "Escape") el.drawer?.classList.remove("open"); });

  const state = { path: getQuery("path", "/") };

  async function refreshInfo(){
    try{
      const info = await fs.fsInfo();
      setText(el.fsTotal, (info.total ?? "–").toLocaleString?.() ?? String(info.total ?? "–"));
      setText(el.fsUsed,  (info.used  ?? "–").toLocaleString?.() ?? String(info.used  ?? "–"));
    }catch{
      setText(el.fsTotal, "–"); setText(el.fsUsed, "–");
    }
  }

  async function refreshList(){
    try{
      const data = await fs.fsList(state.path);
      setText(el.path, data.path);
      el.tbody.innerHTML = (data.entries?.length
        ? data.entries.map(e => rowHtml(e, data.path)).join("")
        : `<tr><td colspan="4" class="muted">Empty</td></tr>`);
      setText(el.lastUpdated, `Last update: ${fmtTime()}`);
    }catch(e){
      el.tbody.innerHTML = `<tr><td colspan="4" class="muted">Error loading list</td></tr>`;
      setText(el.lastUpdated, `Last update: ${fmtTime()} (error)`);
      console.warn("[files] /fs/list failed:", e?.message || e);
    }
  }

  // Upload
  on(el.uploadForm, "submit", async (e)=>{
    e.preventDefault();
    const file = el.fileInput?.files?.[0];
    if (!file) return;
    try{
      await fs.fsUpload(state.path, file, !!el.overwrite?.checked);
      await refreshInfo(); await refreshList();
      el.fileInput.value = "";
    }catch(err){
      alert(err?.message || err);
    }
  });

  // Rename (no overwrite)
  on(el.renameForm, "submit", async (e)=>{
    e.preventDefault();
    const fromN = el.fromName.value.trim();
    const toN   = el.toName.value.trim();
    if (!fromN || !toN) return;
    const pfx = state.path.endsWith("/") ? state.path : state.path + "/";
    try{
      await fs.fsRename(pfx + fromN, pfx + toN);
      await refreshList();
      el.fromName.value = ""; el.toName.value = "";
    }catch(err){
      alert(err?.message || err);
    }
  });

  // go
  (async function main(){
    await refreshInfo();
    await refreshList();
  })();
}
