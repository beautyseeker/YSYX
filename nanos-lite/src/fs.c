#include <fs.h>
#include <common.h>

extern size_t serial_write(const void *buf, size_t offset, size_t len);
extern size_t events_read(void *buf, size_t offset, size_t len);

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
#include "files.h"
};

void init_fs() {
  // TODO: initialize the size of /dev/fb
}

Finfo* get_file_table() {
  return file_table;
}

int fs_open(const char *pathname, int flags, int mode) {
  Log("fs_open: pathname = %s, flags = %d, mode = %d", pathname, flags, mode);
  for (size_t i = 0; i < sizeof(file_table) / sizeof(file_table[0]); i++) {
    if (strcmp(pathname, file_table[i].name) == 0) {
      return i;
    }
  }
  printf("File open failed: cannot find file '%s'\n", pathname);
  panic("cannot find file: %s", pathname);
  return -1;
}

size_t fs_read(int fd, void *buf, size_t count) {
  assert(fd >= 0 && fd < sizeof(file_table) / sizeof(file_table[0])\
   && fd != FD_STDOUT && fd != FD_STDERR && "fs_read assert failed");
  // fd为标准输入TODO
  Finfo *f = &file_table[fd];
  size_t offset = f->open_offset;
  size_t size = f->size;
  size_t ret = 0;
  if(offset + count > size) {
    printf("Truncation Warning: read file:%s out of file size, \
    count = %d, offset = %d, size = %d\n", f->name, count, offset, size);
    count = size - offset;
  }
  ret = (f->read == NULL) ? \
  ramdisk_read(buf, f->disk_offset + offset, count) :\
  f->read(buf, offset, count);

  f->open_offset += ret;
  return ret;
}

size_t fs_write(int fd, const void *buf, size_t count) {
  assert(fd >= 0 && fd < sizeof(file_table) / sizeof(file_table[0])\
   && fd != FD_STDIN && "fs_write assert failed");
  if(fd == FD_STDOUT || fd == FD_STDERR) {
    return serial_write(buf, 0, count);
  }
  Finfo *f = &file_table[fd];
  size_t offset = f->open_offset;
  size_t size = f->size;
  size_t ret = 0;
  if(offset + count > size) {
    printf("Truncation Warning: write file:%s out of file size, \
    count = %d, offset = %d, size = %d\n", f->name, count, offset, size);
    count = size - offset;
  }
  ret = (f->write == NULL) ? \
  ramdisk_write(buf, f->disk_offset + offset, count) :\
  f->write(buf, offset, count);
  f->open_offset += ret;
  return ret;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  if (fd == 0 || fd == 1 || fd == 2) {
    // 标准输入输出不支持lseek，返回错误
    printf("File lseek failed: fd %d does not support lseek\n", fd);
    return -1;
  }
  int new_pos;
  switch (whence) {
    case SEEK_SET: new_pos = offset; break;
    case SEEK_CUR: new_pos = file_table[fd].open_offset + offset; break;
    case SEEK_END: new_pos = file_table[fd].size + offset; break;
    default: new_pos = file_table[fd].open_offset;
  }

  if (new_pos < 0) {
    return -1;
  }

  file_table[fd].open_offset = new_pos;
  return new_pos;
}

int fs_close(int fd) {
  // 目前不需要真正关闭文件，直接返回成功
  return 0;
}
