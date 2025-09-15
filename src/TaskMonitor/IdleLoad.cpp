#include "IdleLoad.h"
#include <esp_timer.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace TaskMonitor { namespace detail {

// ---------------- config ----------------
static uint32_t  s_baselineWindowMs = 250;
static uint32_t  s_measureWindowMs  = 500;

// ---------------- idle spin counters ----------------
static volatile uint32_t s_spin0 = 0;
static volatile uint32_t s_spin1 = 0;

// baseline spins (max idle) per core
static uint32_t  s_base0 = 1;
static uint32_t  s_base1 = 1;
static bool      s_calibrated = false;

// idle meter tasks
static TaskHandle_t s_idleTask0 = nullptr;
static TaskHandle_t s_idleTask1 = nullptr;

// background sampler task
static TaskHandle_t s_samplerTask = nullptr;

// cached load + timestamp
static float     s_lastLoad0 = 0.f;
static float     s_lastLoad1 = 0.f;
static uint32_t  s_lastStamp = 0;

// ---------------- utils ----------------
uint32_t nowMs() { return (uint32_t)(esp_timer_get_time() / 1000ULL); }

static inline void sampleSpins(uint32_t windowMs, uint32_t &d0, uint32_t &d1) {
  const uint32_t a0 = s_spin0, a1 = s_spin1;
  vTaskDelay(pdMS_TO_TICKS(windowMs));
  const uint32_t b0 = s_spin0, b1 = s_spin1;
  d0 = (b0 - a0);
  d1 = (b1 - a1);
}

static inline float spinsToLoadPct(uint32_t spins, uint32_t base) {
  if (base == 0) base = 1;
  float idlePct = 100.0f * (float)spins / (float)base;
  if (idlePct > 100.0f) idlePct = 100.0f;
  float loadPct = 100.0f - idlePct;
  if (loadPct < 0.0f) loadPct = 0.0f;
  if (loadPct > 100.0f) loadPct = 100.0f;
  return loadPct;
}

// ---------------- idle meter tasks ----------------
static void IdleMeterTask0(void*) { for(;;){ s_spin0++; } }
static void IdleMeterTask1(void*) { for(;;){ s_spin1++; } }

// ---------------- calibration ----------------
static void calibrateOnce() {
  if (s_calibrated) return;
  Serial.printf("[SYS] Calibrating (idle baseline) %ums...\n", s_baselineWindowMs);
  uint32_t best0 = 0, best1 = 0;
  for (int i=0; i<3; ++i) {
    uint32_t d0=0, d1=0; sampleSpins(s_baselineWindowMs, d0, d1);
    if (d0 > best0) best0 = d0;
    if (d1 > best1) best1 = d1;
  }
  s_base0 = best0 ? best0 : 1;
  s_base1 = best1 ? best1 : 1;
  s_calibrated = true;
  Serial.printf("[SYS] Baseline spins: core0=%u core1=%u\n", best0, best1);
}

// ---------------- background load sampler ----------------
static void LoadSamplerTask(void*) {
  for(;;){
    uint32_t d0=0, d1=0;
    sampleSpins(s_measureWindowMs, d0, d1);
    s_lastLoad0 = spinsToLoadPct(d0, s_base0);
    s_lastLoad1 = spinsToLoadPct(d1, s_base1);
    s_lastStamp = nowMs();
    // mini breather om andere ready tasks kans te geven (zonder accuracy te raken)
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// ---------------- public (detail) ----------------
void idleLoadBegin(uint32_t baselineWindowMs, uint32_t measureWindowMs) {
  s_baselineWindowMs = baselineWindowMs ? baselineWindowMs : 250;
  s_measureWindowMs  = measureWindowMs  ? measureWindowMs  : 500;

  const UBaseType_t prioIdle = tskIDLE_PRIORITY; // 0
  const uint32_t stackWords  = 1024;

  if (!s_idleTask0) {
    xTaskCreatePinnedToCore(IdleMeterTask0, "IdleMeter0", stackWords, nullptr, prioIdle, &s_idleTask0, 0);
    Serial.printf("[SYS] IdleMeter0 core0 (prio=%u)\n", (unsigned)prioIdle);
  }
  if (!s_idleTask1) {
    xTaskCreatePinnedToCore(IdleMeterTask1, "IdleMeter1", stackWords, nullptr, prioIdle, &s_idleTask1, 1);
    Serial.printf("[SYS] IdleMeter1 core1 (prio=%u)\n", (unsigned)prioIdle);
  }

  calibrateOnce();

  if (!s_samplerTask) {
    const UBaseType_t prio = tskIDLE_PRIORITY + 1; // net boven idle
    xTaskCreate(LoadSamplerTask, "LoadSampler", 2048, nullptr, prio, &s_samplerTask);
    Serial.printf("[SYS] LoadSampler started (window=%ums)\n", s_measureWindowMs);
  }
}

void idleLoadGet(float &core0, float &core1, uint32_t* ageMs) {
  core0 = s_lastLoad0; core1 = s_lastLoad1;
  if (ageMs) *ageMs = nowMs() - s_lastStamp;
}

// ---------------- console helpers ----------------
static void printHeapAndPsram() {
  const uint32_t heapFree = (uint32_t)ESP.getFreeHeap();
  const uint32_t heapMin  = (uint32_t)ESP.getMinFreeHeap();
  Serial.printf("Heap         : free=%lu  min=%lu bytes\n",
                (unsigned long)heapFree, (unsigned long)heapMin);
#if CONFIG_SPIRAM
  if (psramFound()) {
    const uint32_t psFree = (uint32_t)ESP.getFreePsram();
    Serial.printf("PSRAM        : free=%lu bytes\n", (unsigned long)psFree);
  } else {
    Serial.println(F("PSRAM        : not present"));
  }
#else
  Serial.println(F("PSRAM        : disabled (no CONFIG_SPIRAM)"));
#endif
}

void printHeader() {
  Serial.println(F("\n========== TaskMonitor =========="));
  Serial.printf("Uptime       : %lu s\n", (unsigned long)(millis()/1000UL));
  printHeapAndPsram();
}

void printCpu() {
  float l0=0.f, l1=0.f; uint32_t age=0;
  idleLoadGet(l0, l1, &age);
  Serial.printf("CPU Load     : core0=%5.1f%%  core1=%5.1f%%  (age=%lums, window=%ums)\n",
                l0, l1, (unsigned long)age, s_measureWindowMs);
}

void printFooter() {
  Serial.println(F("==================================\n"));
}

}} // namespace TaskMonitor::detail
