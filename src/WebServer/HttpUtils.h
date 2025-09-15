#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>

namespace HttpUtils {
  String sanitizePath(const String& in);
  void   setNoCache(AsyncWebServerResponse* r);
  void   setCacheLong(AsyncWebServerResponse* r);
  String guessMime(const String& p);
  String jsonEscape(const String& s);
  void   sendJson(AsyncWebServerRequest* req, const String& json);
  void   sendFilePlain(AsyncWebServerRequest* req, String path); // chunked
}
