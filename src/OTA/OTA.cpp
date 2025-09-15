#include "OTA.h"

#include <WiFi.h>
#include <ArduinoOTA.h>

OTA OTAService;

void OTA::begin(const char* hostname, const char* password, uint16_t port) {
  _host = (hostname && *hostname) ? String(hostname) : String("esp32");
  _port = port;
  _updating = false;
  _lastPct = 255;
  _lastPrintMs = 0;

  // Basisconfig
  ArduinoOTA.setPort(_port);
  ArduinoOTA.setHostname(_host.c_str());
  if (password && *password) {
    ArduinoOTA.setPassword(password);         // desnoods: setPasswordHash(md5)
  }

  // Callbacks
  ArduinoOTA.onStart([this]() {
    _updating = true;
    _lastPct = 255;
    _lastPrintMs = 0;

    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    // NB: Bij filesystem OTA (LittleFS/SPIFFS) moet je eigen code evt. FS afsluiten
    // voordat de update start. Dit is alleen een melding/log.
    Serial.printf("[OTA] Start (%s)\n", type.c_str());
  });

  ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
    if (total == 0) return;
    uint8_t pct = (progress * 100U) / total;
    // Print niet té vaak (max ~2x/sec) en alleen bij nieuw percentage
    unsigned long now = millis();
    if (pct != _lastPct && (now - _lastPrintMs) > 500) {
      _lastPct = pct;
      _lastPrintMs = now;
      Serial.printf("[OTA] %3u%%  (%u/%u)\n", pct, progress, total);
    }
  });

  ArduinoOTA.onEnd([this]() {
    Serial.println("\n[OTA] End");
    _updating = false;
  });

  ArduinoOTA.onError([this](ota_error_t error) {
    _updating = false;
    Serial.printf("[OTA] Error[%u]: ", static_cast<unsigned>(error));
    switch (error) {
      case OTA_AUTH_ERROR:    Serial.println("Auth Failed"); break;
      case OTA_BEGIN_ERROR:   Serial.println("Begin Failed"); break;
      case OTA_CONNECT_ERROR: Serial.println("Connect Failed"); break;
      case OTA_RECEIVE_ERROR: Serial.println("Receive Failed"); break;
      case OTA_END_ERROR:     Serial.println("End Failed"); break;
      default:                Serial.println("Unknown"); break;
    }
  });

  ArduinoOTA.begin();

  // Korte samenvatting in log
  Serial.println("========================================");
  Serial.println("[OTA] Ready");
  Serial.printf("[OTA] Hostname : %s\n", _host.c_str());
  Serial.printf("[OTA] Port     : %u\n", _port);
  Serial.printf("[OTA] IP       : %s\n",
                WiFi.isConnected() ? WiFi.localIP().toString().c_str() : "(no WiFi)");
  Serial.println("========================================");
}

void OTA::loop() {
  // Belangrijk: regelmatig aanroepen in je main loop
  ArduinoOTA.handle();
}

bool OTA::isUpdating() const {
  return _updating;
}
