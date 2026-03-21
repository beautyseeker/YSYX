#define SDL_malloc  malloc
#define SDL_free    free
#define SDL_realloc realloc

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"

SDL_Surface* IMG_Load_RW(SDL_RWops *src, int freesrc) {
  assert(src->type == RW_TYPE_MEM);
  assert(freesrc == 0);
  return NULL;
}

SDL_Surface* IMG_Load(const char *filename) {
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    fprintf(stderr, "Failed to open file: %s\n", filename);
    return NULL;
  }
    // 获取文件大小
  fseek(fp, 0, SEEK_END);
  long fsize = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  unsigned char *buffer = malloc(fsize);
  if (!buffer) {
    fprintf(stderr, "Failed to allocate memory for file: %s\n", filename);
    fclose(fp);
    return NULL;
  }
  
  size_t read_bytes = fread(buffer, 1, fsize, fp);
  if (read_bytes != fsize) {
    fprintf(stderr, "Failed to read file: %s\n", filename);
    free(buffer);
    fclose(fp);
    return NULL;
  }
  SDL_Surface *surface = STBIMG_LoadFromMemory(buffer, fsize);
  free(buffer);
  fclose(fp);
  return surface;
}

int IMG_isPNG(SDL_RWops *src) {
  return 0;
}

SDL_Surface* IMG_LoadJPG_RW(SDL_RWops *src) {
  return IMG_Load_RW(src, 0);
}

char *IMG_GetError() {
  return "Navy does not support IMG_GetError()";
}
