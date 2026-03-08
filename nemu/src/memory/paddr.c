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

#include <memory/host.h>
#include <memory/paddr.h>
#include <device/mmio.h>
#include <isa.h>

#if   defined(CONFIG_PMEM_MALLOC)
static uint8_t *pmem = NULL;
#else // CONFIG_PMEM_GARRAY
static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};
#endif
#define CONFIG_MTRACE_COND(addr) (1)
#define CONFIG_DTRACE_COND(addr) (1)

uint8_t* guest_to_host(paddr_t paddr) { return pmem + paddr - CONFIG_MBASE; }
paddr_t host_to_guest(uint8_t *haddr) { return haddr - pmem + CONFIG_MBASE; }

static word_t pmem_read(paddr_t addr, int len) {
  word_t ret = host_read(guest_to_host(addr), len);
  return ret;
}

static void pmem_write(paddr_t addr, int len, word_t data) {
  host_write(guest_to_host(addr), len, data);
}

static void out_of_bound(paddr_t addr) {
  panic("address = " FMT_PADDR " is out of bound of pmem [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
      addr, PMEM_LEFT, PMEM_RIGHT, cpu.pc);
}

void init_mem() {
#if   defined(CONFIG_PMEM_MALLOC)
  pmem = malloc(CONFIG_MSIZE);
  assert(pmem);
#endif
  IFDEF(CONFIG_MEM_RANDOM, memset(pmem, rand(), CONFIG_MSIZE));
  Log("physical memory area [" FMT_PADDR ", " FMT_PADDR "]", PMEM_LEFT, PMEM_RIGHT);
  Log("Memory Trace: %s", MUXDEF(CONFIG_MTRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
}

word_t paddr_read(paddr_t addr, int len) {
  if (likely(in_pmem(addr))) {
    word_t ret = pmem_read(addr, len);
    IFDEF(CONFIG_MTRACE, if (CONFIG_MTRACE_COND(addr))
    { Trace("Memory", ANSI_FG_CYAN, "mtrace_read addr: ["FMT_PADDR"] => "FMT_WORD, addr, ret); });
    return ret;
  }
  IFDEF(CONFIG_DEVICE, {
    word_t ret = mmio_read(addr, len);
    IFDEF(CONFIG_DTRACE, if (CONFIG_DTRACE_COND(addr)) {
      IOMap* map = fetch_mmio_map(addr);
      Trace("Device", ANSI_FG_MAGENTA, "device:%s mmio_read addr: ["FMT_PADDR"] => "FMT_WORD, map->name, addr, ret);
    });
    return ret;
  });
  out_of_bound(addr);
  return 0;
}

void paddr_write(paddr_t addr, int len, word_t data) {
  if (likely(in_pmem(addr))) { 
    IFDEF(CONFIG_MTRACE, if (CONFIG_MTRACE_COND(addr))
    { Trace("Memory", ANSI_FG_CYAN, "mtrace_write addr: ["FMT_PADDR"] <= "FMT_WORD, addr, data); });
    pmem_write(addr, len, data);
    return; }
  IFDEF(CONFIG_DEVICE, {
    IFDEF(CONFIG_DTRACE, if (CONFIG_DTRACE_COND(addr)) {
      IOMap* map = fetch_mmio_map(addr);
      Trace("Device", ANSI_FG_MAGENTA, "device:%s mmio_write addr: ["FMT_PADDR"] <= "FMT_WORD, map->name, addr, data);
    });
    mmio_write(addr, len, data);
    return;
  });
  out_of_bound(addr);
}
