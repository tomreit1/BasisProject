#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>

class WebServerHandler {
public:
struct Options {
  uint16_t port;
  bool enableFsApi;
  bool fsApiAuth;

  Options()
  : port(80)
  , enableFsApi(true)
#if defined(WEBSERVER_AUTH_USER) && defined(WEBSERVER_AUTH_PASS)
  , fsApiAuth(true)
#else
  , fsApiAuth(false)
#endif
  {}
};


  bool begin(const Options& opts = Options{}); // start server
  void stop();                                  // stop server

  bool isRunning() const { return _server != nullptr; }
  uint16_t port()  const { return _serverPort; }
  AsyncWebServer* server() { return _server; }

private:
  void _installRoutes();

  AsyncWebServer* _server = nullptr;
  uint16_t _serverPort = 80;
  Options _opts{};
};

extern WebServerHandler WebServerService;
