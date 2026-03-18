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

#include <cpu/cpu.h>
#include "../isa/riscv32/local-include/reg.h"
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>
#include "../monitor/sdb/sdb.h"

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 10
#define RING_SIZE 32

CPU_state cpu = {};
uint64_t g_nr_guest_inst = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;
static char iring_buf[RING_SIZE][128];
static char symbol_buf[256];
static InstSnapshot g_current_inst;

void device_update();
void get_symbol_str(Decode *s);

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {

#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) { log_write("%s\n", _this->logbuf); }
#endif
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); }
  IFDEF(CONFIG_FTRACE, if(symbol_buf[0] != '\0') \
  { Trace("Function", ANSI_FG_YELLOW, "ftrace pc:0x%08x: %s", _this->pc, symbol_buf); });
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));
#ifdef CONFIG_WATCHPOINT
  wp_scan_wp();
#endif
}

static void exec_once(Decode *s, vaddr_t pc) {
  s->pc = pc;
  s->snpc = pc;
  isa_exec_once(s);
  cpu.pc = s->dnpc;
#ifdef CONFIG_ITRACE
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst;
#ifdef CONFIG_ISA_x86
  for (i = 0; i < ilen; i ++) {
#else
  for (i = ilen - 1; i >= 0; i --) {
#endif
    p += snprintf(p, 4, " %02x", inst[i]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

#ifdef CONFIG_FTRACE
  get_symbol_str(s);
#endif

  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst, ilen);

  g_current_inst.pc = s->pc;
  g_current_inst.inst = s->isa.inst;
  strncpy(g_current_inst.logbuf, s->logbuf, sizeof(g_current_inst.logbuf) - 1);
  g_current_inst.logbuf[sizeof(g_current_inst.logbuf) - 1] = '\0';
  memmove(iring_buf[g_nr_guest_inst % RING_SIZE], s->logbuf, sizeof(s->logbuf));
#endif
}

void get_symbol_str(Decode *s) {
  symbol_buf[0] = '\0';
  uint32_t opcode = s->isa.inst & 0x7f;
  
  if(opcode == 0x6f) { // jal
    const char* name = get_symbol_name(s->dnpc);
    if (name != NULL) {
      snprintf(symbol_buf, sizeof(symbol_buf), ANSI_FG_YELLOW"call <%s@0x%08x>"ANSI_NONE, name, s->dnpc);
    } else {
      snprintf(symbol_buf, sizeof(symbol_buf), ANSI_FG_YELLOW"call <Unknown>"ANSI_NONE);
    }
    symbol_buf[sizeof(symbol_buf) - 1] = '\0';
  } 
  else if(opcode == 0x67) { // jalr
    uint32_t rd = (s->isa.inst >> 7) & 0x1f;
    uint32_t rs1 = (s->isa.inst >> 15) & 0x1f;
    uint32_t imm = (s->isa.inst >> 20);
    if (rd == 1) { // jalr ra, ...
      const char* name = get_symbol_name(s->dnpc);
      if (name != NULL) {
        snprintf(symbol_buf, sizeof(symbol_buf), ANSI_FG_YELLOW"call <%s@0x%08x>"ANSI_NONE, name, s->dnpc);
      } else {
        snprintf(symbol_buf, sizeof(symbol_buf), ANSI_FG_YELLOW"call <Unknown>"ANSI_NONE);
      }
      symbol_buf[sizeof(symbol_buf) - 1] = '\0';
    }
    if (rd == 0 && rs1 == 1 && imm == 0) { // ret
      const char* name = get_symbol_name(s->dnpc);
      if (name != NULL) {
        snprintf(symbol_buf, sizeof(symbol_buf), ANSI_FG_YELLOW"ret <%s@0x%08x>"ANSI_NONE, name, s->dnpc);
      } else {
        snprintf(symbol_buf, sizeof(symbol_buf), ANSI_FG_YELLOW"ret <Unknown>"ANSI_NONE);
      }
      symbol_buf[sizeof(symbol_buf) - 1] = '\0';
    }
  }
  else {
    symbol_buf[0] = '\0';
  }
}

InstSnapshot* get_current_inst_snapshot() {
  return &g_current_inst;
}

static void execute(uint64_t n) {
  Decode s;
  for (;n > 0; n --) {
    exec_once(&s, cpu.pc);
    g_nr_guest_inst ++;
    trace_and_difftest(&s, cpu.pc);
    if (nemu_state.state != NEMU_RUNNING) break;
    IFDEF(CONFIG_DEVICE, device_update());
  }
}

void print_iring() {
  int i;
  int start = g_nr_guest_inst % RING_SIZE;
  printf(ANSI_FMT("--------------Instruction Ring Buffer (last %d instructions)-------------:\n", 
  ANSI_FG_CYAN), RING_SIZE);
  for (i = 0; i < RING_SIZE; i++) {
    int idx = (start + i) % RING_SIZE;
    Trace("Iring", ANSI_FG_YELLOW, "%s", iring_buf[idx]);
  }
  printf(ANSI_FMT("--------------End of Instruction Ring Buffer-------------\n", ANSI_FG_CYAN));
}

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
  if (g_timer > 0) Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency");
}

void assert_fail_msg() {
  isa_reg_display();
  statistic();
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  g_print_step = (n < MAX_INST_TO_PRINT);
  switch (nemu_state.state) {
    case NEMU_END: case NEMU_ABORT: case NEMU_QUIT:
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
      return;
    default: nemu_state.state = NEMU_RUNNING;
  }

  uint64_t timer_start = get_time();

  execute(n);

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;

    case NEMU_END: case NEMU_ABORT:
      Log("nemu: %s at pc = " FMT_WORD,
          (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) :
           (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) :
            ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
          nemu_state.halt_pc);
      IFDEF(CONFIG_ITRACE, if (nemu_state.halt_ret != 0) { print_iring(); });
      // fall through
    case NEMU_QUIT: statistic();
  }
}
