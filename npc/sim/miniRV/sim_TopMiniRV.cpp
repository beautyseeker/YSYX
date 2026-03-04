#include "Vtop_TopMiniRV.h"
#include <macro.h>
#include <utils.h>
#include <capstone/capstone.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <./generated/autoconf.h>

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
extern "C" void handle_mem_access_error(uint32_t addr, uint32_t mapped_addr);
extern "C" const char* get_img_path();
extern "C" void mmio_write(uint32_t addr, int data, uint8_t wmask);
extern "C" uint64_t mmio_read(uint32_t addr);

const char *regs_name[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

enum MMIO_ADDR {
    SERIAL_ADDR = 0x10000000,
    RTC_ADDR = 0x10000048
};

enum SimState{
    RUNNING,
    STOP,
    ABORT,
    QUIT,
    END
};

const char* sim_state_names[] = {
    "RUNNING",
    "STOP",
    "ABORT",
    "QUIT",
    "END"
};

enum ErrorCause{
    OUT_OF_CYCLES = 0,
    MEM_ACCESS_ERROR,
    NORMAL_EXIT
};
const char* error_cause_names[] = {
    "OUT_OF_CYCLES",
    "MEM_ACCESS_ERROR",
    "NORMAL_EXIT"
};

struct CPUPrintInfo {
    uint32_t PC_current;
    uint32_t instruction;
    std::string asm_str; // 反汇编字符串

    std::string to_string() const {
        char buf[128];
        snprintf(buf, sizeof(buf), "PC=0x%08x INST=0x%08x ASM=%s", 
        PC_current, instruction, asm_str.c_str());
        return std::string(buf);
    }
};

class Simlator {
private:
    // 被管理的硬件实体
    Vtop_TopMiniRV* top;
    
    // 辅助调试汇编码
    csh cap_handle;
    cs_insn *insn;
    std::string asm_str;
    // 寄存器镜像
    uint32_t regs_snapshot[32];
    
    // 内部状态
    uint64_t cycle_max;
    uint64_t cycle_nr;
    uint64_t inst_nr;
    uint64_t load_inst_num;

    ErrorCause error_cause;
    SimState sim_state;

    uint64_t boot_time;

    std::string img_path;
    std::string vcd_path;
    std::string img_name;

    CPUPrintInfo ring_buffer[RING_BUFFER_SIZE]; // 环形缓冲区，保存最近5条指令的状态

    // 代理原有的硬件访问，保持接口简洁
    uint32_t get_pc() const { return top->PC_current; }
    uint32_t get_inst() const { return top->instruction; }
    void parse_args(int argc, char **argv) {
        for (int i = 1; i < argc; ++i) {
            if (strncmp(argv[i], "IMG=", 4) == 0) {
                img_path = argv[i] + 4;
            }
            else if (strncmp(argv[i], "CYCLES=", 7) == 0) {
                cycle_max = atoi(argv[i] + 7);
            }
            else if (strncmp(argv[i], "VCD=", 4) == 0) {
                vcd_path = argv[i] + 4;
            }
        }
    }

    const char* get_filename() const {
        const char* filename = strrchr(img_path.c_str(), '/');
        if (filename) {
            return filename + 1; // 返回文件名部分
        } else {
            return img_path.c_str(); // 如果没有路径分隔符，直接返回输入字符串
        }
    }

    void set_asm() {
        for(int i=0; i<RING_BUFFER_SIZE; i++) {
            CPUPrintInfo cur_inst = ring_buffer[i];
            size_t count = cs_disasm(cap_handle, (uint8_t*)&(cur_inst.instruction), 
            4, cur_inst.PC_current, 1, &insn);
            if (count > 0) {
                asm_str = std::string(insn[0].mnemonic) + " " + insn[0].op_str;
                cs_free(insn, count);
            } else {
                asm_str = "<invalid>";
            }
        }
    }

    std::string get_asm() const {
        return asm_str;
    }

    std::string log_str() const {
        char buf[128];
        snprintf(buf, sizeof(buf), "PC=0x%08x INST=0x%08x ASM=%s", 
        top->PC_current, top->instruction, asm_str.c_str());
        return std::string(buf);
    }

    uint64_t get_time_internal() const {
        struct timeval now;
        gettimeofday(&now, NULL);
        uint64_t us = now.tv_sec * 1000000 + now.tv_usec;
        return us;
    }

    uint64_t get_uptime() const {
        if (boot_time == 0) return 0;
        uint64_t now = get_time_internal();
        return now - boot_time;
    }

public:
    static Simlator* instance; // 单例实例 
    // 构造时传入已创建好的 top
    Simlator(Vtop_TopMiniRV* model, int argc, char **argv) : 
    top(model), error_cause(NORMAL_EXIT), sim_state(RUNNING) {
        parse_args(argc, argv);
        img_path = get_img_path();
        img_name = get_filename();

        cs_open(CS_ARCH_RISCV, CS_MODE_RISCV32, &cap_handle);
        instance = this;
        reset();
    }

    const char* get_img_path() {
        //获取img_path文件读入的指令数量，并打印出来
        if (!img_path.empty()) {
            FILE *fp = fopen(img_path.c_str(), "r");
            if (fp) {
                load_inst_num = 0;
                char ch;
                while ((ch = fgetc(fp)) != EOF) {
                    if (ch == '\n') load_inst_num++;
                }
                fclose(fp);
                SIMLOG("instructions: %ld load from file: %s", 
                load_inst_num, img_path.c_str());
            } else {
                SIMERROR("Failed to open image file: %s", img_path.c_str());
            }
        }
        return img_path.empty() ? "" : img_path.c_str();
    }

    // 封装时钟步进逻辑
    void clock_step(uint64_t n) {
        for (uint64_t i = 0; i < n && sim_state == RUNNING; ++i) {
            inst_nr += (top->rst_n == 0) ? 0 : 1; // 如果处于复位状态，不增加指令计数

            ring_buffer[inst_nr % RING_BUFFER_SIZE] = 
            {.PC_current = top->PC_current, .instruction = top->instruction, \
            .asm_str = "<disassembly not implemented>"};

            top->clk = 1; top->eval();
            // 这里可以做一些时钟上升沿的同步逻辑
            top->clk = 0; top->eval();
            cycle_nr++;
        }
        // 这里可以添加一些周期级的监控逻辑，比如检查特定寄存器的值，或者监控特定的指令执行等
    }

    uint32_t get_gpr(int idx) const { 
        if (idx >= 0 && idx < 32) {
            return top->gpr[idx];
        } else {
            SIMERROR("Invalid register index: %d\n", idx);
            return -1; // 返回一个错误值
        }
    }

    uint32_t get_gpr(const char* name) const {
        for (int i = 0; i < 32; i++) {
            if (strcmp(name, regs_name[i]) == 0) {
                return top->gpr[i];
            }
        }
        SIMERROR("Invalid register name: %s\n", name);
        return -1; // 返回一个错误值
    }

    SimState get_sim_state() const {
        return sim_state;
    }

    void set_sim_state(SimState new_state) {
        sim_state = new_state;
    }

    uint32_t get_paddr_read(uint32_t addr, int len) {
        // Assert(top != nullptr, "Top module is not initialized");
        // Assert(top->rootp != nullptr && top->rootp->top_TopMiniRV != nullptr 
        //     && top->rootp->top_TopMiniRV->lsu != nullptr 
        //     && top->rootp->top_TopMiniRV->lsu->MEM != nullptr 
        //     && top->rootp->top_TopMiniRV->lsu->PMEM_BASE != nullptr 
        //     && top->rootp->top_TopMiniRV->lsu->PMEM_SIZE != nullptr, 
        //     "Missing LSU or its memory components in the top module");
        // Assert(len == 1 || len == 2 || len == 4, "misaligned memory access with length: %d\n", len);
        // auto PMEM_BASE = top->rootp->top_TopMiniRV->lsu->CONFIG_BASE;
        // auto PMEM_SIZE = top->rootp->top_TopMiniRV->lsu->PMEM_SIZE;
        // if (addr < PMEM_BASE || addr >= PMEM_BASE + PMEM_SIZE) {
        //     print_mem_access_error(addr, addr - PMEM_BASE);
        //     SIMERROR("Address 0x%08x is out of bounds [0x%08x - 0x%08x]\n",
        //     addr, PMEM_BASE, PMEM_BASE + PMEM_SIZE);
        //     return -1;
        // }
        // auto MEM = top->rootp->top_TopMiniRV->lsu->MEM;
        // switch(len) {
        //     case 1: return MEM[addr-PMEM_BASE] & 0xFF;
        //     case 2: return MEM[addr-PMEM_BASE] & 0xFFFF;
        //     case 4: return MEM[addr-PMEM_BASE] & 0xFFFFFFFF;
        //     default:
        //         SIMERROR("Misaligned memory access at address:\
        //         0x%08x with length: %d\n", addr, len);
        //         return -1;
        // }
        // return 0xdeadbeef;
    }

    void run() {
        if(sim_state == RUNNING) {
        #if ALWAYS_RUN
            while (true) {
                clock_step(1);
            }
        #else
            clock_step(-1);
        #endif
        print_sim_reach_max();
        }

    }

    void reset() {
        top->rst_n = 0;
        clock_step(5);
        top->rst_n = 1;
        cycle_nr = 0;
        inst_nr = 0;
        boot_time = get_time_internal();
        sim_state = RUNNING;
        printf("-------------Starting simulation of %s...---------------\n", img_name.c_str());
    }

    void print_ring_buffer() const {
        printf("-------------------------Recent %d instructions-----------------\n", RING_BUFFER_SIZE);
        // 如果总指令数还没填满缓冲区，从 0 开始；否则从当前写入位置开始
        int start = (inst_nr < RING_BUFFER_SIZE) ? 0 : (inst_nr % RING_BUFFER_SIZE);
        int count = (inst_nr < RING_BUFFER_SIZE) ? inst_nr : RING_BUFFER_SIZE;
        for (int i = 1; i < count+1; i++) {
            int idx = (start + i) % RING_BUFFER_SIZE;
            IFDEF(SIM_DEBUG, printf(ANSI_FMT("[%ld] %s\n", ANSI_FG_BLUE),\
             inst_nr - count + i, ring_buffer[idx].to_string().c_str()));
        }
    }

    void print_trap_state(int state) {
        IFDEF(CONFIG_ITRACE, do {set_asm(); print_ring_buffer();} while(0));
        if(state == 0) {
            printf(ANSI_FMT("HIT A GOOD TRAP in %s!\n", ANSI_FG_GREEN), img_name.c_str());
            sim_state = END;
        } else {
            printf(ANSI_FMT("[%ld] %s \nHIT A BAD TRAP in %s due to %s!\n", ANSI_FG_RED), 
            inst_nr, log_str().c_str(), img_name.c_str(), error_cause_names[error_cause]);
            sim_state = ABORT;
        }
        print_statistics();
    }

    void print_mem_access_error(uint32_t addr, uint32_t mapped_addr) {
        SIMERROR("Memory access error at address: 0x%08x (mapped to 0x%08x)\n", addr, mapped_addr);
        error_cause = MEM_ACCESS_ERROR;
        print_trap_state(-1);
    }

    void print_sim_reach_max() {
        SIMERROR("Simulation reached max cycles (%lu) in %s!\n", cycle_max, img_name.c_str());
        error_cause = OUT_OF_CYCLES;
        print_trap_state(-1);
    }

    void print_regs() const {
        printf("\n------------------------- Register Dump -------------------------\n");
        for (int i = 0; i < 32; i++) {
            // %-4s  : 名称左对齐，占4位
            // 0x%08x: 16进制补0对齐，占8位
            // |     : 分隔符增加视觉可读性
            printf("%-4s: 0x%08x  ", regs_name[i], top->gpr[i]);
            // 每 8 个寄存器换一行（8列打印通常比4列更适合终端宽度）
            if ((i + 1) % 8 == 0) {
                printf("\n");
            }
        }
        printf("-----------------------------------------------------------------\n");
    }

    void print_statistics() const {
        print_regs();
        SIMLOG("---------------------Simulation end statistics-------------------------");
        if (cycle_nr > 0 && inst_nr > 0) {
            uint64_t sim_time_us = get_uptime();
            SIMLOG("%s Simulation time: %.3f ms MIPS: %.6f", 
            img_name.c_str(), sim_time_us / 1000.0, float(inst_nr) / sim_time_us);
            SIMLOG("Cycles executed:%lu Instructions executed:%lu CPI: %.2f", 
                cycle_nr, inst_nr, (double)cycle_nr / inst_nr);
        }
        SIMLOG("-----------------------------------------------------------------------");
        exit(0);
    }


    uint64_t print_mmio_read(uint32_t addr) const {
        static uint64_t latched_time = 0;
        if (addr == RTC_ADDR) {
            latched_time = get_uptime(); // 读取启动时间
            IOLOG("CPP MMIO Read from RTC address: 0x%08x, data: 0x%lx\n",
             addr, latched_time);
            return latched_time; 
        } else if (addr == RTC_ADDR + 4) {
            return latched_time; // 读取高位时返回上次采样的值，保证原子性
        }
        return 0;
    }

    void print_mmio_write(uint32_t addr, uint32_t data, uint8_t wmask = 0) const {
        if (addr == SERIAL_ADDR) {
            IOLOG("CPP MMIO Write to SERIAL address: 0x%08x, data: 0x%08x, wmask: 0x%02x\n",
             addr, data, wmask);
            putchar(data & wmask);
        } else {
            SIMERROR("CPP MMIO Write to unknown address: 0x%08x\n", addr);
        }
    }

    ~Simlator() {
        cs_close(&cap_handle);
        // 这里不一定要 delete top，可以由外部管理，也可以在这里管理
    }
};

extern "C" void handle_sys_brk() {
    Simlator::instance->print_trap_state(0);
}

extern "C" void handle_mem_access_error(uint32_t addr, uint32_t mapped_addr) {
    Simlator::instance->print_mem_access_error(addr, mapped_addr);
}

extern "C" const char* get_img_path() {
    return Simlator::instance ? Simlator::instance->get_img_path() : "";
}

extern "C" void mmio_write(uint32_t addr, int data, uint8_t wmask) {
    Simlator::instance->print_mmio_write(addr, data, wmask);
}

extern "C" uint64_t mmio_read(uint32_t addr) {
    return Simlator::instance->print_mmio_read(addr);
}

Simlator* Simlator::instance = nullptr;

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Vtop_TopMiniRV* top = new Vtop_TopMiniRV;
    Simlator cpu_sim(top, argc, argv);
    cpu_sim.run();
    return -1;
}
