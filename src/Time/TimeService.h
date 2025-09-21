#pragma once
/**
 * TimeService
 * -----------
 * Lightweight singleton to manage wall-clock time on ESP32:
 * - Initializes SNTP with a given time zone and NTP server list.
 * - Exposes "isValid()" to check if wall-clock time is synced.
 * - Provides helpers to obtain ISO-8601 timestamps and time_t/struct tm.
 * - Auto re-tries sync whenever Wi-Fi becomes connected (via WiFiService callback).
 *
 * Usage:
 *   TimeService.begin();                 // set TZ + start SNTP + quick warm-up
 *   if (TimeService.isValid()) { ... }   // check if time is valid
 *   String ts = TimeService.nowIso8601(); // "2025-09-19T10:21:07" or "" if not valid
 *   TimeService.onSynced([]{ ... });     // callback once when time becomes valid
 */

#include <Arduino.h>
#include <time.h>
#include <functional>

class TimeServiceClass {
public:
  /**
   * Initialize SNTP with a timezone and up to 3 NTP servers.
   * Also performs a short "warm-up" polling to quickly determine validity.
   *
   * @param tz        POSIX TZ string (default: CET with NL DST rules).
   * @param srv1..3   NTP server hostnames.
   * @param fastTries How many quick getLocalTime polls at boot (default 20).
   * @param fastDelay Delay (ms) between polls (default 100 ms).
   * @param attachWiFiCallback If true, auto re-tries sync when Wi-Fi connects.
   */
  void begin(const char* tz = "CET-1CEST,M3.5.0,M10.5.0/3",
             const char* srv1 = "pool.ntp.org",
             const char* srv2 = "time.google.com",
             const char* srv3 = "time.cloudflare.com",
             int fastTries = 20,
             int fastDelay = 100,
             bool attachWiFiCallback = true);

  /** Call regularly (optional). Currently not required, but kept for symmetry. */
  void loop();

  /** True if wall-clock time looks valid (SNTP synced or RTC sane). */
  bool isValid() const;

  /** Current epoch (seconds since 1970). Returns 0 if invalid. */
  time_t now() const;

  /**
   * Fill a struct tm with local time.
   * @param out       destination
   * @param timeoutMs getLocalTime timeout (default 50 ms)
   * @return          true if valid local time was obtained
   */
  bool getTm(struct tm& out, uint32_t timeoutMs = 50) const;

  /**
   * ISO-8601 timestamp, e.g. "2025-09-19T10:21:07".
   * Returns "" if time is not yet valid.
   */
  String nowIso8601(uint32_t timeoutMs = 50) const;

  /** Force SNTP (re)configuration and attempt to sync again. */
  void forceSync();

  /**
   * Register a callback that fires once the time becomes valid (edge-triggered).
   * If time is already valid, the callback is invoked immediately.
   */
  void onSynced(std::function<void(void)> cb);

private:
  // Internal state
  bool _synced = false;
  bool _wifiCallbackAttached = false;

  // Cached TZ + servers so we can re-apply on forceSync()
  String _tz, _s1, _s2, _s3;

  // Helper: (re)apply configTzTime with current settings
  void _applyConfig();

  // Helper: update _synced flag and fire onSynced() if we transition to valid
  void _updateSyncedFlag(bool newValid);

  // Optional one-shot callback
  std::function<void(void)> _onSynced;
};

// Global singleton (consistent naming with your other services)
extern TimeServiceClass TimeService;
