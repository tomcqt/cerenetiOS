#include "ui/overlay.h"
#include "config.h"

#include <graphx.h>
#include <sys/lcd.h>

namespace ui::Overlay {
  void draw_dimmed() {
    // TODO: properly implement
    gfx_FillScreen(gfx::Color::Black);
  }
}