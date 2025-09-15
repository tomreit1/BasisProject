export const clamp   = (n, a, b) => Math.min(b, Math.max(a, n));
export const fmtTime = () => new Date().toLocaleTimeString();

export function rssiToQuality(rssi){
  // -90..-30 dBm -> 0..100 %
  const q = ((rssi + 90) / 60) * 100;
  return clamp(Math.round(q), 0, 100);
}

export function fmtBytes(n){ return (typeof n==="number") ? n.toLocaleString() : "–"; }

export function fmtSec(s){
  if (typeof s!=="number") return "–";
  const h = Math.floor(s/3600), m = Math.floor((s%3600)/60), ss = s%60;
  return h>0 ? `${h}h ${m}m ${ss}s` : `${m}m ${ss}s`;
}
