#include <nterm.h>
#include <stdarg.h>
#include <unistd.h>
#include <SDL.h>

char handle_key(SDL_Event *ev);

static void sh_printf(const char *format, ...) {
  static char buf[256] = {};
  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buf, 256, format, ap);
  va_end(ap);
  term->write(buf, len);
}

static void sh_banner() {
  sh_printf("Built-in Shell in NTerm (NJU Terminal)\n\n");
}

static void sh_prompt() {
  sh_printf("sh> ");
}

static void sh_handle_cmd(const char *cmd) {
  if(cmd == NULL || cmd[0] == '\0') {
    sh_printf("\n"); 
    return;
  }
  char *full_cmd = strdup(cmd);
  char *opt = strtok(full_cmd, " \t\r\n");
  if(opt == NULL) {
    free(full_cmd);
    return;
  }
  char *args = strtok(NULL, "\t\r\n");
  if(strcmp(opt, "echo") == 0) {
    sh_printf("%s\n", args ? args : "");
  } else if (strcmp(opt, "clear") == 0) {
    term->clear();
  } else if (strcmp(opt, "exit") == 0) {
    exit(0);
  } else if(strcmp(opt, "exec") == 0) {
    if(args == NULL) {
      sh_printf("Usage: exec CMD\n");
    } else {
      setenv("PATH", "/bin", 1);
      char *argv[] = {NULL};
      execvp(args, argv);
      // execv(args, NULL);
      sh_printf("Failed to exec %s\n", args);
    }
  } else {
    sh_printf("Unknown command: %s\n", opt);
  }
  free(full_cmd);
  // char opt[16];
  // char arg[64];
  // if (sscanf(cmd, "%s %s", opt, arg) == 2) {
  //   if (strcmp(opt, "echo") == 0) {
  //     sh_printf("%s\n", arg);
  //   } else if (strcmp(opt, "clear") == 0) {
  //     term->clear();
  //   } else if (strcmp(opt, "exit") == 0) {
  //     exit(0);
  //   } else {
  //     sh_printf("Unknown command: %s", cmd);
  //   }
  // }
  // else
  //   sh_printf("Invalid command: %s", cmd);
}

void builtin_sh_run() {
  sh_banner();
  sh_prompt();

  while (1) {
    SDL_Event ev;
    if (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_KEYUP || ev.type == SDL_KEYDOWN) {
        const char *res = term->keypress(handle_key(&ev));
        if (res) {
          sh_handle_cmd(res);
          sh_prompt();
        }
      }
    }
    refresh_terminal();
  }
}
