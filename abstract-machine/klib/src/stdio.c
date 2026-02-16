#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  // panic("Not implemented");
  return 0;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  char *p = out;
  const char *f = fmt;
  while (*f != '\0') {
    if (*f == '%') { 
      f++;
      if (*f == 'd') {
        int num = va_arg(ap, int);
        char buf[12]; // enough to hold -2147483648 and '\0'
        int i = 0;
        if (num < 0) {
          *p++ = '-';
          num = -num;
        }
        do{
          buf[i++] = (char)('0' + num % 10);
          num /= 10;
        } while (num > 0);
        
        for (int j = i - 1; j >= 0; j--) {
          *p++ = buf[j];
        }
      }
      else if (*f == 's') {
        char *str = va_arg(ap, char *);
        strcpy(p, str);
        p += strlen(str);
      }
      else if (*f == 'c') {
        char c = (char)va_arg(ap, int);
        *p++ = c;
      }
      else if (*f == '%') {
        *p++ = '%';
      }
      else {
        // Unsupported format specifier, just ignore it
      }
    }
    else {
      *p++ = *f;
    }
    f++;
  }
  *p = '\0';
  return p - out;
}


int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(out, fmt, ap);
  va_end(ap);

  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return ret;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  char *p = out;
  const char *f = fmt;
  size_t written = 0;
  while (*f != '\0' && written < n - 1) {
    if (*f == '%') {
      f++;
      if (*f == 'd') {
        int num = va_arg(ap, int);
        unsigned int unum;
        char buf[12]; // enough to hold -2147483648 and '\0'
        int i = 0;
        if (num < 0) {
          if (written < n - 1) {
            *p++ = '-';
            written++;
          }
          unum = (unsigned int)-(num+1) + 1;
        }
        else {
          unum = (unsigned int)num;
        }
        
        do{
          buf[i++] = (char)('0' + unum % 10);
          unum /= 10;
        } while (unum > 0);
        
        for (int j = i - 1; j >= 0; j--) {
          if (written < n - 1) {
            *p++ = buf[j];
            written++;
          }
        }
      }
      else if (*f == 's') {
        char *str = va_arg(ap, char *);
        while (*str != '\0' && written < n - 1) {
          *p++ = *str++;
          written++;
        }
      }
      else if (*f == 'c') {
        char c = (char)va_arg(ap, int);
        if (written < n - 1) {
          *p++ = c;
          written++;
        }
      }
      else if (*f == '%') {
        if (written < n - 1) {
          *p++ = '%';
          written++;
        }
      }
      else {
        // Unsupported format specifier, just ignore it
      }
    }
    else {
      if (written < n - 1) {
        *p++ = *f;
        written++;
      }
    }
    f++;
  }
  *p = '\0';
  return written;
}

#endif
