#pragma once
#include <ESPAsyncWebServer.h>

namespace Routes {
  void installSys(AsyncWebServer& srv); // /sys/info, /sys/active
}
