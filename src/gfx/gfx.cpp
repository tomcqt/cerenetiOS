#include "gfx/gfx.h"
#include "gfx/fonts.h"

#include <fontlibc.h>

namespace gfx {
  void init() {
    // load fontlibc and custom smallfont
    fontlib_SetWindow(0, 0, GFX_LCD_WIDTH, GFX_LCD_HEIGHT);
    fontlib_SetTransparency(true);
    fontlib_SetLineSpacing(0, 0);
  }

  void draw_rect_filled(int x, int y, int w, int h, uint8_t color) {
    gfx_SetColor(color);
    gfx_FillRectangle(x, y, w, h);
  }

  void draw_text(const char* str, unsigned int x, uint8_t y, uint8_t fg) {
    fontlib_SetFont(fnt_main, (fontlib_load_options_t)0);
    fontlib_SetForegroundColor(fg);
    fontlib_SetTransparency(true);
    fontlib_SetCursorPosition(x, y - 2);
    fontlib_DrawString(str);
  }

  void draw_text_small(const char* str, unsigned int x, uint8_t y, uint8_t fg) {
    fontlib_SetFont(fnt_small, (fontlib_load_options_t)0); // stupid hack for font loading because 0 didnt work
    fontlib_SetForegroundColor(fg);
    fontlib_SetTransparency(true);
    fontlib_SetCursorPosition(x, y);
    fontlib_DrawString(str);
  }
}