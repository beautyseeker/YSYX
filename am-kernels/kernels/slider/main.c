#include <am.h>
#include <klib.h>
#include <klib-macros.h>

extern uint32_t image[][400][300];
extern uint32_t image_end[][400][300];
#define NR_IMG (image_end - image)

void display_image(int i) {
  io_write(AM_GPU_FBDRAW, 0, 0, &image[i][0][0], 400, 300, true);
}

int main() {
  ioe_init();
  io_read(AM_GPU_CONFIG);

  int i = 0;
  unsigned long last = 0;
  unsigned long current;

  display_image(i);
  bool has_kbd = io_read(AM_INPUT_CONFIG).present;

  while (1) {
    current = io_read(AM_TIMER_UPTIME).us / 1000;
    if (current - last > 5000) {
      // change image every 5s
      i = (i + 1) % NR_IMG;
      display_image(i);
      last = current;
    }
    if (has_kbd) {
      AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
      if (ev.keycode == AM_KEY_NONE) continue;
      if (ev.keycode == AM_KEY_ESCAPE && ev.keydown) break; // exit on ESC
      printf("Got  (kbd): %d %s\n", ev.keycode, ev.keydown ? "DOWN" : "UP");
    }
  }
  return 0;
}
