import { clamp } from "../../utils/format.js";

export function drawSparkline(polylineEl, values){
  if (!polylineEl) return;
  if (!values || !values.length){
    polylineEl.setAttribute("points", "");
    return;
  }
  const vals = values.slice(-60);
  const points = vals.map((v,i)=>{
    const x = (i / (vals.length - 1 || 1)) * 100;
    const y = 30 - clamp(((v + 90) / 60) * 30, 0, 30);
    return `${x.toFixed(2)},${y.toFixed(2)}`;
  }).join(" ");
  polylineEl.setAttribute("points", points);
}
