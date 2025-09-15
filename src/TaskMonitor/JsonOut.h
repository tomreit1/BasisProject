#pragma once
#include <Arduino.h>

namespace TaskMonitor { namespace detail {
  void writeJsonInfo(Print& out);

  // console helpers die door façade gebruikt worden
  void printHeader();
  void printCpu();
  void printFooter();
}}
