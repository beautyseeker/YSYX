#pragma once
#include "common.h"
#include <string>
#include <cstring>
#include <cstdlib>
#include "Vtop_TopMiniRV.h"

#define DEFAULT_SIM_CYCLES SIM_CYCLES
#define RING_BUFFER_SIZE 5
#define SIM_INFO 1
#define ALWAYS_RUN 0

#define SIMLOG(format, ...) \
    IFONE(SIM_INFO, printf(ANSI_FG_BLUE format ANSI_NONE"\n",##__VA_ARGS__));


#define SIMERROR(format, ...) \
    IFONE(SIM_INFO, printf(ANSI_FG_RED format ANSI_NONE "\n",##__VA_ARGS__));

#define IOLOG(format, ...) \
    IFDEF(CONFIG_DTRACE, printf(ANSI_FG_BLUE "[%s:%d %s] " ANSI_NONE format "\n",\
         __FILE__, __LINE__, __func__, ##__VA_ARGS__));

#define MEMLOG(format, ...) \
    IFDEF(CONFIG_MTRACE, printf(ANSI_FG_CYAN "[%s:%d %s] " ANSI_NONE format "\n",\
         __FILE__, __LINE__, __func__, ##__VA_ARGS__));

#define INSTLOG(format, ...) \
    IFDEF(CONFIG_ITRACE, printf(ANSI_FG_GREEN "[%s:%d %s] " ANSI_NONE format "\n",\
         __FILE__, __LINE__, __func__, ##__VA_ARGS__));


extern "C" void handle_sys_brk();
extern "C" void handle_mem_access_error(uint32_t pc, uint32_t mapped_addr);
extern "C" const char* get_img_path();
extern "C" void mmio_write(uint32_t io_addr, int data, uint8_t wmask);
extern "C" uint64_t mmio_read(uint32_t io_addr);
