#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>
typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char expr[256];
  unsigned last_value;

  /* TODO: Add more members if necessary */

} WP;

word_t expr(char *e, bool *success);

WP* new_wp();
void free_wp(WP* wp);
int free_wp_by_no(int NO);
void wp_list_show();
int wp_scan_wp();
void init_wp_pool();
void wp_display(WP* wp);
void load_random_expr_test();
void init_elf(const char *filename);
const char* get_symbol_name(paddr_t addr);

#endif
