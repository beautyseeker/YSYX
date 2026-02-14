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

#include <common.h>

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();
extern unsigned expr(char *args, bool *success);

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

#ifdef CONFIG_EXPR_TEST
  Log("====================    Expression test starts    =========================\n");
  FILE *fp = fopen("./tools/gen-expr/input", "r");
  FILE *log_fp = fopen("unmatch_log.txt", "w");
  Assert(fp != NULL && log_fp != NULL, "Can not open input file:./tools/gen-expr/input");
  char expr_str[256];
  unsigned result_gold;
  bool success;
  char line[1024];
  int passed = 0, failed = 0, total = 0;
  while (fgets(line, sizeof(line), fp)) {
    // 跳过空行
    if (line[0] == '\n' || line[0] == '\0') continue;
    // 解析第一列数字和第二列表达式（含空格）
    int matched = sscanf(line, "%u %[^\n]", &result_gold, expr_str);
    if (matched != 2) continue; // 跳过格式不对的行
    char *args = expr_str;
    unsigned result = expr(args, &success);
    if(result == result_gold) {
      // printf("Pass:expr: %s, result: %u\n", expr_str, result);
      passed++;
    }
    else {
      Log(ANSI_FG_RED "Fail:expr: %s, result: %u, expected: %u" ANSI_NONE, 
          expr_str, result, result_gold);
      fprintf(log_fp, "expr: %s, result: %u, expected: %u\n", 
        expr_str, result, result_gold);
      fflush(log_fp);
      failed++;
    }
    total++;
  }
  fclose(fp);
  fclose(log_fp);
  Log(ANSI_FG_BLUE "Total: %d, " ANSI_FG_GREEN "Passed: %d, " ANSI_FG_RED "Failed: %d" ANSI_NONE, 
    total, passed, failed);
  Log("====================    Expression test ends    =========================\n");
#endif

  /* Start engine. */
  engine_start();

  return is_exit_status_bad();
}
