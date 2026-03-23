#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>
#include <fcntl.h>

static int evtdev = -1;
static int fbdev = -1;
static int fbctl = -1;
static int dispinfo = -1;
static int screen_w = 0, screen_h = 0;
static int canvas_w = 0, canvas_h = 0;
static int offset_x = 0, offset_y = 0;
static struct timeval now;

uint32_t NDL_GetTicks() {
  int ret = gettimeofday(&now, NULL);
  if(ret != 0) {
    printf("gettimeofday failed in NDL_GetTicks\n");
    return -1;
  }
  uint32_t ms = (now.tv_sec * 1000) + (now.tv_usec / 1000); // 毫秒部分
  return ms;
}

int NDL_PollEvent(char *buf, int len) {
  if(buf == NULL || len <= 0) {
    return 0;
  }
  if(evtdev < 0) {
    printf("evtdev not initialized in NDL_PollEvent\n");
    return 0;
  }
  int ret = read(evtdev, buf, len-1);
  if (ret > 0) {
    buf[ret] = '\0'; // 确保字符串以 null 结尾
    return ret;
  }
  return 0;
}

void NDL_OpenCanvas(int *w, int *h) {
  if (getenv("NWM_APP")) {
    fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }
  else {
    char buf[64];
    int nread = read(dispinfo, buf, sizeof(buf) - 1);
    if (nread <= 0) {
      printf("Failed to read dispinfo in NDL_OpenCanvas\n");
      exit(1);
    }
    buf[nread] = '\0';
    sscanf(buf, "WIDTH : %d\nHEIGHT:%d", &screen_w, &screen_h);
  }
  if (screen_w == 0 || screen_h == 0) {
      // 如果解析失败，给一个保底的物理分辨率（根据你的 NEMU 配置）
      printf("Failed to scanf screen size, using default 400x300\n");
      screen_w = 400; screen_h = 300; 
  }

  if (*w == 0 && *h == 0) {
    *w = screen_w; *h = screen_h;
  }
  canvas_w = *w; canvas_h = *h;

  // 2. 计算居中偏移量
  offset_x = (screen_w - canvas_w) / 2;
  offset_y = (screen_h - canvas_h) / 2;

  printf("NDL_OpenCanvas: screen_w = %d, screen_h = %d\n", screen_w, screen_h);
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  if(pixels == NULL || x < 0 || y < 0 || w < 0 || h < 0) {
    printf("Invalid parameters for NDL_DrawRect: pixels = %p, x = %d, y = %d, w = %d, h = %d\n",
           pixels, x, y, w, h);
    assert(0 && "invalid NDL_DrawRect parameters");
  }
  if(fbdev < 0) {
    printf("fbdev not initialized in NDL_DrawRect\n");
    assert(0 && "fbdev not initialized");
  }

  for (int i = 0; i < h; i++) {
    // 1. 计算这一行在显存中的起始位置（单位：像素）
    // 假设渲染位置需要加上画布本身的偏移 (canvas_x, canvas_y)
    int draw_x = x + offset_x;
    int draw_y = y + i + offset_y;
    uintptr_t offset = (draw_y * screen_w + draw_x) * sizeof(uint32_t);

    // 2. 移动文件指针到这一行的开头
    lseek(fbdev, offset, SEEK_SET);

    // 3. 写入这一行的 w 个像素
    // 注意：pixels 指针也要跟着移动，指向当前要画的那一行的开头
    write(fbdev, pixels + i * w, w * sizeof(uint32_t));
  }
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  else {
    evtdev = open("/dev/events", 0, 0);
    if(evtdev < 0) {
      printf("Failed to open /dev/events in NDL_Init\n");
      return -1;
    }
    fbctl = open("/dev/fbctl", 0, 0);
    if(fbctl < 0) {
      printf("Failed to open /dev/fbctl in NDL_Init\n");
      return -1;
    }
    fbdev = open("/dev/fb", 0, 0);
    if(fbdev < 0) {
      printf("Failed to open /dev/fb in NDL_Init\n");
      return -1;
    }
    dispinfo = open("/proc/dispinfo", 0, 0);
    if(dispinfo < 0) {
      printf("Failed to open /proc/dispinfo in NDL_Init\n");
      return -1;
    }
  }
  printf("NDL_Init Finish: evtdev = %d, fbctl = %d, fbdev = %d, dispinfo = %d\n",
  evtdev, fbctl, fbdev, dispinfo);
  fflush(stdout);
  return 0;
}

void NDL_Quit() {
}
