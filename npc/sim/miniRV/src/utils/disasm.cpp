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

#include <dlfcn.h>
#include <capstone/capstone.h>
#include <common.h>
#include "Simlator.hpp"
#include <stdlib.h>

#if defined(__APPLE__)
#define CS_LIB_SUFFIX "5.dylib"
#elif defined(__linux__)
#define CS_LIB_SUFFIX "so.5"
#else
#error "Unsupported platform"
#endif

static size_t (*cs_disasm_dl)(csh handle, const uint8_t *code,
    size_t code_size, uint64_t address, size_t count, cs_insn **insn);
static void (*cs_free_dl)(cs_insn *insn, size_t count);

static csh handle;

void InstTracer::init_disasm() {
  char *nemu_home = getenv("NEMU_HOME");
  if (nemu_home == NULL) {
    printf("Error: Environment variable NEMU_HOME is not set!\n");
    assert(0);
  }
  std::string lib_path = std::string(nemu_home) + "/tools/capstone/repo/libcapstone." CS_LIB_SUFFIX;
  printf("Attempting to load libcapstone from: %s\n", lib_path.c_str());
  void *dl_handle = dlopen(lib_path.c_str(), RTLD_LAZY);
  assert(dl_handle);


  cs_err (*cs_open_dl)(cs_arch arch, cs_mode mode, csh *handle) = NULL;
  cs_open_dl = reinterpret_cast<cs_err (*)(cs_arch, cs_mode, csh *)>(dlsym(dl_handle, "cs_open"));
  assert(cs_open_dl);

  cs_disasm_dl = reinterpret_cast<size_t (*)(csh, const uint8_t *, size_t, uint64_t, size_t, cs_insn **)>(dlsym(dl_handle, "cs_disasm"));
  assert(cs_disasm_dl);

  cs_free_dl = reinterpret_cast<void (*)(cs_insn *, size_t)>(dlsym(dl_handle, "cs_free"));
  assert(cs_free_dl);

  cs_arch arch = MUXDEF(CONFIG_ISA_x86,      CS_ARCH_X86,
                   MUXDEF(CONFIG_ISA_mips32, CS_ARCH_MIPS,
                   MUXDEF(CONFIG_ISA_riscv,  CS_ARCH_RISCV,
                   MUXDEF(CONFIG_ISA_loongarch32r,  CS_ARCH_LOONGARCH, -1))));
  cs_mode mode = CS_MODE_RISCV32;
	int ret = cs_open_dl(arch, mode, &handle);
  assert(ret == CS_ERR_OK);

#ifdef CONFIG_ISA_x86
  cs_err (*cs_option_dl)(csh handle, cs_opt_type type, size_t value) = NULL;
  cs_option_dl = reinterpret_cast<cs_err (*)(csh, cs_opt_type, size_t)>(dlsym(dl_handle, "cs_option"));
  assert(cs_option_dl);

  ret = cs_option_dl(handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_ATT);
  assert(ret == CS_ERR_OK);
#endif
}

void InstTracer::disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte) {
	cs_insn *insn;
	size_t count = cs_disasm_dl(handle, code, nbyte, pc, 0, &insn);
  assert(count == 1);
  int ret = snprintf(str, size, "%s", insn->mnemonic);
  if (insn->op_str[0] != '\0') {
    snprintf(str + ret, size - ret, "\t%s", insn->op_str);
  }
  cs_free_dl(insn, count);
}
