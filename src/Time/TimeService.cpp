#include "TimeService.h"
#include <time.h>
#include <Wifihandler/Wifihandler.h>   // to catch Wi-Fi connect events

TimeServiceClass TimeService;

void TimeServiceClass::begin(const char* tz,
                             const char* srv1,
                             const char* srv2,
                             const char* srv3,
                             int fastTries,
                             int fastDelay,
                             bool attachWiFiCallback) {
  // Cache settings for later re-apply (forceSync / Wi-Fi reconnect)
  _tz = (tz && *tz) ? tz : "CET-1CEST,M3.5.0,M10.5.0/3";
  _s1 = (srv1 && *srv1) ? srv1 : "pool.ntp.org";
  _s2 = (srv2 && *srv2) ? srv2 : "time.google.com";
  _s3 = (srv3 && *srv3) ? srv3 : "time.cloudflare.com";

  // Apply TZ before SNTP start (POSIX format, handles DST automatically)
  setenv("TZ", _tz.c_str(), 1);
  tzset();

  _applyConfig();

  // Optional: attach a Wi-Fi connection-change hook to re-try sync
  if (attachWiFiCallback && !_wifiCallbackAttached) {
    WiFiService.onConnectionChange([this](bool connected){
      if (connected) {
        // Re-apply configuration to nudge SNTP after a reconnect
        _applyConfig();
        // Quick probe to flip _synced fast if servers respond quickly
        struct tm probe {};
        if (getLocalTime(&probe, 100)) _updateSyncedFlag(true);
      }
    });
    _wifiCallbackAttached = true;
  }

  // Quick warm-up: poll getLocalTime a few times to determine validity early
  struct tm tm {};
  bool valid = false;
  const int tries = max(0, fastTries);
  for (int i = 0; i < tries; ++i) {
    if (getLocalTime(&tm, 50)) {
      const int year = tm.tm_year + 1900;
      if (year >= 2020 && year <= 2099) { valid = true; break; }
    }
    delay(max(0, fastDelay));
  }
  _updateSyncedFlag(valid);
}

void TimeServiceClass::loop() {
  // Currently nothing to do. Kept for future periodic health checks if desired.
}

bool TimeServiceClass::isValid() const {
  struct tm tm {};
  if (!getLocalTime(&tm, 50)) return false;
  const int year = tm.tm_year + 1900;
  return (year >= 2020 && year <= 2099);
}

time_t TimeServiceClass::now() const {
  if (!isValid()) return 0;
  return time(nullptr);
}

bool TimeServiceClass::getTm(struct tm& out, uint32_t timeoutMs) const {
  if (!getLocalTime(&out, timeoutMs)) return false;
  const int year = out.tm_year + 1900;
  return (year >= 2020 && year <= 2099);
}

String TimeServiceClass::nowIso8601(uint32_t timeoutMs) const {
  struct tm tm {};
  if (!getTm(tm, timeoutMs)) return String();  // not valid yet
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
  return String(buf);
}

void TimeServiceClass::forceSync() {
  // Re-apply SNTP with cached settings (idempotent)
  _applyConfig();

  // Probe once; if valid → flip flag and trigger callback
  struct tm tm {};
  if (getLocalTime(&tm, 200)) {
    const int year = tm.tm_year + 1900;
    _updateSyncedFlag(year >= 2020 && year <= 2099);
  }
}

void TimeServiceClass::onSynced(std::function<void(void)> cb) {
  _onSynced = std::move(cb);
  // If time is already valid, fire immediately (edge-or-now semantics)
  if (isValid() && _onSynced) _onSynced();
}

void TimeServiceClass::_applyConfig() {
  // configTzTime starts SNTP (or reconfigures it) with TZ + server list.
  configTzTime(_tz.c_str(), _s1.c_str(), _s2.c_str(), _s3.c_str());
}

void TimeServiceClass::_updateSyncedFlag(bool newValid) {
  const bool was = _synced;
  _synced = newValid;
  if (!was && _synced && _onSynced) {
    // Fire exactly once on the rising edge
    _onSynced();
  }
}
