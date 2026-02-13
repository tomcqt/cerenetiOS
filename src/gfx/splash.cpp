#include "gfx/splash.h"

#include <graphx.h>
#include <keypadc.h>
#include <sys/lcd.h>
#include <sys/timers.h>

#include <cstring>

#include "images/splash_gfx.h"
#include "config.h"

static constexpr uint32_t SPLASH_TIMEOUT_SEC = 3;

namespace gfx::splash {
  void show() {
    // apply the converted palette
    gfx_SetPalette(splash_palette, sizeof_splash_palette, 0);

    // copy raw pixel data directly to lcd vram
    memcpy((void*)gfx_vram, splash_data, 320 * 240);

    // wait for timeout or keypress
    uint32_t start = timer_GetSafe(1, TIMER_UP);
    bool dismissed = false;

    while (!dismissed) {
      kb_Scan();

      for (int group = 1; group <= 7; group++) {
        if (kb_Data[group] != 0) {
          dismissed = true;
          break;
        }
      }

      uint32_t elapsed = timer_GetSafe(1, TIMER_UP) - start;
      if (elapsed >= SPLASH_TIMEOUT_SEC)
        dismissed = true;
    }

    // switch to double-buffered mode for the main loop
    gfx_SetDrawBuffer();

    // reset palette
    gfx_SetDefaultPalette(gfx_8bpp);
  }
}