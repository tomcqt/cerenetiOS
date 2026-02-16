#ifndef INPUT_H
#define INPUT_H

#include <keypadc.h>
#include <cstdint>

namespace hal::Input {

  void init();
  void update();

  // returns true only on the frame the key is first pressed (edge-detect)
  bool key_pressed(uint8_t group, uint8_t key);

  // returns true as long as the key is held
  bool key_held(uint8_t group, uint8_t key);

  // convenience wrappers for common keys
  bool key_up();
  bool key_down();
  bool key_left();
  bool key_right();
  bool key_enter();
  bool key_clear();
  bool key_2nd();
  bool key_alpha();

}

#endif