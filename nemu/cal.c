#include <stdio.h>

typedef struct {
    char **top;
    char *data[];
} Stack;

void init(Stack* s) {
    s->top = s->data;
}

int empty(Stack* s) {
    return s->top == s->data;
}

char* pop(Stack* s) {
    if (empty(s)) {
        return NULL; // Stack is empty
    }
    s->top--;
    return *(s->top);
}

void push(Stack* s, char* str) {
    *(s->top) = str;
    s->top++;
}

int size(Stack* s) {
    return s->top - s->data;
}

int main() {

}

