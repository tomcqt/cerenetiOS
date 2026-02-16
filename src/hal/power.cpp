#include "hal/power.h"
#include <sys/power.h>
#include <gfx/gfx.h>

#include <cstdio>

namespace hal::Power {
  bool charging() {
    return (boot_BatteryCharging() != 0);
  }

  int percent() {
    uint16_t mv = boot_GetBatteryStatus();

    switch (mv) {
      case 4:
        return 100;
      case 3:
        return 75;
      case 2:
        return 50;
      case 1:
        return 25;
      case 0:
        return 1;
    }
    return 0;
  }
}