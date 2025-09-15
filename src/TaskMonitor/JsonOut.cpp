#include "JsonOut.h"
#include "IdleLoad.h"
#include <esp_system.h>

namespace TaskMonitor { namespace detail {

void writeJsonInfo(Print& out) {
  float l0=0.f, l1=0.f; uint32_t age=0;
  idleLoadGet(l0, l1, &age);

  const uint32_t up = millis()/1000UL;
  const uint32_t heapFree = (uint32_t)ESP.getFreeHeap();
  const uint32_t heapMin  = (uint32_t)ESP.getMinFreeHeap();

  out.print(F("{\"uptime_s\":"));      out.print(up);
  out.print(F(",\"heap_free\":"));     out.print(heapFree);
  out.print(F(",\"heap_min\":"));      out.print(heapMin);
#if CONFIG_SPIRAM
  out.print(F(",\"psram_present\":")); out.print((int)psramFound());
  if (psramFound()) { out.print(F(",\"psram_free\":")); out.print((uint32_t)ESP.getFreePsram()); }
#else
  out.print(F(",\"psram_present\":false"));
#endif
  out.print(F(",\"cpu_load\":["));
  out.print(l0,1); out.print(','); out.print(l1,1);
  out.print(F("],\"cpu_age_ms\":")); out.print(age);
  out.print(F("}"));
}

}} // namespace
