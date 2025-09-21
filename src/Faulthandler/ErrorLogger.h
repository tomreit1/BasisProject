#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <FS.h>

#include <Wifihandler/Wifihandler.h>         // mode/IP/SSID/RSSI
#include <Time/TimeService.h>

class ErrorLogger {
public:
  // Start de logger. logPath = append-only logbestand in LittleFS.
  // checkpointSec: hoe vaak (sec) we de uptime in RTC bijwerken.
  bool begin(const char* logPath = "/error.log", uint32_t checkpointSec = 30);

  // Periodiek aanroepen in loop(); bewaart uptime in RTC.
  void loop();

  // Handige API’s om zélf regels te schrijven.
  void logInfo(const String& msg);
  void logError(const String& msg);

  // Reset het logbestand (schrijft header opnieuw).
  bool clear();

  // Schrijf een compacte [BOOT]-regel (wordt ook in begin() gedaan).
  void logBootSummary();

  // “Breadcrumbs”: korte contextregels die in RTC ringbuffer komen en
  // bij eerstvolgende boot automatisch worden gedumpt.
  void breadcrumb(const char* tag);

  // Extra snapshots die je handmatig kunt triggeren (optioneel):
  void logNetSnapshot(const char* label = "NET");
  void logHeapSnapshot(const char* label = "HEAP");

private:
  // ---------- bestandsbeheer ----------
  bool _ensureFS();
  void _appendLine(const String& line);

  // ---------- tijd/format ----------
  String _nowIso8601(bool& timeValid, uint32_t timeoutMs = 200) const;

  // ---------- inhoud bouwers ----------
  String _resetReasonToStr(esp_reset_reason_t r) const;
  String _wifiModeStr() const;
  void   _dumpBootDetails();      // chip/cpu/heap/psram details
  void   _dumpBreadcrumbs();      // ringbuffer uit RTC
  void   _logTaskWatermarks();    // (best-effort) watermarks van bekende tasks

  // ---------- uptime & RTC ----------
  uint32_t _currentUptimeSec() const;

  struct Breadcrumb {
    uint32_t ms;           // millis() op het moment van loggen
    uint32_t free_heap;    // free heap
    int8_t   rssi;         // STA RSSI of 0 als niet van toepassing
    char     tag[16];      // korte tag (wordt getrimd)
  };

  // Let op: ATTR *definitie* staat in .cpp (niet hier).
  static RTC_DATA_ATTR uint32_t   _rtcLastUptimeSec;
  static RTC_DATA_ATTR uint32_t   _bcIndex;
  static RTC_DATA_ATTR Breadcrumb _bcRing[16];

  // ---------- state ----------
  String _path = "/error.log";
  uint32_t _checkpointSec = 30;
  bool _fsReady = false;
  unsigned long _lastTickMs = 0;

  // Handvatten om watermarks te tonen (optioneel gevuld in begin()).
  TaskHandle_t _loopTask = nullptr;
  TaskHandle_t _idle0    = nullptr;
  TaskHandle_t _idle1    = nullptr;
};

// Global singleton
extern ErrorLogger ErrorLogService;
