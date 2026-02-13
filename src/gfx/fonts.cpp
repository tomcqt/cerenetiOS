#include "gfx/fonts.h"

static const uint8_t fnt_small_data[] = {
  #include "fonts/small.inc"
};

const fontlib_font_t *fnt_small = (const fontlib_font_t *)fnt_small_data;

static const uint8_t fnt_main_data[] = {
  #include "fonts/main.inc"
};

const fontlib_font_t *fnt_main = (const fontlib_font_t *)fnt_main_data;