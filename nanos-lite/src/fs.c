#include <fs.h>
#include <common.h>

extern size_t serial_write(const void *buf, size_t offset, size_t len);
extern size_t events_read(void *buf, size_t offset, size_t len);
extern size_t dispinfo_read(void *buf, size_t offset, size_t len);
extern size_t fb_write(const void *buf, size_t offset, size_t len);
extern size_t fb_ctl_write(const void *buf, size_t offset, size_t len);
extern void init_device();
extern int fb_size, window_w, window_h;

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, 0, invalid_read, serial_write},
  [FD_STDERR] = {"stderr", 0, 0, 0, invalid_read, serial_write},
  [FD_EVT]    = {"/dev/events", 0, 0, 0, events_read, invalid_write},
  [FD_FBCTL]  = {"/dev/fbctl", 0, 0, 0, invalid_read, fb_ctl_write},
  [FD_FBDEV]  = {"/dev/fb", 0, 0, 0, invalid_read, fb_write},
  [FD_DISPINFO] = {"/proc/dispinfo", 0, 0, 0, dispinfo_read, invalid_write},
#include "files.h"
};
#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  // TODO: initialize the size of /dev/fb
  window_h = io_read(AM_GPU_CONFIG).height;
  window_w = io_read(AM_GPU_CONFIG).width;
  fb_size = io_read(AM_GPU_CONFIG).vmemsz;
  printf("init_fb: window_w = %d, window_h = %d, fb_size = %d\n", window_w, window_h, fb_size);
  assert(fb_size > 0 && "fb_size not initialized");
  file_table[FD_FBDEV].size = fb_size;
}

Finfo* get_file_table() {
  return file_table;
}

int fs_open(const char *pathname, int flags, int mode) {
  Log("fs_open: pathname = %s, flags = %d, mode = %d", pathname, flags, mode);
  for (size_t i = 0; i < NR_FILES; i++) {
    if (strcmp(pathname, file_table[i].name) == 0) {
      file_table[i].cur_pos = 0; // 核心：每次打开都回滚到开头
      return i;
    }
  }
  printf("File open failed: cannot find file '%s'\n", pathname);
  panic("cannot find file: %s", pathname);
  return -1;
}

size_t fs_read(int fd, void *buf, size_t count) {
  // printf("filename:%s fs_read: fd = %d, buf = 0x%x, count = %d\n",
  // file_table[fd].name, fd, (uintptr_t)buf, count);
  if(fd < 0 || fd >= NR_FILES || buf == NULL || count == 0) {
    printf("Invalid parameter in %s\n", __func__);
    return 0;
  }
  // fd为标准输入TODO
  Finfo *f = &file_table[fd];
  size_t ret = 0;

  size_t size = f->size;
  if(f->read != NULL) {
    ret = f->read(buf, f->cur_pos, count);
    f->cur_pos += ret;
    return ret;
  }

  // 如果是普通文件越界读取则进行截断处理
  if(f->cur_pos >= size) return 0; // 已经到末尾了
  if(f->cur_pos + count > size) {
    printf("Truncation Warning: read file:%s lseek_ptr %d out of file size = %d,\
    actual read bytes = %d\n", f->name, f->cur_pos + count, size, count);
    count = size - f->cur_pos;
  }
  ret = ramdisk_read(buf, f->disk_pos + f->cur_pos, count);

  f->cur_pos += ret;
  return ret;
}

size_t fs_write(int fd, const void *buf, size_t count) {
  // 1. 基础校验（VFS层）
  if (fd < 0 || fd >= NR_FILES || buf == NULL || count == 0) return 0;
  
  Finfo *f = &file_table[fd];
  size_t ret = 0;
  
  // 2. 如果是设备文件（有自己的 write 函数）
  if (f->write != NULL) {
    // 设备驱动自己决定如何处理 count 和 offset
    // 比如 serial_write 忽略 offset，fb_write 检查越界
    ret = f->write(buf, f->cur_pos, count);
    f->cur_pos += ret;
    return ret;
  }

  // 3. 如果是普通文件越界写入则进行截断处理（ramdisk_write）
  if (f->cur_pos >= f->size) return 0; // 已经到末尾了
  if (f->cur_pos + count > f->size) {
    count = f->size - f->cur_pos; // 截断
    printf("Truncation Warning: write file:%s lseek_ptr %d out of file size = %d,\
    actual write bytes= %d\n", f->name, f->cur_pos + count, f->size, count);
  }

  ret = ramdisk_write(buf, f->disk_pos + f->cur_pos, count);
  f->cur_pos += ret;
  return ret;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  if (fd == FD_STDIN || fd == FD_STDOUT || fd == FD_STDERR) {
    // 标准输入输出不支持lseek，返回错误
    printf("File lseek failed: fd %d does not support lseek\n", fd);
    return -1;
  }
  // if(strcmp(file_table[fd].name, "/share/pictures/projectn.bmp") == 0)
  //   printf("fs_lseek: fd = %d, offset = %d, whence = %d\n", fd, offset, whence);
  int new_pos;
  switch (whence) {
    case SEEK_SET: new_pos = offset; break;
    case SEEK_CUR: new_pos = file_table[fd].cur_pos + offset; break;
    case SEEK_END: new_pos = file_table[fd].size + offset; break;
    default: new_pos = file_table[fd].cur_pos;
  }

  if (new_pos < 0) {
    printf("Warnning Invalid lseek offset in file: %s, new_pos = %d\n", file_table[fd].name, new_pos);
    return -1;
  }
  if (new_pos > file_table[fd].size) new_pos = file_table[fd].size;

  file_table[fd].cur_pos = new_pos;
  return new_pos;
}

int fs_close(int fd) {
  // 目前不需要真正关闭文件，直接返回成功
  return 0;
}
