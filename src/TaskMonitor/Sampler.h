#pragma once
#include <Arduino.h>

namespace TaskMonitor { namespace detail {
  // Console print van “active tasks” lijst (blokkerend voor windowMs)
  void printTasksOnce(uint32_t windowMs, uint32_t stepMs, uint8_t topN);

  // JSON uitstoot voor “active tasks”
  void writeJsonActive(Print& out, uint32_t windowMs, uint32_t stepMs, uint8_t topN);
}}
