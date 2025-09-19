#include <Arduino.h>
#include <OTA/OTA.h>
#include <TaskMonitor/TaskMonitor.h>
#include <Wifihandler/Wifihandler.h>
#include "WebServer/WebServer.h"
#include "Passwords.h" // bevat WIFI_SSID, WIFI_PASS, WIFI_AP_SSID,


void setup()
{
  Serial.begin(115200);
  TaskMonitor::begin(500);  // 500 ms venster + autocalibratie
  TaskMonitor::printOnce(); // eerste snapshot
  WiFiService.begin("mijn-esp32",
                    WiFiModeSel::STA,
                    WIFI_SSID, WIFI_PASS, /*Wifi netwerk*/
                    WIFI_AP_SSID, WIFI_AP_PASS /*Accespoint netwerk*/,
                    /*useWiFiManager=*/false);
  OTAService.begin("Temperatuur_Sensor_Woonkamer", /*wachtwoord maar nu nog leeg*/ "", 3232);
  WebServerService.begin();
} // hihi

void loop()
{
  WiFiService.loop();
  OTAService.loop();
  TaskMonitor::loop(10000); // elke 10s een statusregel
}