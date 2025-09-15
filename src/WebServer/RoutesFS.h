#pragma once
#include <ESPAsyncWebServer.h>

namespace Routes {
  void installFS(AsyncWebServer& srv, bool requireAuth = false);
}
