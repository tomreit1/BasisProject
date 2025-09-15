#pragma once
#include <Arduino.h>
class OTA {
 public:
  void begin(const char* hostname, const char* password = nullptr, uint16_t port = 3232);
  void loop();
  bool isUpdating() const;
 private:
  bool   _updating = false;
  uint16_t _port = 3232;
  uint8_t  _lastPct = 255;
  unsigned long _lastPrintMs = 0;
  String _host;
};
extern OTA OTAService;
