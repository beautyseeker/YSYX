#include <NDL.h>
#include <SDL.h>
#include <string.h>

#define keyname(k) #k,

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}

int SDL_PollEvent(SDL_Event *ev) {
  char evt_buf[64];
  if(NDL_PollEvent(evt_buf, sizeof(evt_buf)) == 0) {
    return 0; // 没有事件
  }
  char evt_val[64];
  char evt_type[16];
  // 解析事件字符串，格式为 "kd KEYNAME" 或 "ku KEYNAME"
  if(sscanf(evt_buf, "%s %s", evt_type, evt_val) == 2) {
    // printf("Parsed event type = %s, value = %s\n", evt_type, evt_val);
    ev->key.keysym.sym = 0; // 默认值
    for (int i = 0; i < sizeof(keyname) / sizeof(keyname[0]); i++) {
      if (strcmp(evt_val, keyname[i]) == 0) {
        ev->key.keysym.sym = i;
        break;
      }
    }
    if (strcmp(evt_type, "kd") == 0) {
      ev->type = SDL_KEYDOWN;
    } else if (strcmp(evt_type, "ku") == 0) {
      ev->type = SDL_KEYUP;
    } else {
      ev->type = 0; // 未知事件类型
    }
    evt_buf[0] = '\0'; // 清空事件缓冲区
    return 1;
  }
  printf("Failed to parse event string: %s\n", evt_buf);
  return 0;
}

int SDL_WaitEvent(SDL_Event *event) {
  if(event == NULL) return 0;
  while (1) {
    if (SDL_PollEvent(event)) {
      return 1;
    }
  }
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  return NULL;
}
