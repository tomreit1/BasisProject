#include "WebServer.h"
#include <LittleFS.h>

#include "RoutesCore.h"
#include "RoutesInfo.h"
#include "RoutesFS.h"
#include "RoutesSys.h"

WebServerHandler WebServerService;

bool WebServerHandler::begin(const Options& opts) {
  if (_server) { Serial.println(F("[Web] Already running")); return true; }

  // Mount LittleFS (auto-format on first use = true)
  if (!LittleFS.begin(true)) {
    Serial.println(F("[Web] LittleFS mount FAILED"));
    return false;
  }

  _opts = opts;
  _serverPort = opts.port;
  _server = new AsyncWebServer(_serverPort);
  if (!_server) {
    Serial.println(F("[Web] Failed to allocate server"));
    return false;
  }

  _installRoutes();
  _server->begin();
  Serial.printf("[Web] Server started on port %u\n", _serverPort);
  return true;
}

void WebServerHandler::stop() {
  if (!_server) return;
  _server->end();
  delete _server;
  _server = nullptr;
  Serial.println(F("[Web] Server stopped"));
}

void WebServerHandler::_installRoutes() {
  using namespace Routes;
  installCore(*_server);                      // favicon, root, onNotFound, assets
  installInfo(*_server);                      // /info, /health
  if (_opts.enableFsApi) installFS(*_server, _opts.fsApiAuth); // /fs/*
  installSys(*_server);                       // /sys/info, /sys/active
}
