#pragma once
#include <Arduino.h>

namespace TaskMonitor { namespace detail {
  // Start idle-meters (per core) + achtergrond-sampler task voor CPU-load.
  void idleLoadBegin(uint32_t baselineWindowMs, uint32_t measureWindowMs);
  // Lees gecachte load; ageMs (optioneel) = leeftijd van de meting in ms.
  void idleLoadGet(float &core0, float &core1, uint32_t* ageMs);

  // Helpers die ook door printers/JSON gebruikt worden
  uint32_t nowMs();

  // Console output helpers
  void printHeader();
  void printCpu();
  void printFooter();
}}
