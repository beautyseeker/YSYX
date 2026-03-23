#include <am.h>
#include <NDL.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

static const char *keyname[] = {
  "NONE",
  AM_KEYS(AM_KEY_NAMES)
};

bool ioe_init() {
  return NDL_Init(0);
}

void ioe_read (int reg, void *buf) { 
  switch (reg) {
    case AM_TIMER_UPTIME: {
      AM_TIMER_UPTIME_T *uptime = (AM_TIMER_UPTIME_T *)buf;
      uptime->us = NDL_GetTicks() * 1000; // 转换为微秒
      break;
    }
    case AM_INPUT_KEYBRD: {
      AM_INPUT_KEYBRD_T *kbd = (AM_INPUT_KEYBRD_T *)buf;
      char event[64];
      char keydown_str[8];
      char keycode_str[16];
      int len = NDL_PollEvent(event, sizeof(event));
      if (len > 0) {
        if(sscanf(event, "%s %s", keydown_str, keycode_str) == 2) {
          kbd->keydown = (strcmp(keydown_str, "kd") == 0);
          kbd->keycode = AM_KEY_NONE; // 默认值
          for(int i = 0; i < sizeof(keyname) / sizeof(keyname[0]); i++) {
            if (strcmp(keycode_str, keyname[i]) == 0) {
              kbd->keycode = i;
              break;
            }
          }
        } 
      }
      else {
        kbd->keydown = false;
        kbd->keycode = AM_KEY_NONE;
      }

      break;
    }
    case AM_GPU_CONFIG: {
      AM_GPU_CONFIG_T *cfg = (AM_GPU_CONFIG_T *)buf;
      NDL_OpenCanvas(&cfg->width, &cfg->height);  
      cfg->present = true;
      cfg->has_accel = false;
      cfg->vmemsz = cfg->width * cfg->height * sizeof(uint32_t); // 每个像素4字节
      break;
    }

    default:
      printf("ioe_read: unsupported reg %d", reg);
      assert(0 && "unsupported reg");
  }
}
void ioe_write(int reg, void *buf) { 
  switch (reg) {
    case AM_GPU_FBDRAW: {
      AM_GPU_FBDRAW_T *ctl = (AM_GPU_FBDRAW_T *)buf;
      NDL_DrawRect(ctl->pixels, ctl->x, ctl->y, ctl->w, ctl->h);
      break;
    }
    default:
      printf("ioe_write: unsupported reg %d", reg);
      assert(0 && "unsupported reg");
  }
}
