import { rssiToQuality } from "../../utils/format.js";
import { setWidth } from "../../utils/dom.js";

export function applySignalQuality(barEl, textEl, rssi){
  if (typeof rssi !== "number"){
    setWidth(barEl, "0%");
    if (textEl) textEl.textContent = "–%";
    return 0;
  }
  const q = rssiToQuality(rssi);
  setWidth(barEl, `${q}%`);
  if (textEl) textEl.textContent = `${q}%`;
  return q;
}
