import { getJSON } from "./http.js";

export const fsInfo = () => getJSON("/fs/info");
export const fsList = (path = "/") =>
  getJSON(`/fs/list?path=${encodeURIComponent(path)}`);

export function fsDownloadUrl(path){
  return `/fs/download?path=${encodeURIComponent(path)}`;
}

export async function fsUpload(dir, file, overwrite = false){
  const fd = new FormData();
  fd.append("path", dir || "/");
  fd.append("overwrite", overwrite ? "1" : "0");
  fd.append("file", file, file.name);
  const r = await fetch("/fs/upload", { method: "POST", body: fd });
  if (!r.ok) throw new Error(`Upload failed (${r.status})`);
  return r.headers.get("content-type")?.includes("json") ? r.json() : r.text();
}

export async function fsRename(from, to){
  const body = new URLSearchParams({ from, to }).toString();
  const r = await fetch("/fs/rename", {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body
  });
  if (!r.ok) throw new Error(`Rename failed (${r.status})`);
  return r.headers.get("content-type")?.includes("json") ? r.json() : r.text();
}
