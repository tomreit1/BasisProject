#include "RoutesSys.h"
#include <TaskMonitor/TaskMonitor.h>

namespace Routes {

void installSys(AsyncWebServer& srv){
  // GET /sys/info
  srv.on("/sys/info", HTTP_GET, [](AsyncWebServerRequest* req){
    auto* res = req->beginResponseStream("application/json");
    TaskMonitor::writeJsonInfo(*res);
    req->send(res);
  });

  // GET /sys/active[?window=500&step=1&top=12]
  srv.on("/sys/active", HTTP_GET, [](AsyncWebServerRequest* req){
    uint32_t window = 500, step = 1; uint8_t top = 12;
    if (req->hasParam("window")) window = req->getParam("window")->value().toInt();
    if (req->hasParam("step"))   step   = req->getParam("step")->value().toInt();
    if (req->hasParam("top"))    top    = req->getParam("top")->value().toInt();
    auto* res = req->beginResponseStream("application/json");
    TaskMonitor::writeJsonActive(*res, window, step, top);
    req->send(res);
  });
}

} // namespace Routes
