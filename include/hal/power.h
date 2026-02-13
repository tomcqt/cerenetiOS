#ifndef POWER_H
#define POWER_H

#pragma once

#include <stdint.h>

namespace hal::power {
  uint16_t voltage_mv();
  int percent();
  bool charging();
}

#endif