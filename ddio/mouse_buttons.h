#ifndef D3_MOUSE_BUTTONS_H
#define D3_MOUSE_BUTTONS_H

// SDL physical buttons: left=1, middle=2, right=3, then side buttons.
// Engine slots 4/5 (zero-based) are reserved for wheel up/down.
constexpr int EngineMouseButtonIndex(unsigned sdl_button) {
  switch (sdl_button) {
  case 1: return 0;
  case 2: return 2;
  case 3: return 1;
  case 4: return 3;
  case 5: return 6;
  case 6: return 7;
  default: return -1;
  }
}
#endif
