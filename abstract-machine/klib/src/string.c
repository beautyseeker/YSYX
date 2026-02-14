#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

char *strcpy(char *dst, const char *src) {
  size_t i = 0;
  while(src[i] != '\0') {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i;
  for (i = 0; i < n && src[i] != '\0'; i++) {
    dst[i] = src[i];
  }
  for (; i < n; i++) {
    dst[i] = '\0';
  }
  return dst;
}

char *strcat(char *dst, const char *src) {
  size_t dst_len = strlen(dst);
  size_t i;
  for (i = 0; src[i] != '\0'; i++) {
    dst[dst_len + i] = src[i];
  }
  dst[dst_len + i] = '\0';
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  size_t i = 0;
  while (s1[i] == s2[i]) {
    if (s1[i] == '\0') return 0;
    i++;
  }
  return (unsigned char)s1[i] - (unsigned char)s2[i];
}


int strncmp(const char *s1, const char *s2, size_t n) {
  if (n == 0) return 0;
  
  size_t i = 0;
  while (i < n && s1[i] == s2[i]) {
    if (s1[i] == '\0' || i == n - 1) break; 
    i++;
  }
  
  if (i == n) return 0;
  return (unsigned char)s1[i] - (unsigned char)s2[i];
}

void *memset(void *s, int c, size_t n) {
  size_t i;
  for (i = 0; i < n; i++) {
    ((unsigned char *)s)[i] = (unsigned char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  size_t i;
  if (dst < src) {
    for (i = 0; i < n; i++) {
      ((unsigned char *)dst)[i] = ((const unsigned char *)src)[i];
    }
  } else {
    for (i = n; i > 0; i--) {
      ((unsigned char *)dst)[i - 1] = ((const unsigned char *)src)[i - 1];
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  size_t i;
  for (i = 0; i < n; i++) {
    ((unsigned char *)out)[i] = ((const unsigned char *)in)[i];
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  size_t i = 0;
  while(i < n && ((unsigned char *)s1)[i] == ((unsigned char *)s2)[i]) {
    i++;
  }
  return (i == n) ? 0 : ((unsigned char *)s1)[i] - ((unsigned char *)s2)[i];
}

#endif
