/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "sdb.h"
#include <cpu/cpu.h>

#define NR_WP 32

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

WP* new_wp() {
  if(free_ != NULL) {
    WP* wp = free_;
    free_ = free_->next;
    wp->next = head;

    head = wp;
    return wp;
  }
  Assert(0, "No free watchpoint available!");
  return NULL;
}

void free_wp(WP* wp) {
  Assert(wp != NULL, "Trying to free a NULL watchpoint!");

  // 从head链表中移除wp
  if(head == wp) {
    head = head->next;
  } else {
    WP* prev = head;
    while(prev != NULL && prev->next != wp) {
      prev = prev->next;
    }
    if(prev != NULL) {
      prev->next = wp->next;
    }
  }
  // 将wp加入free链表
  wp->next = free_;
  free_ = wp;
}

int free_wp_by_no(int NO) {
  WP* curr = head;
  while(curr != NULL) {
    if(curr->NO == NO) {
      free_wp(curr);
      return 0;
    }
    curr = curr->next;
  }
  printf(ANSI_FG_RED "Watchpoint %d not found.\n" ANSI_NONE, NO);
  return -1;
}

int wp_scan_wp() {
  int triggered = 0;
  WP* curr = head;
  while(curr != NULL) {
    bool success;
    unsigned cur_value = expr(curr->expr, &success);
    if(!success) {
      printf("Failed to evaluate watchpoint %d expression: %s\n", curr->NO, curr->expr);
      curr = curr->next;
      continue;
    }
    if(cur_value != curr->last_value) {
      curr->last_value = cur_value;
      triggered = 1;
      nemu_state.state = NEMU_STOP;
      printf(ANSI_FG_BLUE "Watchpoint %d triggered: %s inst_asm: %s\n" ANSI_NONE,
         curr->NO, curr->expr, get_current_inst_snapshot()->logbuf);
    }
    curr = curr->next;
  }
  return triggered;
}

void wp_list_show() {
  WP* curr = head;
  if(curr == NULL) {
    printf(ANSI_FG_BLUE "No watchpoints set.\n" ANSI_NONE);
    return;
  }

  printf("%-4s  %-20s  %-10s\n", "Num", "Expression", "Last Value"); // 表头也用固定宽度
  printf("------------------------------------------\n");
  while(curr != NULL) {
      printf("%-4d  %-20.20s  %-10u\n", 
        curr->NO, curr->expr, curr->last_value);
      curr = curr->next;
  }
  printf("------------------------------------------\n");
}

void wp_display(WP* wp) {
  if(wp == NULL) {
    printf(ANSI_FG_RED "Watchpoint is NULL.\n" ANSI_NONE);
    return;
  }
  printf(ANSI_FG_BLUE "Set watchpoint %d: %s\n" ANSI_NONE, wp->NO, wp->expr);
}
/* TODO: Implement the functionality of watchpoint */

