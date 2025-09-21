#include <Arduino.h>
#include <OTA/OTA.h>
#include <TaskMonitor/TaskMonitor.h>
#include <Wifihandler/Wifihandler.h>
#include "WebServer/WebServer.h"
#include "Passwords.h" // bevat WIFI_SSID, WIFI_PASS, WIFI_AP_SSID,
#include <Faulthandler/ErrorLogger.h>
#include <Time/TimeService.h>

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

  // Start NTP (NL TZ), warm-up, en luister naar Wi-Fi connect events
  TimeService.begin();
  ErrorLogService.begin("/error.log", 30);

  // (optioneel) iets doen zodra tijd “ready” is
  TimeService.onSynced([]
                       { Serial.println("[Time] Time synced!"); });
  OTAService.begin("Temperatuur_Sensor_Woonkamer", /*wachtwoord maar nu nog leeg*/ "", 3232);
  WebServerService.begin();
  ErrorLogService.logInfo("boot completed");

  WiFiService.onConnectionChange([](bool up){
  if (up) {
    ErrorLogService.breadcrumb("wifi_up");
    ErrorLogService.logNetSnapshot("NET");
  } else {
    ErrorLogService.breadcrumb("wifi_down");
  }
});
}

void loop()
{
  WiFiService.loop();
  OTAService.loop();
  ErrorLogService.loop();   // bewaart uptime periodiek in RTC
  TaskMonitor::loop(10000); // elke 10s een statusregel
}