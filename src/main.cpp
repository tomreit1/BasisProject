#include <Arduino.h>
#include <OTA/OTA.h>
#include <TaskMonitor/TaskMonitor.h>
#include <Wifihandler/Wifihandler.h>
#include "WebServer/WebServer.h"


void setup() {
  Serial.begin(115200);
  TaskMonitor::begin(500);   // 500 ms venster + autocalibratie
  TaskMonitor::printOnce();  // eerste snapshot
  WiFiService.begin("mijn-esp32",
                    WiFiModeSel::AP,
                    "MijnSSID", "MijnPass",/*Wifi netwerk*/
                    "ESP-AP", "SterkAPWachtwoord"/*Accespoint netwerk*/,
                    /*useWiFiManager=*/false);
  OTAService.begin("mijn-esp32", "OTApassword", 3232);
  WebServerService.begin();

}

void loop() {
  WiFiService.loop();
  OTAService.loop();
  TaskMonitor::loop(10000); // elke 10s een statusregel
}