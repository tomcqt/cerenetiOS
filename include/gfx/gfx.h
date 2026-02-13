#ifndef GFX_H
#define GFX_H

#include <graphx.h>
#include <cstdint>
#include "colors.h"

namespace gfx {
  namespace Color {
    constexpr uint8_t Black      = CLR_BLACK;
    constexpr uint8_t White      = CLR_WHITE;
    constexpr uint8_t DarkGray   = CLR_GRAY;
    constexpr uint8_t LightGray  = CLR_L_GRAY;
    constexpr uint8_t Blue       = CLR_CYAN;
    constexpr uint8_t Green      = CLR_LIME;
    constexpr uint8_t Red        = CLR_RED;
    constexpr uint8_t Highlight  = CLR_A_CYAN; // light blue
  }

  void init();

  // utility drawing functions
  void draw_rect_filled(int x, int y, int w, int h, uint8_t color);
  void draw_text(const char* str, unsigned int x, uint8_t y, uint8_t fg);
  void draw_text_small(const char* str, unsigned int x, uint8_t y, uint8_t fg);
}

#endif