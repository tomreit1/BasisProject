#include "DHT11.h"

DHTSensor DHTService;

void DHTSensor::begin(uint8_t pin, DHTesp::DHT_MODEL_t model) {
  _pin = pin;
  _dht.setup(_pin, model);
  delay(2000); // stabiliseren
  _statusText = "ready";
  Serial.printf("[DHT] Initialized on pin %u (model=%u)\n", _pin, model);
}

bool DHTSensor::read() {
  TempAndHumidity data = _dht.getTempAndHumidity();
  _status = _dht.getStatus();
  _statusText = _dht.getStatusString();   // hier mag het (niet-const context)

  if (_status == 0) {
    _temperature = data.temperature;
    _humidity    = data.humidity;
    Serial.printf("[DHT] Temp: %.1f °C | Humidity: %.1f %%\n", _temperature, _humidity);
    return true;
  }

  Serial.printf("[DHT] Read error: %s\n", _statusText.c_str());
  return false;
}
