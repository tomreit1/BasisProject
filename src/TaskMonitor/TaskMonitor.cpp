#include "TaskMonitor.h"
#include "IdleLoad.h"
#include "Sampler.h"
#include "JsonOut.h"

namespace TaskMonitor {

void begin(uint32_t sampleMs) {
  detail::idleLoadBegin(sampleMs, /*measureWindowMs=*/500);
}

void printOnce(bool includeTasks) {
  detail::printHeader();
  detail::printCpu();
  if (includeTasks) detail::printTasksOnce(/*windowMs=*/500, /*stepMs=*/1, /*topN=*/12);
  detail::printFooter();
}

void loop(uint32_t printEveryMs) {
  // load loopt in een eigen task; hier alleen optioneel print pacing
  static uint32_t lastPrint = 0;
  if (!printEveryMs) return;
  uint32_t now = detail::nowMs();
  if ((uint32_t)(now - lastPrint) >= printEveryMs) {
    lastPrint = now;
    detail::printHeader();
    detail::printCpu();
    detail::printFooter();
  }
}

void getCpuLoad(float &core0, float &core1) {
  detail::idleLoadGet(core0, core1, nullptr);
}

void getCpuLoadCached(float &core0, float &core1, uint32_t* ageMs) {
  detail::idleLoadGet(core0, core1, ageMs);
}

void writeJsonInfo(Print& out) {
  detail::writeJsonInfo(out);
}

void writeJsonActive(Print& out, uint32_t windowMs, uint32_t stepMs, uint8_t topN) {
  detail::writeJsonActive(out, windowMs, stepMs, topN);
}

} // namespace TaskMonitor
