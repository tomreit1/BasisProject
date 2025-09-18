import { byId, setText, setWidth, setDot, on } from "../../utils/dom.js";

export function initIndex(){
  const host = $("hostname");
  const year = document.getElementById("year");
  if (host) setText(host, location.hostname || "esp32.local");
  if (year) setText(year, new Date().getFullYear());

  // drawer toggle (basic)
  const menuBtn = document.getElementById("menuBtn");
  const drawer  = document.getElementById("drawer");
  menuBtn?.addEventListener("click", () => drawer?.classList.toggle("open"));
  document.addEventListener("keydown", e => { if (e.key === "Escape") drawer?.classList.remove("open"); });
  drawer?.addEventListener("click", e => { if (e.target.tagName === "A") drawer?.classList.remove("open"); });
}
