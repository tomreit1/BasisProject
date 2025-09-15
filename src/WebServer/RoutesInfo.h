#pragma once
#include <ESPAsyncWebServer.h>

namespace Routes {
  void installInfo(AsyncWebServer& srv); // /info, /health
}
