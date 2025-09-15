#include "Sampler.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace TaskMonitor { namespace detail {

struct TaskCount { TaskHandle_t h; uint32_t count; };
static const size_t MAX_TRACK = 64;

static void reset(TaskCount* a){
  for (size_t i=0;i<MAX_TRACK;i++){ a[i].h=nullptr; a[i].count=0; }
}
static void add(TaskCount* a, TaskHandle_t h){
  if(!h) return;
  for(size_t i=0;i<MAX_TRACK;i++){
    if(a[i].h==h){ a[i].count++; return; }
    if(a[i].h==nullptr){ a[i].h=h; a[i].count=1; return; }
  }
}
static int cmp(const void* A, const void* B){
  auto a=(const TaskCount*)A; auto b=(const TaskCount*)B;
  if(a->count < b->count) return 1;
  if(a->count > b->count) return -1;
  return 0;
}

void printTasksOnce(uint32_t windowMs, uint32_t stepMs, uint8_t topN){
  TaskCount counts[MAX_TRACK]; reset(counts);
  uint32_t samples = windowMs / stepMs; if (!samples) samples = 1;
  for(uint32_t i=0;i<samples;i++){
    add(counts, xTaskGetCurrentTaskHandleForCPU(0));
    add(counts, xTaskGetCurrentTaskHandleForCPU(1));
    vTaskDelay(pdMS_TO_TICKS(stepMs + ((i%3)==0 ? 1 : 0)));
  }
  qsort(counts, MAX_TRACK, sizeof(TaskCount), cmp);
  const uint32_t total = samples * 2;

  Serial.printf("\n-- Active tasks (sampling %ums @ ~%u–%ums) --\n",
                (unsigned)windowMs, (unsigned)stepMs, (unsigned)(stepMs+1));
  Serial.println(F("Name                Core Prio StackMin  Share%"));
  Serial.println(F("------------------------------------------------"));

  uint8_t printed = 0;
  for(size_t i=0; i<MAX_TRACK && counts[i].h && printed<topN; ++i){
    TaskHandle_t h = counts[i].h;
    const char* name = pcTaskGetName(h);
    UBaseType_t prio = uxTaskPriorityGet(h);
#if (INCLUDE_uxTaskGetStackHighWaterMark == 1)
    UBaseType_t stackMin = uxTaskGetStackHighWaterMark(h);
#else
    UBaseType_t stackMin = 0;
#endif
    int core = -1;
    TaskHandle_t c0 = xTaskGetCurrentTaskHandleForCPU(0);
    TaskHandle_t c1 = xTaskGetCurrentTaskHandleForCPU(1);
    if (h==c0) core=0; else if (h==c1) core=1;

    const float pct = total? (100.f*(float)counts[i].count/(float)total) : 0.f;
    Serial.printf("%-18s %4s %4u %8u %7.2f%%\n",
      (name ? name : "(null)"),
      (core==0?"0": core==1?"1":"?"),
      (unsigned)prio,
      (unsigned)stackMin,
      pct
    );
    printed++;
  }
  Serial.println(F("------------------------------------------------"));
}

void writeJsonActive(Print& out, uint32_t windowMs, uint32_t stepMs, uint8_t topN){
  TaskCount counts[MAX_TRACK]; reset(counts);
  uint32_t samples = windowMs / stepMs; if (!samples) samples = 1;
  for(uint32_t i=0;i<samples;i++){
    add(counts, xTaskGetCurrentTaskHandleForCPU(0));
    add(counts, xTaskGetCurrentTaskHandleForCPU(1));
    vTaskDelay(pdMS_TO_TICKS(stepMs + ((i%3)==0 ? 1 : 0)));
  }
  qsort(counts, MAX_TRACK, sizeof(TaskCount), cmp);
  const uint32_t total = samples * 2;

  out.print(F("{\"window_ms\":")); out.print(windowMs);
  out.print(F(",\"step_ms\":"));  out.print(stepMs);
  out.print(F(",\"total_samples\":")); out.print(total);
  out.print(F(",\"tasks\":["));

  uint8_t printed=0;
  for(size_t i=0; i<MAX_TRACK && counts[i].h && printed<topN; ++i){
    TaskHandle_t h = counts[i].h;
    const char* name = pcTaskGetName(h);
    UBaseType_t prio = uxTaskPriorityGet(h);
#if (INCLUDE_uxTaskGetStackHighWaterMark == 1)
    UBaseType_t stackMin = uxTaskGetStackHighWaterMark(h);
#else
    UBaseType_t stackMin = 0;
#endif
    int core = -1;
    TaskHandle_t c0 = xTaskGetCurrentTaskHandleForCPU(0);
    TaskHandle_t c1 = xTaskGetCurrentTaskHandleForCPU(1);
    if (h==c0) core=0; else if (h==c1) core=1;

    const float share = total? (100.f*(float)counts[i].count/(float)total) : 0.f;

    if(printed++) out.print(',');
    out.print(F("{\"name\":\"")); out.print(name?name:"(null)");
    out.print(F("\",\"core\":")); out.print(core);
    out.print(F(",\"prio\":"));   out.print((unsigned)prio);
    out.print(F(",\"stack_min\":")); out.print((unsigned)stackMin);
    out.print(F(",\"share\":"));  out.print(share,2);
    out.print('}');
  }
  out.print(F("]}"));
}

}} // namespace TaskMonitor::detail
