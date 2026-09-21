#include <am.h>
#include <klib.h>
#include <arch/SoC.h>

/* PS/2 Scan Code Set 2 → AM_KEY_*（未列出为 AM_KEY_NONE） */
static const int keymap[256] = {
  [0x76] = AM_KEY_ESCAPE,
  [0x05] = AM_KEY_F1,  [0x06] = AM_KEY_F2,  [0x04] = AM_KEY_F3,  [0x0c] = AM_KEY_F4,
  [0x03] = AM_KEY_F5,  [0x0b] = AM_KEY_F6,  [0x83] = AM_KEY_F7,  [0x0a] = AM_KEY_F8,
  [0x01] = AM_KEY_F9,  [0x09] = AM_KEY_F10, [0x78] = AM_KEY_F11, [0x07] = AM_KEY_F12,
  [0x0e] = AM_KEY_GRAVE,
  [0x16] = AM_KEY_1, [0x1e] = AM_KEY_2, [0x26] = AM_KEY_3, [0x25] = AM_KEY_4,
  [0x2e] = AM_KEY_5, [0x36] = AM_KEY_6, [0x3d] = AM_KEY_7, [0x3e] = AM_KEY_8,
  [0x46] = AM_KEY_9, [0x45] = AM_KEY_0,
  [0x4e] = AM_KEY_MINUS, [0x55] = AM_KEY_EQUALS, [0x66] = AM_KEY_BACKSPACE,
  [0x0d] = AM_KEY_TAB,
  [0x15] = AM_KEY_Q, [0x1d] = AM_KEY_W, [0x24] = AM_KEY_E, [0x2d] = AM_KEY_R,
  [0x2c] = AM_KEY_T, [0x35] = AM_KEY_Y, [0x3c] = AM_KEY_U, [0x43] = AM_KEY_I,
  [0x44] = AM_KEY_O, [0x4d] = AM_KEY_P,
  [0x54] = AM_KEY_LEFTBRACKET, [0x5b] = AM_KEY_RIGHTBRACKET, [0x5d] = AM_KEY_BACKSLASH,
  [0x58] = AM_KEY_CAPSLOCK,
  [0x1c] = AM_KEY_A, [0x1b] = AM_KEY_S, [0x23] = AM_KEY_D, [0x2b] = AM_KEY_F,
  [0x34] = AM_KEY_G, [0x33] = AM_KEY_H, [0x3b] = AM_KEY_J, [0x42] = AM_KEY_K,
  [0x4b] = AM_KEY_L,
  [0x4c] = AM_KEY_SEMICOLON, [0x52] = AM_KEY_APOSTROPHE, [0x5a] = AM_KEY_RETURN,
  [0x12] = AM_KEY_LSHIFT,
  [0x1a] = AM_KEY_Z, [0x22] = AM_KEY_X, [0x21] = AM_KEY_C, [0x2a] = AM_KEY_V,
  [0x32] = AM_KEY_B, [0x31] = AM_KEY_N, [0x3a] = AM_KEY_M,
  [0x41] = AM_KEY_COMMA, [0x49] = AM_KEY_PERIOD, [0x4a] = AM_KEY_SLASH,
  [0x59] = AM_KEY_RSHIFT,
  [0x14] = AM_KEY_LCTRL, [0x11] = AM_KEY_LALT, [0x29] = AM_KEY_SPACE,
};

static int is_release;
static int is_ext; /* 0xe0 扩展前缀 */

static uint8_t ps2_read_raw(void) {
  return (uint8_t)(*(volatile uint32_t *)(PS2_BASE + PS2_KEY_SCAN));
}

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  kbd->keydown = false;
  kbd->keycode = AM_KEY_NONE;

  uint8_t sc = ps2_read_raw();
  if (sc == 0) return;

  /* 断码前缀：下一字节才是键；本次对上层返回「无」 */
  if (sc == 0xf0) {
    is_release = 1;
    return;
  }
  /* 扩展键前缀 */
  if (sc == 0xe0) {
    is_ext = 1;
    return;
  }

  int key = AM_KEY_NONE;
  if (is_ext) {
    switch (sc) {
      case 0x75: key = AM_KEY_UP;       break;
      case 0x72: key = AM_KEY_DOWN;     break;
      case 0x6b: key = AM_KEY_LEFT;     break;
      case 0x74: key = AM_KEY_RIGHT;    break;
      case 0x70: key = AM_KEY_INSERT;   break;
      case 0x71: key = AM_KEY_DELETE;   break;
      case 0x6c: key = AM_KEY_HOME;     break;
      case 0x69: key = AM_KEY_END;      break;
      case 0x7d: key = AM_KEY_PAGEUP;   break;
      case 0x7a: key = AM_KEY_PAGEDOWN; break;
      case 0x14: key = AM_KEY_RCTRL;    break;
      case 0x11: key = AM_KEY_RALT;     break;
      default:   break;
    }
  } else {
    key = keymap[sc];
  }

  is_ext = 0;
  if (key == AM_KEY_NONE) {
    is_release = 0;
    return;
  }

  kbd->keycode = key;
  kbd->keydown = !is_release;
  is_release = 0;
}
