import { getJSON } from "./http.js";

export const getInfo    = () => getJSON("/info", 4000);
export const ping       = () => getJSON("/health", 3000);
export const sysInfo    = () => getJSON("/sys/info", 4000);
export const sysActive  = () => getJSON("/sys/active", 4000);
