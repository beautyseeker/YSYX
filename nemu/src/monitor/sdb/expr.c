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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
<<<<<<< HEAD

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */

=======
#include <stdbool.h>

enum {
  TK_NOTYPE = 256, 
  TK_EQ = 255,
  TK_NE = 254,
  TK_AND = 253,
  TK_NEG = 252,
  TK_REG = 251,
  TK_DEREF = 250,
  TK_LESS = 249,
  TK_LESSEQ = 248,
  TK_GREATER = 247,
  TK_GREATEREQ = 246,

};

enum {
  unary = 1,
  binary = 2,
  ternary = 3,
>>>>>>> pa_repo/master
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

<<<<<<< HEAD
  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
=======
  {"0x[0-9a-fA-F]+", 'h'}, // hexadecimal number
  {"\\$[a-zA-Z0-9]+", TK_REG}, // register

  {"==", TK_EQ},        // equal
  {"!=", TK_NE},        // not equal
  {"<", TK_LESS},        // less than
  {"<=", TK_LESSEQ},    // less than or equal
  {">", TK_GREATER},      // greater than
  {">=", TK_GREATEREQ}, // greater than or equal
  {"&&", TK_AND},        // and

  {"\\+", '+'},         // plus
  {"\\-", '-'},        // minus
  {"\\*", '*'},        // multiply
  {"\\/", '/'},        // divide
  {"\\(", '('},        // left parenthesis
  {"\\)", ')'},        // right parenthesis

  {" +", TK_NOTYPE},    // spaces
  {"[0-9]+", 'd'},    // decimal number


>>>>>>> pa_repo/master
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};
<<<<<<< HEAD
=======
static char expr_buf[65536] = {};

static int get_priority(int type);
static int find_main_op(int p, int q);
static bool check_parentheses(int p, int q); 
static int32_t eval(int p, int q, bool *success);
extern word_t isa_reg_str2val(const char *s, bool *success);
extern word_t paddr_read(paddr_t addr, int len);

>>>>>>> pa_repo/master

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

<<<<<<< HEAD
static Token tokens[32] __attribute__((used)) = {};
=======
static Token tokens[256] __attribute__((used)) = {};
>>>>>>> pa_repo/master
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

<<<<<<< HEAD
        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          default: TODO();
=======
        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          default: 
            tokens[nr_token].type = rules[i].token_type;
            Assert(substr_len < 32, "Too long token beyond char size 32");
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token ++;
            break;
          ;
>>>>>>> pa_repo/master
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
<<<<<<< HEAD
=======
  // 新增逻辑：区分单目运算符（负号）和双目运算符（减号）
  for (int i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '-') {
      // 一元运算符前面不能是操作数或右括号
      if(i == 0 || (tokens[i-1].type != 'd' 
        && tokens[i-1].type != 'h' && tokens[i-1].type != ')')) {
        tokens[i].type = TK_NEG;
      }
    }
    else if (tokens[i].type == '*') {
      if(i == 0 || (tokens[i-1].type != 'd' 
        && tokens[i-1].type != 'h' && tokens[i-1].type != ')')) {
        tokens[i].type = TK_DEREF;
      }
    }
  }
>>>>>>> pa_repo/master

  return true;
}

<<<<<<< HEAD

word_t expr(char *e, bool *success) {
=======
bool check_parentheses(int p, int q) {
  if (tokens[p].type != '(' || tokens[q].type != ')') return false;
  int cnt = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') cnt++;
    else if (tokens[i].type == ')') cnt--;
    if (cnt == 0 && i < q) return false; // 括号提前闭合
  }
  return cnt == 0;
}

int32_t eval(int p, int q, bool *success) {
  if(*success == false) return 0;

  if (p > q) {
    *success = false;
    Assert(0, "Bad expression:%s\n", expr_buf);

  }
  else if (p == q) {
    // 单token递归基
    if (tokens[p].type == 'd') {
      return (int32_t)atoi(tokens[p].str);
    }
    else if (tokens[p].type == 'h') {
      return (int32_t)strtol(tokens[p].str, NULL, 16);
    }
    else if (tokens[p].type == TK_REG) {
      int32_t reg_val = isa_reg_str2val(tokens[p].str + 1, success);
      return *success ? reg_val : 0;
    }
    else {
    *success = false;
    Assert(0, "Bad expression:%s\n", expr_buf);
    return 0;
    }
  }
  else if (check_parentheses(p, q) == true) {
    // 外层去括号处理
    return eval(p + 1, q - 1, success);
  }

  else {
    // 表达式二元运算符处理
    int op = find_main_op(p, q);
    if(op == -1) {  
      // 二元运算符没找到才去处理一元运算符
      switch (tokens[p].type) {
        case TK_NEG:
          return *success ? - eval(p + 1, q, success): 0;
        case TK_DEREF: {
          word_t addr = *success ? eval(p + 1, q, success) : 0;
          return *success ? paddr_read(addr, 4) : 0;
        }
        default: break;
      }
    }
    else{
      int32_t left = eval(p, op-1, success);
      if(*success == false) return 0;
      int32_t right = eval(op+1, q, success);
      if(*success == false) return 0;
      switch (tokens[op].type) {
        case '+': return left + right;
        case '-': return left - right;
        case '*': return left * right;
        case '/': 
          if(right == 0) {
            // Assert(0, "Division by zero:%s\n", expr_buf);
            *success = false;
            return 0;
          }
          return left / right;
        case TK_EQ: return left == right;
        case TK_NE: return left != right;
        case TK_AND: return left && right;
        case TK_LESS: return left < right;
        case TK_LESSEQ: return left <= right;
        case TK_GREATER: return left > right;
        case TK_GREATEREQ: return left >= right;
        default: 
          *success = false;
          printf("Invalid operator: %c", tokens[op].type);
          return 0;
      }
    }
  }
  return 0;
}

int find_main_op(int p, int q) {
  int priorest = -1; // 修改：初始优先级设为无效
  int main_op = -1;  // 修改：默认没找到主运算符
  int parentheses = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') parentheses++;
    else if (tokens[i].type == ')') parentheses--;
    else if (parentheses == 0) {
      if(tokens[i].type != '+' && tokens[i].type != '-' 
        && tokens[i].type != '*' && tokens[i].type != '/' 
        && tokens[i].type != TK_EQ && tokens[i].type != TK_NE 
        && tokens[i].type != TK_AND && tokens[i].type != TK_LESS
        && tokens[i].type != TK_LESSEQ && tokens[i].type != TK_GREATER
        && tokens[i].type != TK_GREATEREQ) continue;
      int prio = get_priority(tokens[i].type);
      if (prio >= priorest) {  // 主运算符为最低优先级里最靠右的
        priorest = prio;
        main_op = i;
      }
    }
  }
  Assert(parentheses == 0, "Unmatched parentheses");
  return main_op;
}

int get_priority(int type) {
  switch (type) {
    case '*':
    case '/':
      return 3;
    case '+':
    case '-':
      return 4;
    case TK_EQ:
    case TK_NE:
      return 7;
    case TK_AND:
      return 11;
    case TK_LESS:
    case TK_LESSEQ:
    case TK_GREATER:
    case TK_GREATEREQ:
      return 8;
    default:return -1;
  }
  return -1; // 非运算符
}

word_t expr(char *e, bool *success) {
  *success = true;
  strncpy(expr_buf, e, sizeof(expr_buf) - 1);
  expr_buf[sizeof(expr_buf) - 1] = '\0';
>>>>>>> pa_repo/master
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

<<<<<<< HEAD
  /* TODO: Insert codes to evaluate the expression. */
  TODO();

  return 0;
=======
  word_t result = eval(0, nr_token - 1, success);

  return result;
}

void load_random_expr_test() {
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
>>>>>>> pa_repo/master
}
