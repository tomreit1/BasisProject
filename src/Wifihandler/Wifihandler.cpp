#include "WiFiHandler.h"

#ifdef USE_WIFI_MANAGER
  #include <WiFiManager.h>
#endif

WiFiHandler WiFiService;

// --- Helpers ----------------------------------------------------------------
void WiFiHandler::_safeSetHostname(const String& host) {
  WiFi.setHostname(host.c_str());
}

// --- Public API --------------------------------------------------------------
void WiFiHandler::begin(const char* hostname,
                        WiFiModeSel mode,
                        const char* staSsid,
                        const char* staPass,
                        const char* apSsid,
                        const char* apPass,
                        bool useWiFiManager) {
  _hostname = (hostname && *hostname) ? String(hostname) : String("esp32");
  _mode = mode;

  if (staSsid && *staSsid) _staSsid = staSsid; else _staSsid = String();
  if (staPass && *staPass) _staPass = staPass; else _staPass = String();

  _apSsid = (apSsid && *apSsid) ? String(apSsid) : String("ESP32-AP");
  _apPass = (apPass && *apPass) ? String(apPass) : String(); // empty = open AP
  _useWiFiManager = useWiFiManager;

  _connected = false;
  _backoffMs = _minBackoffMs;
  _lastAttemptMs = 0;

  // Base mode first
  switch (_mode) {
    case WiFiModeSel::STA:    WiFi.mode(WIFI_STA); break;
    case WiFiModeSel::AP:     WiFi.mode(WIFI_AP);  break;
    case WiFiModeSel::AP_STA: WiFi.mode(WIFI_AP_STA); break;
  }

  _safeSetHostname(_hostname);
  _attachWiFiEvents();

  // Apply STA static IP if requested
  if (_useStaticIP && (_mode == WiFiModeSel::STA || _mode == WiFiModeSel::AP_STA)) {
    if (!WiFi.config(_ip, _gw, _sn, _dns1, _dns2)) {
      Serial.println(F("[WiFi] Failed to apply static IP config"));
    } else {
      Serial.printf("[WiFi] Static IP set: %s\n", _ip.toString().c_str());
    }
  }

  // Start AP side if needed
  if (_mode == WiFiModeSel::AP || _mode == WiFiModeSel::AP_STA) {
    _startAP(_apSsid.c_str(), _apPass.length() ? _apPass.c_str() : nullptr);
  }

  // Start STA side if needed
  if (_mode == WiFiModeSel::STA || _mode == WiFiModeSel::AP_STA) {
    _startSTA(_staSsid.length() ? _staSsid.c_str() : nullptr,
              _staPass.length() ? _staPass.c_str() : nullptr,
              _useWiFiManager);
  }
}

void WiFiHandler::loop() {
  // Only handle reconnect logic for STA-capable modes
  if (_mode == WiFiModeSel::STA || _mode == WiFiModeSel::AP_STA) {
    if (!_connected || _forceReconnectFlag) {
      const unsigned long now = millis();
      if (now - _lastAttemptMs >= _backoffMs) {
        _lastAttemptMs = now;
        _connectIfNeededSTA();
        _backoffMs = (_backoffMs == 0) ? _minBackoffMs : (_backoffMs * 2);
        if (_backoffMs > _maxBackoffMs) _backoffMs = _maxBackoffMs;
      }
    }
  }
}

bool WiFiHandler::isConnected() const { return WiFi.status() == WL_CONNECTED; }
IPAddress WiFiHandler::localIP() const {
  if (_mode == WiFiModeSel::AP && WiFi.status() != WL_CONNECTED) return WiFi.softAPIP();
  return WiFi.localIP();
}
String WiFiHandler::currentSSID() const { return WiFi.SSID(); }
int32_t WiFiHandler::rssi() const { return (_mode == WiFiModeSel::AP) ? 0 : WiFi.RSSI(); }

void WiFiHandler::setMode(WiFiModeSel mode, bool forceRestart) {
  if (mode == _mode && !forceRestart) return;

  // Stop existing AP if switching away
  if (_mode != WiFiModeSel::AP && mode == WiFiModeSel::AP) {
    // switching to AP-only
  }
  if (_mode == WiFiModeSel::AP && mode != WiFiModeSel::AP) {
    _stopAP();
  }

  _mode = mode;

  if (forceRestart) {
    WiFi.disconnect(true, true);
    switch (_mode) {
      case WiFiModeSel::STA:    WiFi.mode(WIFI_STA);    break;
      case WiFiModeSel::AP:     WiFi.mode(WIFI_AP);     break;
      case WiFiModeSel::AP_STA: WiFi.mode(WIFI_AP_STA); break;
    }
    _safeSetHostname(_hostname);

    if (_mode == WiFiModeSel::AP || _mode == WiFiModeSel::AP_STA) {
      _startAP(_apSsid.c_str(), _apPass.length() ? _apPass.c_str() : nullptr);
    }
    if (_mode == WiFiModeSel::STA || _mode == WiFiModeSel::AP_STA) {
      _startSTA(_staSsid.length() ? _staSsid.c_str() : nullptr,
                _staPass.length() ? _staPass.c_str() : nullptr,
                _useWiFiManager);
    }
  }
}

bool WiFiHandler::setStaticIP(IPAddress ip, IPAddress gateway, IPAddress subnet,
                              IPAddress dns1, IPAddress dns2) {
  _useStaticIP = true;
  _ip = ip; _gw = gateway; _sn = subnet; _dns1 = dns1; _dns2 = dns2;
  return true;
}

bool WiFiHandler::setAPIP(IPAddress ip, IPAddress gateway, IPAddress subnet) {
  _apHasCustomIP = true;
  _apIP = ip; _apGW = gateway; _apSN = subnet;
  return true;
}

void WiFiHandler::forceReconnect() {
  if (_mode == WiFiModeSel::AP) return;
  _forceReconnectFlag = true;
  WiFi.disconnect(true, true);
}

void WiFiHandler::onConnectionChange(std::function<void(bool)> cb) {
  _onChange = std::move(cb);
}

// --- Private: events & logging ---------------------------------------------
void WiFiHandler::_attachWiFiEvents() {
  // STA events
  WiFi.onEvent([this](WiFiEvent_t e, WiFiEventInfo_t info){ _onGotIP(e, info); },
               ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent([this](WiFiEvent_t e, WiFiEventInfo_t info){ _onDisconnected(e, info); },
               ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  // AP client events (optional diagnostics)
  WiFi.onEvent([this](WiFiEvent_t e, WiFiEventInfo_t info){ _onAPClientJoin(e, info); },
               ARDUINO_EVENT_WIFI_AP_STACONNECTED);
  WiFi.onEvent([this](WiFiEvent_t e, WiFiEventInfo_t info){ _onAPClientLeave(e, info); },
               ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
}

void WiFiHandler::_logSummarySTA() const {
  Serial.println("----------- WiFi (STA) -----------");
  Serial.printf(" Hostname : %s\n", _hostname.c_str());
  Serial.printf(" SSID     : %s\n", WiFi.SSID().c_str());
  Serial.printf(" IP       : %s\n", WiFi.localIP().toString().c_str());
  Serial.printf(" RSSI     : %d dBm\n", WiFi.RSSI());
  Serial.println("----------------------------------");
}

void WiFiHandler::_logSummaryAP() const {
  Serial.println("----------- WiFi (AP) ------------");
  Serial.printf(" AP SSID  : %s\n", _apSsid.c_str());
  Serial.printf(" AP IP    : %s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf(" Channel  : %u  Hidden: %s  MaxConn: %u\n",
                _apChannel, _apHidden ? "yes" : "no", _apMaxConn);
  Serial.println("----------------------------------");
}

// --- STA logic --------------------------------------------------------------
void WiFiHandler::_startSTA(const char* ssid, const char* pass, bool useWiFiManager) {
#ifdef USE_WIFI_MANAGER
  if (useWiFiManager) {
    WiFiManager wm;
    wm.setHostname(_hostname);
    wm.setBreakAfterConfig(true);
    bool ok = wm.autoConnect("ESP32-Setup", "12345678");
    if (!ok) {
      Serial.println(F("[WiFi] WiFiManager failed; will retry with backoff."));
      _connected = false;
      return;
    }
    _connected = true;
    _forceReconnectFlag = false;
    _backoffMs = _minBackoffMs;
    _logSummarySTA();
    return;
  }
#else
  (void)useWiFiManager;
#endif

  if (ssid && *ssid) {
    Serial.printf("[WiFi] Connecting to SSID: %s\n", ssid);
    WiFi.begin(ssid, (pass ? pass : ""));
  } else {
    Serial.println(F("[WiFi] Connecting with stored credentials"));
    WiFi.begin();
  }
}

void WiFiHandler::_connectIfNeededSTA() {
  if (WiFi.status() == WL_CONNECTED) {
    _connected = true;
    _forceReconnectFlag = false;
    _backoffMs = _minBackoffMs;
    return;
  }
  if (_forceReconnectFlag) {
    Serial.println(F("[WiFi] Forcing reconnect..."));
    _startSTA(_staSsid.length() ? _staSsid.c_str() : nullptr,
              _staPass.length() ? _staPass.c_str() : nullptr,
              _useWiFiManager);
    _forceReconnectFlag = false;
    return;
  }
  Serial.println(F("[WiFi] Attempting reconnect (STA)..."));
  _startSTA(_staSsid.length() ? _staSsid.c_str() : nullptr,
            _staPass.length() ? _staPass.c_str() : nullptr,
            _useWiFiManager);
}

void WiFiHandler::_onGotIP(WiFiEvent_t, WiFiEventInfo_t) {
  const bool prev = _connected;
  _connected = true;
  _forceReconnectFlag = false;
  _backoffMs = _minBackoffMs;

  Serial.printf("[WiFi] STA connected. IP: %s  SSID: %s  RSSI: %d dBm\n",
                WiFi.localIP().toString().c_str(),
                WiFi.SSID().c_str(),
                WiFi.RSSI());
  _safeSetHostname(_hostname);
  _logSummarySTA();
  if (_onChange && prev != _connected) _onChange(_connected);
}

void WiFiHandler::_onDisconnected(WiFiEvent_t, WiFiEventInfo_t info) {
  const bool prev = _connected;
  _connected = false;
  Serial.printf("[WiFi] STA disconnected (reason=%u). Backoff=%lu ms.\n",
                info.wifi_sta_disconnected.reason, (unsigned long)_backoffMs);
  if (_onChange && prev != _connected) _onChange(_connected);
}

// --- AP logic ---------------------------------------------------------------
void WiFiHandler::_startAP(const char* apSsid, const char* apPass) {
  if (_apHasCustomIP) {
    if (!WiFi.softAPConfig(_apIP, _apGW, _apSN)) {
      Serial.println(F("[WiFi] softAPConfig failed; using defaults."));
    }
  }
  bool ok = WiFi.softAP(apSsid, (apPass ? apPass : nullptr), _apChannel, _apHidden, _apMaxConn);
  if (!ok) {
    Serial.println(F("[WiFi] Failed to start AP"));
    return;
  }
  Serial.printf("[WiFi] AP started: SSID=%s  IP=%s  %s\n",
                apSsid, WiFi.softAPIP().toString().c_str(),
                (apPass && *apPass) ? "(WPA2-PSK)" : "(OPEN)");
  _logSummaryAP();
}

void WiFiHandler::_stopAP() {
  WiFi.softAPdisconnect(true);
  Serial.println(F("[WiFi] AP stopped"));
}

void WiFiHandler::_onAPClientJoin(WiFiEvent_t, WiFiEventInfo_t info) {
  Serial.printf("[WiFi] AP client joined: %02X:%02X:%02X:%02X:%02X:%02X\n",
                info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1],
                info.wifi_ap_staconnected.mac[2], info.wifi_ap_staconnected.mac[3],
                info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5]);
}

void WiFiHandler::_onAPClientLeave(WiFiEvent_t, WiFiEventInfo_t info) {
  Serial.printf("[WiFi] AP client left: %02X:%02X:%02X:%02X:%02X:%02X\n",
                info.wifi_ap_stadisconnected.mac[0], info.wifi_ap_stadisconnected.mac[1],
                info.wifi_ap_stadisconnected.mac[2], info.wifi_ap_stadisconnected.mac[3],
                info.wifi_ap_stadisconnected.mac[4], info.wifi_ap_stadisconnected.mac[5]);
}
