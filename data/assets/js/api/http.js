export async function getJSON(url, timeoutMs = 4000){
  const ctrl = new AbortController();
  const timer = setTimeout(() => ctrl.abort(), timeoutMs);
  const t0 = performance.now();
  try{
    const res = await fetch(url, { signal: ctrl.signal, cache: "no-store" });
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const data = await res.json();
    data._rt = Math.round(performance.now() - t0);
    return data;
  } finally {
    clearTimeout(timer);
  }
}
