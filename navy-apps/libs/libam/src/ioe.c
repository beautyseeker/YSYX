#include <am.h>
#include <NDL.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#define STR(k) #k,
static const char *keyname[] = {
  "NONE",
  AM_KEYS(STR)
};

static int screen_w = 0, screen_h = 0;

bool ioe_init() {
  bool ret = NDL_Init(0);
    // 在初始化时就确定屏幕大小
    NDL_OpenCanvas(&screen_w, &screen_h);
    return ret;
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
      cfg->width = screen_w;
      cfg->height = screen_h;
      cfg->present = true;
      cfg->has_accel = false;
      cfg->vmemsz = cfg->width * cfg->height * sizeof(uint32_t); // 每个像素4字节
      break;
    }

    case AM_INPUT_CONFIG: {
      AM_INPUT_CONFIG_T *cfg = (AM_INPUT_CONFIG_T *)buf;
      cfg->present = true;
      break;
    }

    case AM_TIMER_CONFIG: {
      AM_TIMER_CONFIG_T *cfg = (AM_TIMER_CONFIG_T *)buf;
      cfg->present = true;
      cfg->has_rtc = true;
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
      // 只有当 w, h 不为 0 时才调用绘图
      if (ctl->w > 0 && ctl->h > 0) {
        NDL_DrawRect(ctl->pixels, ctl->x, ctl->y, ctl->w, ctl->h);
      }
      // 关键：处理同步信号
      if (ctl->sync) {
        // 如果你的 NDL 有显式的同步接口，在这里调用
        // 或者 NDL_DrawRect 内部已经处理了同步。
        // 在某些实现中，NDL_RenderPresent() 或是类似的
      }
      break;
    }
  }
}
