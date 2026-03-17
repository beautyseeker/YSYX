#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[], char *envp[]) {
  write(1, "Hello World!\n", 13);
  int i = 2;
  volatile int j = 0;
  while (1) {
    j ++;
    if (j == 10000) {
      printf("Hello World from Navy-apps user '%s' for the %dth time!\n", argv[0], i ++);
      j = 0;
    }
  }
  return 0;
}
