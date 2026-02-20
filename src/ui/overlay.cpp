#include "ui/overlay.h"
#include "gfx/colors.h"

#include <sys/lcd.h>
#include <graphx.h>

static constexpr int DS_W = LCD_WIDTH / 2;
static constexpr int DS_H = LCD_HEIGHT / 2;
static uint8_t snapshot[DS_W * DS_H];

namespace ui::Overlay {

  void capture() {
    uint8_t* src = (uint8_t*)gfx_vram;

    for (int y = 0; y < DS_H; y++) {
      int sr = y * 2 * LCD_WIDTH;
      int dr = y * DS_W;
      for (int x = 0; x < DS_W; x++)
        snapshot[dr + x] = PAL_DIM[src[sr + x * 2]];
    }
  }

  void draw() {
    uint8_t* dst = (uint8_t*)gfx_vbuffer;

    for (int y = 0; y < DS_H; y++) {
      uint8_t* s = &snapshot[y * DS_W];
      uint8_t* d0 = &dst[(y * 2) * LCD_WIDTH];
      uint8_t* d1 = d0 + LCD_WIDTH;

      for (int x = 0; x < DS_W; x++) {
        uint8_t c = s[x];
        int dx = x * 2;
        d0[dx] = c; d0[dx+1] = c;
        d1[dx] = c; d1[dx+1] = c;
      }
    }
  }

}