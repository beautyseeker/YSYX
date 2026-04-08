#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

int main(int argc, char *argv[], char *envp[]);
extern char **environ;
void call_main(uintptr_t *args) {
  int argc = (int)args[0];
  char ** argv = (char **)(args + 1);
  environ = (char **)(args + 1 + argc + 1);
  printf("libos CRT0 call_main:\
    args_ptr:%p argc=%d argv=%p envp:%p\n",
            args, argc, argv[0], environ);
  int ret = main(argc, argv, environ);
  exit(ret);
  assert(0);
}
