#include "ErrorLogger.h"
#include <WiFi.h>

// -------- RTC data (persist tussen resets) --------
RTC_DATA_ATTR uint32_t   ErrorLogger::_rtcLastUptimeSec = 0;
RTC_DATA_ATTR uint32_t   ErrorLogger::_bcIndex = 0;
RTC_DATA_ATTR ErrorLogger::Breadcrumb ErrorLogger::_bcRing[16] = {};

// -------- Global instance --------
ErrorLogger ErrorLogService;

// --------------------------------------------------
bool ErrorLogger::begin(const char* logPath, uint32_t checkpointSec) {
  _path = (logPath && *logPath) ? String(logPath) : String("/error.log");
  _checkpointSec = checkpointSec > 0 ? checkpointSec : 30;

  _ensureFS();

  // Probeer bekende task handles te pakken voor watermarks (best-effort)
  _loopTask = xTaskGetCurrentTaskHandle();           // aanroepen vanuit setup()
#if CONFIG_FREERTOS_UNICORE
  _idle0 = xTaskGetIdleTaskHandle();
  _idle1 = nullptr;
#else
  _idle0 = xTaskGetIdleTaskHandleForCPU(0);
  _idle1 = xTaskGetIdleTaskHandleForCPU(1);
#endif

  // Altijd: boot-samenvatting + extra contextregels
  logBootSummary();
  _dumpBootDetails();
  _dumpBreadcrumbs();
  _logTaskWatermarks();

  return true;
}

void ErrorLogger::loop() {
  const unsigned long now = millis();
  if (now - _lastTickMs >= (unsigned long)_checkpointSec * 1000UL) {
    _lastTickMs = now;
    _rtcLastUptimeSec = _currentUptimeSec();
  }
}

// --------------------------------------------------
void ErrorLogger::logBootSummary() {
  bool tv = false;
  const String ts = _nowIso8601(tv, 250);

  const esp_reset_reason_t r = esp_reset_reason();
  const String rStr = _resetReasonToStr(r);

  // Netwerk snapshot zoals je /info doet
  wifi_mode_t mode = WiFi.getMode();
  bool apOn  = mode & WIFI_MODE_AP;
  bool staOn = mode & WIFI_MODE_STA;
  bool staConnected = staOn && (WiFi.status() == WL_CONNECTED);

  String modeStr = staConnected ? (apOn ? "AP+STA" : "STA")
                                : (apOn ? "AP" : "OFF");

  String ip   = staConnected ? WiFi.localIP().toString()
                             : (apOn ? WiFi.softAPIP().toString() : String("0.0.0.0"));

  String ssid = staConnected ? WiFi.SSID()
                             : (apOn ? WiFi.softAPSSID() : String(""));

  const int32_t rssi = staConnected ? WiFi.RSSI() : 0;
  const uint32_t prevUptime = _rtcLastUptimeSec;

  String line;
  line.reserve(256);
  line += "[BOOT] ";
  line += (tv ? ts : String("time=unsynced"));
  line += " | reason=" + rStr + "(" + String((int)r) + ")";
  line += " | prev_uptime_s=" + String(prevUptime);
  line += " | mode=" + modeStr;
  line += " | ip=" + ip;
  if (ssid.length())        line += " | ssid=" + ssid;
  if (staConnected)         line += " | rssi_dbm=" + String(rssi);
  line += " | wakeup=" + String((int)esp_sleep_get_wakeup_cause());

  _appendLine(line);

  // Reset “vorige uptime” voor de nieuwe sessie
  _rtcLastUptimeSec = 0;
}

void ErrorLogger::logNetSnapshot(const char* label) {
  wifi_mode_t mode = WiFi.getMode();
  bool apOn  = mode & WIFI_MODE_AP;
  bool staOn = mode & WIFI_MODE_STA;
  bool staConnected = staOn && (WiFi.status() == WL_CONNECTED);

  String modeStr = staConnected ? (apOn ? "AP+STA" : "STA")
                                : (apOn ? "AP" : "OFF");

  String ip   = staConnected ? WiFi.localIP().toString()
                             : (apOn ? WiFi.softAPIP().toString() : String("0.0.0.0"));

  String ssid = staConnected ? WiFi.SSID()
                             : (apOn ? WiFi.softAPSSID() : String(""));

  int32_t rssi = staConnected ? WiFi.RSSI() : 0;

  bool tv=false; String ts=_nowIso8601(tv);
  String line = String("[") + (label?label:"NET") + "] " + (tv?ts:"time=unsynced") +
                " | mode=" + modeStr + " | ip=" + ip +
                (ssid.length() ? " | ssid=" + ssid : "") +
                (staConnected ? " | rssi_dbm=" + String(rssi) : "");
  _appendLine(line);
}

void ErrorLogger::logHeapSnapshot(const char* label) {
  bool tv=false; String ts=_nowIso8601(tv);
  size_t free8   = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  size_t min8    = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
  size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

  String line = String("[") + (label?label:"HEAP") + "] " + (tv?ts:"time=unsynced") +
                " | free=" + String(free8) +
                " | min="  + String(min8) +
                " | largest=" + String(largest);

#ifdef BOARD_HAS_PSRAM
  line += " | psram_free=" + String(ESP.getFreePsram());
#endif
  _appendLine(line);
}

void ErrorLogger::_dumpBootDetails() {
  bool tv=false; String ts=_nowIso8601(tv);
  String line = String("[DETAIL] ") + (tv?ts:"time=unsynced");

  line += " | cpu_mhz=" + String(getCpuFrequencyMhz());
  line += " | chip=" + String(ESP.getChipModel());
  line += " | rev="  + String(ESP.getChipRevision());
  line += " | sdk="  + String(ESP.getSdkVersion());
  line += " | flash=" + String(ESP.getFlashChipSize());

  size_t free8   = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  size_t min8    = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
  size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  line += " | heap_free=" + String(free8) +
          " | heap_min="  + String(min8) +
          " | heap_largest=" + String(largest);

#ifdef BOARD_HAS_PSRAM
  line += " | psram_total=" + String(ESP.getPsramSize()) +
          " | psram_free="  + String(ESP.getFreePsram());
#endif
  _appendLine(line);
}

void ErrorLogger::_logTaskWatermarks() {
#if (configUSE_TRACE_FACILITY == 1)
  bool tv=false; String ts=_nowIso8601(tv);
  if (_loopTask) {
    _appendLine(String("[TASK] ") + (tv?ts:"time=unsynced") +
                " | name=loop | hw=" + String(uxTaskGetStackHighWaterMark(_loopTask)));
  }
  if (_idle0) {
    _appendLine(String("[TASK] ") + (tv?ts:"time=unsynced") +
                " | name=IDLE0 | hw=" + String(uxTaskGetStackHighWaterMark(_idle0)));
  }
  if (_idle1) {
    _appendLine(String("[TASK] ") + (tv?ts:"time=unsynced") +
                " | name=IDLE1 | hw=" + String(uxTaskGetStackHighWaterMark(_idle1)));
  }
#endif
}

void ErrorLogger::_dumpBreadcrumbs() {
  // Dump ringbuffer in chronologische volgorde vanaf oudste item.
  const uint32_t N = sizeof(_bcRing)/sizeof(_bcRing[0]);
  if (N == 0) return;

  uint32_t start = (_bcIndex >= N) ? (_bcIndex - N) : 0;
  for (uint32_t i = start; i < _bcIndex; ++i) {
    const Breadcrumb& b = _bcRing[i % N];
    // Sla lege records (default 0) over
    if (b.ms == 0 && b.tag[0] == '\0') continue;

    bool tv=false; String ts=_nowIso8601(tv);
    String line = String("[BC] ") + (tv?ts:"time=unsynced") +
                  " | t+" + String(b.ms) + "ms" +
                  " | heap=" + String(b.free_heap) +
                  " | rssi=" + String((int)b.rssi) +
                  " | tag=" + String(b.tag);
    _appendLine(line);
  }
}

// --------------------------------------------------
void ErrorLogger::logInfo(const String& msg) {
  bool tv=false; String ts=_nowIso8601(tv);
  _appendLine(String("[INFO] ") + (tv?ts:"time=unsynced") + " | " + msg);
}
void ErrorLogger::logError(const String& msg) {
  bool tv=false; String ts=_nowIso8601(tv);
  _appendLine(String("[ERR ] ") + (tv?ts:"time=unsynced") + " | " + msg);
}

bool ErrorLogger::clear() {
  if (!_ensureFS()) return false;
  LittleFS.remove(_path);
  File f = LittleFS.open(_path, "w");
  if (!f) return false;
  f.println("# ESP32 Error log (append-only)");
  f.close();
  return true;
}

// --------------------------------------------------
void ErrorLogger::breadcrumb(const char* tag) {
  Breadcrumb b{};
  b.ms = millis();
  b.free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  b.rssi = (WiFi.getMode() & WIFI_MODE_STA) && (WiFi.status() == WL_CONNECTED) ? (int8_t)WiFi.RSSI() : 0;

  // Trim/Copy tag
  if (tag && *tag) {
    size_t n = strnlen(tag, sizeof(b.tag)-1);
    memcpy(b.tag, tag, n);
    b.tag[n] = '\0';
  } else {
    strcpy(b.tag, "-");
  }

  const uint32_t N = sizeof(_bcRing)/sizeof(_bcRing[0]);
  _bcRing[_bcIndex % N] = b;
  _bcIndex++;
}

// --------------------------------------------------
bool ErrorLogger::_ensureFS() {
  if (_fsReady) return true;
  if (LittleFS.begin(true)) {
    _fsReady = true;
    if (!LittleFS.exists(_path)) {
      File f = LittleFS.open(_path, "w");
      if (f) { f.println("# ESP32 Error log (append-only)"); f.close(); }
    }
  }
  return _fsReady;
}

void ErrorLogger::_appendLine(const String& line) {
  if (!_ensureFS()) return;
  File f = LittleFS.open(_path, "a");
  if (!f) return;
  f.println(line);
  f.close();
}

String ErrorLogger::_nowIso8601(bool& timeValid, uint32_t timeoutMs) const {
  // Gebruik TimeService zodat 1970 niet meer voorkomt zodra SNTP loopt.
  struct tm tm {};
  timeValid = TimeService.getTm(tm, timeoutMs);
  if (!timeValid) return String();
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
  return String(buf);
}

String ErrorLogger::_resetReasonToStr(esp_reset_reason_t r) const {
  switch (r) {
    case ESP_RST_POWERON:    return "POWERON";
    case ESP_RST_EXT:        return "EXT_RESET";
    case ESP_RST_SW:         return "SW_RESET";
    case ESP_RST_PANIC:      return "PANIC";
    case ESP_RST_INT_WDT:    return "INT_WDT";
    case ESP_RST_TASK_WDT:   return "TASK_WDT";
    case ESP_RST_WDT:        return "WDT";
    case ESP_RST_DEEPSLEEP:  return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:   return "BROWNOUT";
    case ESP_RST_SDIO:       return "SDIO";
    default:                 return "UNKNOWN";
  }
}

uint32_t ErrorLogger::_currentUptimeSec() const {
  return (uint32_t)(millis() / 1000UL);
}
