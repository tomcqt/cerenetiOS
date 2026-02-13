#include <graphx.h>
#include <keypadc.h>
#include <fileioc.h>
#include <fontlibc.h>
#include <sys/lcd.h>
#include <sys/timers.h>
#include <sys/rtc.h>
#include <sys/power.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "gfx/gfx.h"
#include "hal/input.h"
#include "ui.h"
#include "gfx/splash.h"

// TODO: write first boot screen
// TODO: state saving
// TODO: write apps
// TODO: find a better way for a todo list

int main(void) {
  // --- init subsystems ---
  gfx_Begin();

  // --- show splash screen ---
  gfx::splash::show();

  kb_EnableOnLatch(); // allow [on] key to be detected
  timer_Enable(1, TIMER_32K, TIMER_NOINT, TIMER_UP);

  gfx::init();
  hal::input::init();
  ui::init();

  // --- main os loop ---
  bool running = true;
  while (running) {
    // poll keypad
    kb_Scan();
    hal::input::update();

    // exit condition: [on] + [mode]
    if (kb_On && kb_IsDown(kb_KeyMode)) {
      running = false;
      continue;
    }

    // update ui state
    ui::update();

    // render frame
    gfx_FillScreen(0x00); // clear screen w/ black
    ui::draw();
    gfx_SwapDraw(); // flip buffer
  }

  // --- shutdown ---
  timer_Disable(1);
  kb_DisableOnLatch();
  gfx_End();

  return 0;
}