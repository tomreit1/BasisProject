#include "RoutesInfo.h"
#include <WiFi.h>

namespace Routes {

void installInfo(AsyncWebServer& srv){
  // /info
  srv.on("/info", HTTP_GET, [](AsyncWebServerRequest* req){
    wifi_mode_t mode = WiFi.getMode();
    bool apOn  = mode & WIFI_MODE_AP;
    bool staOn = mode & WIFI_MODE_STA;
    bool staConnected = staOn && (WiFi.status() == WL_CONNECTED);

    String modeStr = staConnected ? (apOn ? "AP+STA" : "STA")
                                  : (apOn ? "AP" : "OFF");

    String ip   = staConnected ? WiFi.localIP().toString()
                               : (apOn ? WiFi.softAPIP().toString() : String("0.0.0.0"));

    String ssid = staConnected ? WiFi.SSID()
                               : (apOn ? WiFi.softAPSSID() : String(""));

    int clients = apOn ? WiFi.softAPgetStationNum() : 0;

    String json = "{";
    json += "\"mode\":\"" + modeStr + "\",";
    json += "\"ip\":\"" + ip + "\",";
    json += "\"ssid\":\"" + ssid + "\",";
    if (staConnected) json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    else              json += "\"rssi\":" + String(random(0,100)-70) + ",";
    json += "\"ap_clients\":" + String(clients);
    json += "}";

    req->send(200, "application/json", json);
  });

  // /health
  srv.on("/health", HTTP_GET, [](AsyncWebServerRequest* req){
    req->send(200, "application/json", "{\"status\":\"ok\"}");
  });
}

} // namespace Routes
