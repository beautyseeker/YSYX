#include <am.h>
#include <klib-macros.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


void putch(char ch) {
    write(1, &ch, 1);
}

void halt(int code) {
    printf("Navy program halted with code %d\n", code);
    exit(code);
}
