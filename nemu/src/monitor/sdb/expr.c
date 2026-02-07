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

enum {
  TK_NOTYPE = 256, 
  TK_EQ,

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"\\-", '-'},        // minus
  {"\\*", '*'},        // multiply
  {"\\/", '/'},        // divide
  {"\\(", '('},        // left parenthesis
  {"\\)", ')'},        // right parenthesis
  // {"==", '-'},        // equal
  {"[0-9]+", 'd'},    // decimal number
  {"0x[0-9a-fA-F]+", 'h'}, // hexadecimal number

};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

static int get_priority(int type);
static int find_main_op(int p, int q);
static bool check_parentheses(int p, int q); 
static u_int32_t eval(int p, int q, bool *success);
// static void preprocess();


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

static Token tokens[256] __attribute__((used)) = {};
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

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

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
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

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

u_int32_t eval(int p, int q, bool *success) {
  if(*success == false) return 0;

  if (p > q) {
    *success = false;
    Assert(0, "Bad expression:\n"
    "p_type=%d, p_str=%s, op=%c\n"
    "q_type=%d, q_str=%s, op=%c\n",
    tokens[p].type, tokens[p].str, p, 
    tokens[q].type, tokens[q].str, q);

  }
  else if (p == q) {
    // Single token.
    if (tokens[p].type == 'd') {
      return (u_int32_t)atoi(tokens[p].str);
    }
    else if (tokens[p].type == 'h') {
      return (u_int32_t)strtol(tokens[p].str, NULL, 16);
    }
    else {
    *success = false;
    Assert(0, "Bad expression:\n"
    "p_type=%d, p_str=%s, op=%d\n"
    "q_type=%d, q_str=%s, op=%d\n",
    tokens[p].type, tokens[p].str, p, 
    tokens[q].type, tokens[q].str, q);
    return 0;
    }
  }
  else if (check_parentheses(p, q) == true) {
    // The expression is surrounded by a matched pair of parentheses.
    return eval(p + 1, q - 1, success);
  }

  else {
    int op = find_main_op(p, q);
    u_int32_t a = eval(p, op-1, success);
    if(*success == false) return 0;
    u_int32_t b = eval(op+1, q, success);
    if(*success == false) return 0;
    switch (tokens[op].type) {
      case '+': return a + b;
      case '-': return a - b;
      case '*': return a * b;
      case '/': 
        if(b == 0) {
          Assert(0, "Division by zero:\n"
          "a=%u, b=%u\n", a, b);
          *success = false;
          return 0;
        }
        return a / b;
      default: 
        *success = false;
        printf("Invalid operator: %c", tokens[op].type);
        return 0;
    }
  }
  return 0;
}

int find_main_op(int p, int q) {
  int min_prio = 1000;
  int main_op = p;
  int parentheses = 0;
  for (int i = p; i <= q; i++) {
    if(tokens[i].type == 'd' || tokens[i].type == 'h') continue;
    if (tokens[i].type == '(') parentheses++;
    else if (tokens[i].type == ')') parentheses--;
    else if (parentheses == 0) {
      int prio = get_priority(tokens[i].type);
      if (prio <= min_prio) {
        min_prio = prio;
        main_op = i;
      }
    }
  }
  Assert(parentheses == 0, "Unmatched parentheses");
  Assert(main_op<q && main_op>=p, 
    "main operator error, op_type=%d, op_str=%s, main_op=%c",
    tokens[main_op].type, tokens[main_op].str, main_op);
  return main_op;
}

int get_priority(int type) {
  switch (type) {
    case '+':
    case '-':
      return 1;
    case '*':
    case '/':
      return 2;
    default:return -1;
  }
  return -1; // 非运算符
}


word_t expr(char *e, bool *success) {
  *success = true;
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  word_t result = 0;
  result = eval(0, nr_token - 1, success);

  return result;
}
