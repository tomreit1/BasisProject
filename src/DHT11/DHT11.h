#pragma once
#include <Arduino.h>
#include <DHTesp.h>

class DHTSensor {
public:
  void begin(uint8_t pin, DHTesp::DHT_MODEL_t model = DHTesp::DHT11);
  bool read();                               // actieve uitlezing

  float temperature() const { return _temperature; }
  float humidity()    const { return _humidity; }
  int   status()      const { return _status; }
  String statusString() const { return _statusText; } // gebruikt cache

private:
  DHTesp _dht;
  uint8_t _pin = 255;

  float _temperature = NAN;
  float _humidity    = NAN;
  int   _status      = -1;
  String _statusText = String("uninitialized");
};

// Global singleton (optioneel)
extern DHTSensor DHTService;
