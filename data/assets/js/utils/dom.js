export const byId   = (id) => document.getElementById(id);
export const $      = (sel, root=document) => root.querySelector(sel);
export const $$     = (sel, root=document) => Array.from(root.querySelectorAll(sel));
export const setText  = (node, value) => { if (node) node.textContent = value; };
export const setWidth = (node, pct)   => { if (node) node.style.width = pct; };
export const setDot   = (node, ok) => {
  if (!node) return;
  node.classList.toggle("ok", !!ok);
  node.classList.toggle("bad", !ok);
};
export const on = (node, ev, fn) => { if (node) node.addEventListener(ev, fn); };
