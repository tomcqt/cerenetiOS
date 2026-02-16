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
#include "ui/ui.h"
#include "gfx/splash.h"
#include "ui/overlay.h"
#include "os/app.h"

#include "modules/calc.h"

// --- app registry ---

// index must match all_apps[] in ui/ui.cpp
static const AppInterface* app_registry[] = {
  &calc_app, //  0: Calc
  nullptr,   //  1: Graph    (not implemented)
  nullptr,   //  2: Programs (not implemented)
  nullptr,   //  3: Files    (not implenented)
  nullptr,   //  4: Notes    (not implemented)
  nullptr,   //  5: Matrix   (not implemented)
  nullptr,   //  6: Stats    (not implemented)
  nullptr,   //  7: Table    (not implemented)
  nullptr,   //  8: Settings (not implemented)
  nullptr,   //  9: Clock    (not implemented)
  nullptr,   // 10: Memory   (not implemented)
  nullptr,   // 11: About    (not implemented)
};
static constexpr int APP_REGISTRY_COUNT = sizeof(app_registry);

// --- os state ---

static int current_app = 0; // start with calculator
static bool overlay_active = false;

static void switch_app(int app_idx) {
  if (app_idx < 0 || app_idx >= APP_REGISTRY_COUNT) return;
  if (app_registry[app_idx] == nullptr) return;

  // shutdown old app
  if (app_registry[current_app] != nullptr)
    app_registry[current_app] -> shutdown();
  
  current_app = app_idx;
  app_registry[current_app] -> init();
}

// --- main ---

int main(void) {
  gfx_Begin();
  
  kb_EnableOnLatch();
  timer_Enable(1, TIMER_32K, TIMER_NOINT, TIMER_UP);

  gfx::init();
  hal::Input::init();

  gfx::Splash::show();

  gfx_SetDrawBuffer();

  // start default app (calculator)
  app_registry[current_app] -> init();

  // os main loop
  bool running = true;
  while (running) {
    kb_Scan();
    hal::Input::update();

    if (kb_On && kb_IsDown(kb_KeyMode)) {
      running = false;
      continue;
    }

    if (hal::Input::key_pressed(1, kb_Mode)) {
      if (!overlay_active) {
        if (app_registry[current_app] != nullptr)
          app_registry[current_app] -> tick();
        gfx_SwapDraw();

        ui::init();
        overlay_active = true;
        continue;
      } else {
        // close overlay
        overlay_active = false;
        continue;
      }
    }

    if (overlay_active) {
      // overlay mode
      ui::update();

      int sel = ui::get_selected_app();
      if (sel >= 0) {
        // user picked an app
        if (sel != current_app && app_registry[sel] != nullptr)
          switch_app(sel);

        overlay_active = false;
        do { kb_Scan(); } while (kb_AnyKey());
        continue;
      }

      if (ui::dismissed()) {
        overlay_active = false;
        do { kb_Scan(); } while (kb_AnyKey());
        continue;
      }

      // draw dimmed background + overlay ui
      ui::Overlay::draw_dimmed();
      ui::draw();
      gfx_SwapDraw();
    } else {
      // active app mode
      if (app_registry[current_app] != nullptr)
        app_registry[current_app] -> tick();
      
      gfx_SwapDraw();
    }
  }

  // shutdown
  if (app_registry[current_app] != nullptr)
    app_registry[current_app] -> shutdown();

  timer_Disable(1);
  kb_DisableOnLatch();
  gfx_End();

  return 0;
}