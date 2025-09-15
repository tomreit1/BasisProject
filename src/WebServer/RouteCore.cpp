#include "RoutesCore.h"
#include <LittleFS.h>
#include "HttpUtils.h"

using namespace HttpUtils;

namespace Routes {

void installCore(AsyncWebServer& srv){
  // Favicon -> 204
  srv.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* r){ r->send(204); });

  // Root -> /index.html (fallback /www/index.html)
  srv.on("/", HTTP_GET, [](AsyncWebServerRequest* req){
    if (LittleFS.exists("/index.html"))     { sendFilePlain(req, "/index.html"); return; }
    if (LittleFS.exists("/www/index.html")) { sendFilePlain(req, "/www/index.html"); return; }
    req->send(500, "text/plain", "index.html not found");
  });

  // Catch-all: assets -> exact -> SPA fallback -> 404
  srv.onNotFound([](AsyncWebServerRequest* req){
    String url = req->url();
    if (!url.startsWith("/")) url = "/" + url;
    if (url.endsWith("/"))    url += "index.html";

    if (url.startsWith("/assets/")) { sendFilePlain(req, url); return; }
    if (LittleFS.exists(url))       { sendFilePlain(req, url); return; }

    if (LittleFS.exists("/index.html"))     { sendFilePlain(req, "/index.html"); return; }
    if (LittleFS.exists("/www/index.html")) { sendFilePlain(req, "/www/index.html"); return; }

    req->send(404, "text/plain", "Not found");
  });
}

} // namespace Routes
