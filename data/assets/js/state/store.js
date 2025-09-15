import { RSSI_HISTORY_MAX } from "../config.js";

const store = {
  rssi: [],
  pushRssi(v){ this.rssi.push(v); if (this.rssi.length > RSSI_HISTORY_MAX) this.rssi.shift(); },
  clearRssi(){ this.rssi = []; },
  rssiHistory(){ return this.rssi.slice(); }
};

export default store;
