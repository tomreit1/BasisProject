#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <functional>

enum class WiFiModeSel : uint8_t {
  STA = 0,     // Station only
  AP,          // Access Point only
  AP_STA       // Both (AP + STA)
};

class WiFiHandler {
public:
  // Begin with explicit mode selection.
  // - hostname: device hostname for STA (and mDNS if you use it elsewhere)
  // - mode: STA / AP / AP_STA
  // - staSsid/staPass: for STA mode (optional if you rely on stored creds or WiFiManager)
  // - apSsid/apPass:  for AP mode (ignored in pure STA)
  // - useWiFiManager: only used in STA or AP_STA; requires -DUSE_WIFI_MANAGER
  void begin(const char* hostname,
             WiFiModeSel mode,
             const char* staSsid = nullptr,
             const char* staPass = nullptr,
             const char* apSsid  = "ESP32-AP",
             const char* apPass  = nullptr,
             bool useWiFiManager = false);

  // Run periodic tasks (reconnect backoff in STA/AP_STA).
  void loop();

  // Status helpers
  bool isConnected() const;          // STA link status
  IPAddress localIP() const;         // STA IP (in AP-only returns WiFi.softAPIP())
  String currentSSID() const;        // STA SSID
  int32_t rssi() const;              // STA RSSI (in AP-only returns 0)

  // Change mode at runtime (optional). If forceRestart=true, will stop/restart WiFi.
  void setMode(WiFiModeSel mode, bool forceRestart = true);

  // Optional: set static IP for STA BEFORE begin()
  bool setStaticIP(IPAddress ip, IPAddress gateway, IPAddress subnet,
                   IPAddress dns1 = IPAddress(8,8,8,8),
                   IPAddress dns2 = IPAddress(1,1,1,1));

  // Optional: set AP network config BEFORE begin() (default 192.168.4.1)
  bool setAPIP(IPAddress ip, IPAddress gateway, IPAddress subnet);

  // Force reconnect (STA)
  void forceReconnect();

  // Callback when STA connection status flips (false->true / true->false)
  void onConnectionChange(std::function<void(bool)> cb);

  // Quick getters
  WiFiModeSel mode() const { return _mode; }
  String hostname() const { return _hostname; }

private:
  void _attachWiFiEvents();
  void _logSummarySTA() const;
  void _logSummaryAP() const;

  // STA flow
  void _startSTA(const char* ssid, const char* pass, bool useWiFiManager);
  void _connectIfNeededSTA();
  void _onGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
  void _onDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);

  // AP flow
  void _startAP(const char* apSsid, const char* apPass);
  void _stopAP();
  void _onAPClientJoin(WiFiEvent_t event, WiFiEventInfo_t info);
  void _onAPClientLeave(WiFiEvent_t event, WiFiEventInfo_t info);

  // Shared
  static void _safeSetHostname(const String& host);

  // State
  String _hostname;
  String _staSsid, _staPass;
  String _apSsid,  _apPass;
  WiFiModeSel _mode = WiFiModeSel::STA;

  bool _connected = false;          // STA link state
  bool _forceReconnectFlag = false;
  bool _useWiFiManager = false;

  // Backoff (STA)
  unsigned long _lastAttemptMs = 0;
  uint32_t _backoffMs = 0;
  const uint32_t _minBackoffMs = 1000;
  const uint32_t _maxBackoffMs = 30000;

  // STA static IP
  bool _useStaticIP = false;
  IPAddress _ip, _gw, _sn, _dns1, _dns2;

  // AP config
  bool _apHasCustomIP = false;
  IPAddress _apIP = IPAddress(192,168,4,1);
  IPAddress _apGW = IPAddress(192,168,4,1);
  IPAddress _apSN = IPAddress(255,255,255,0);
  uint8_t   _apChannel = 1;
  bool      _apHidden = false;
  uint8_t   _apMaxConn = 4;

  std::function<void(bool)> _onChange;
};

// Global singleton
extern WiFiHandler WiFiService;
