#include <common.h>
#include <am.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

#define CHECK_BUF(buf, len) \
  do { \
    if (buf == NULL || len <= 0) { \
      printf("Invalid buffer or length in %s\n", __func__); \
      return 0; \
    } \
  } while (0)

#define CHECK_VALID(validcond) \
  do { \
    if (!(validcond)) { \
      printf("Invalid parameter in %s\n", __func__); \
      assert(0); \
    } \
  } while (0)

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

int fb_size = 0;
int window_w = 0, window_h = 0;

size_t serial_write(const void *buf, size_t offset, size_t len) {
  CHECK_BUF(buf, len);
  yield();
  size_t i;
  for (i = 0; i < len; i++) {
    putch(((char *)buf)[i]);
  }
  return i;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  CHECK_BUF(buf, len);
  yield();
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  // printf("events_read: keycode = %d, keydown = %d, buf = %p, len = %d\n", 
  //   ev.keycode, ev.keydown, buf, len);
  if (ev.keycode == AM_KEY_NONE) {
    return 0;
  }

  const char *keytype = ev.keydown ? "kd" : "ku";
  const char *key_name = keyname[ev.keycode];
  size_t read_bytes = snprintf(buf, len, "%s %s", keytype, key_name);
  return read_bytes;
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  CHECK_BUF(buf, len);
  AM_GPU_CONFIG_T cfg = io_read(AM_GPU_CONFIG);
  window_h = cfg.height;
  window_w = cfg.width;
  printf("fb parameter initialized by dispinfo_read:\
  window_w = %d, window_h = %d, fb_size = %d\n", window_w, window_h, fb_size);
  size_t read_bytes = snprintf(buf, len, "WIDTH : %d\nHEIGHT:%d\n",
     cfg.width, cfg.height);
  return read_bytes;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  CHECK_BUF(buf, len);
  CHECK_VALID(window_h != 0 && window_w != 0);
  yield();

  if (offset >= fb_size) return 0;
  if (offset + len > fb_size) {
      len = fb_size - offset;
      printf("Truncation Warning: write fb out of fb size,\
      len = %d, offset = %d, fb_size = %d\n", len, offset, fb_size);
  }
  int pixel_idx = offset / sizeof(uint32_t);
  int rect_x = pixel_idx % window_w;
  int rect_y = pixel_idx / window_w;
  int rect_w = len / sizeof(uint32_t);
  int rect_h = 1;

  io_write(AM_GPU_FBDRAW,
    .x = rect_x, .y = rect_y,
    .w = rect_w, .h = rect_h,
    .sync = true,
    .pixels = (void *)buf
  );
  return len;
}

size_t fb_ctl_write(const void *buf, size_t offset, size_t len) {
  CHECK_BUF(buf, len);
  char *p = (char *)buf;
  window_w = 0;
  while(*p != ' ') {
    window_w = window_w * 10 + (*p - '0');
    p++;
  }
  p++; // skip the space
  window_h = 0;
  while(*p != '\0') {
    window_h = window_h * 10 + (*p - '0');
    p++;
  }
  fb_size = window_w * window_h * sizeof(uint32_t);
  printf("fb parameter initialized by fb_ctl_write:\
     window_w = %d, window_h = %d, fb_size = %d\n",
  window_w, window_h, fb_size);
  return len;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
