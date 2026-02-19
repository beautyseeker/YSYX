#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t keycode = inl(KBD_ADDR);
  if (keycode == AM_KEY_NONE) {
    kbd->keydown = false;
    kbd->keycode = AM_KEY_NONE;
  }
  else {
    kbd->keycode = keycode & ~KEYDOWN_MASK;
    kbd->keydown = (keycode & KEYDOWN_MASK) != 0;
  }
}
