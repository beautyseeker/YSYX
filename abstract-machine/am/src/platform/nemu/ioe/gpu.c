#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

static int win_w, win_h;

void __am_gpu_init() {
  int i;
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  for (i = 0; i < win_w * win_h; i ++) fb[i] = i;
  outl(SYNC_ADDR, 1);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  uint32_t info = inl(VGACTL_ADDR);
  win_w = info >> 16;
  win_h = info & 0xffff;
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = win_w, .height = win_h,
    .vmemsz = win_w * win_h * sizeof(uint32_t)
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {

  for(int i = 0; i < ctl->h; i ++) {
    uint32_t *dst = (uint32_t *)(uintptr_t)(FB_ADDR + ((ctl->y + i) * win_w + ctl->x) * sizeof(uint32_t));
    uint32_t *src = (uint32_t *)(ctl->pixels + i * ctl->w * sizeof(uint32_t));
    for(int j = 0; j < ctl->w; j ++) dst[j] = src[j];
  }
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
