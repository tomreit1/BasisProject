#pragma once
#include <Arduino.h>

namespace TaskMonitor {
  // Start de monitor. sampleMs = calibratie-venster (ms) voor idle-baseline.
  void begin(uint32_t sampleMs = 250);

  // Eén snapshot printen (CPU load, heap/psram, uptime, optioneel tasklist).
  void printOnce(bool includeTasks = false);

  // Periodieke service-call; mag je zo vaak als je wilt aanroepen.
  // printEveryMs > 0 => ook elke N ms printen; 0 => geen consoleprint (load loopt nu in eigen task).
  void loop(uint32_t printEveryMs = 0);

  // Directe load (niet blokkerend; levert de laatst gemeten cache).
  void getCpuLoad(float &core0, float &core1);

  // Gecachte load + leeftijd van de meting (ms) in ageMs (optioneel).
  void getCpuLoadCached(float &core0, float &core1, uint32_t* ageMs = nullptr);

  // Systeeminfo + gecachte CPU-load als JSON naar een Print (bv. AsyncResponseStream).
  void writeJsonInfo(Print& out);

  // Sampling-based "active tasks" top naar JSON (blokkerend gedurende windowMs).
  void writeJsonActive(Print& out, uint32_t windowMs = 500, uint32_t stepMs = 1, uint8_t topN = 12);
}
