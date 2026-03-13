#ifndef __FS_H__
#define __FS_H__

#include <common.h>

#ifndef SEEK_SET
enum {SEEK_SET, SEEK_CUR, SEEK_END};
#endif

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  size_t open_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB};

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
void init_ramdisk();
size_t get_ramdisk_size();
Finfo* get_file_table();

int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t count);
size_t fs_write(int fd, const void *buf, size_t count);
size_t fs_lseek(int fd, size_t offset, int whence);
int fs_close(int fd);

#endif
