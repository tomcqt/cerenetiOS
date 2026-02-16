#include "hal/input.h"
#include <cstring>

namespace hal::Input {
  // kb_Data has 8 groups (indices 0-7)
  static uint8_t prev_state[8];
  static uint8_t curr_state[8];

  void init() {
    memset(prev_state, 0, sizeof(prev_state));
    memset(curr_state, 0, sizeof(curr_state));
  }

  void update() {
    // save previous state before scanning
    memcpy(prev_state, curr_state, sizeof(curr_state));
    
    // copy current keypad data
    for (int i = 1; i <= 7; i++) {
      curr_state[i] = kb_Data[i];
    }
  }

  bool key_pressed(uint8_t group, uint8_t key) {
    // true if down now but wasnt last frame
    return (curr_state[group] & key) && !(prev_state[group] & key);
  }

  bool key_held(uint8_t group, uint8_t key) {
    return (curr_state[group] & key);
  }

  // --- conveniece wrappers ---
  
  bool key_up()    { return key_pressed(7, kb_Up);    }
  bool key_down()  { return key_pressed(7, kb_Down);  }
  bool key_left()  { return key_pressed(7, kb_Left);  }
  bool key_right() { return key_pressed(7, kb_Right); }
  bool key_enter() { return key_pressed(6, kb_Enter); }
  bool key_clear() { return key_pressed(6, kb_Clear); }
  bool key_2nd()   { return key_pressed(1, kb_2nd);   }
  bool key_alpha() { return key_pressed(2, kb_Alpha); }
}