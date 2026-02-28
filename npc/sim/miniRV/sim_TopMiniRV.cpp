#include "Vtop_TopMiniRV.h"
#include "verilated.h"
#include <capstone/capstone.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define DEFAULT_SIM_CYCLES SIM_CYCLES
#define RING_BUFFER_SIZE 5

extern "C" void handle_sys_brk();
extern "C" void handle_mem_access_error(uint32_t addr, uint32_t mapped_addr);
extern "C" const char* get_img_path();
extern "C" void mmio_write(uint32_t addr, int data, uint8_t wmask);
extern "C" uint64_t mmio_read(uint32_t addr);

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

typedef enum {
    SERIAL_ADDR = 0x10000000,
    RTC_ADDR = 0x10000004
} MMIO_ADDR;

typedef enum {
    RUNNING,
    STOP
} SimState;

typedef enum {
    OUT_OF_CYCLES = 0,
    MEM_ACCESS_ERROR,
    NORMAL_EXIT
} ErrorCause;

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
    uint32_t get_gpr(int idx) const { return top->gpr[idx]; }
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
        size_t count = cs_disasm(cap_handle, (uint8_t*)&(top->instruction), 
        4, top->PC_current, 1, &insn);
        if (count > 0) {
            asm_str = std::string(insn[0].mnemonic) + " " + insn[0].op_str;
            inst_nr++;
            cs_free(insn, count);
        } else {
            asm_str = "<invalid>";
        }
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

    const char* get_img_path() const {
        //获取img_path文件读入的指令数量，并打印出来
        if (!img_path.empty()) {
            FILE *fp = fopen(img_path.c_str(), "rb");
            if (fp) {
                fseek(fp, 0, SEEK_END);
                long size = ftell(fp);
                fclose(fp);
                printf("Load Image file from: %s, size: %ld bytes, instructions: %ld\n", 
                img_path.c_str(), size, size / 4);
            } else {
                printf("Failed to open image file: %s\n", img_path.c_str());
            }
        }
        return img_path.empty() ? "" : img_path.c_str();
    }

    // 封装时钟步进逻辑
    void clock_step() {
        set_asm();
        ring_buffer[inst_nr % RING_BUFFER_SIZE] = 
        {.PC_current = top->PC_current, .instruction = top->instruction, .asm_str = asm_str};

        top->clk = 1; top->eval();
        // 这里可以做一些时钟上升沿的同步逻辑
        top->clk = 0; top->eval();
        cycle_nr++;
    }

    void run() {
        while (cycle_nr < cycle_max) {
            clock_step();
            // 这里可以添加一些周期级的监控逻辑，比如检查特定寄存器的值，或者监控特定的指令执行等
        }
        print_sim_reach_max();
    }

    void reset() {
        top->rst_n = 0;
        for (int i = 0; i < 5; ++i) {
            clock_step();
        }
        top->rst_n = 1;
        cycle_nr = 0;
        inst_nr = 0;
        boot_time = get_time_internal();
    }

    void print_ring_buffer() const {
        printf("-------------------------Recent %d instructions in %s-----------------\n", 
        RING_BUFFER_SIZE, img_name.c_str());
        int start = inst_nr % RING_BUFFER_SIZE;
        for (int i = 0; i < RING_BUFFER_SIZE; i++) {
            int idx = (start + i) % RING_BUFFER_SIZE;
            printf("\033[34m  [%d] %s \033[0m\n", i, ring_buffer[idx].to_string().c_str());
        }
    }

    void print_trap_state(int state) const {
        print_ring_buffer();
        if(state == 0) {
            printf("\033[32m HIT A GOOD TRAP in %s! \033[0m\n", img_name.c_str());
        } else {
            printf("\033[31m HIT A BAD TRAP in %s due to %s! \033[0m\n", 
            img_name.c_str(), error_cause_names[error_cause]);
        }
        print_statistics();
    }

    void print_mem_access_error(uint32_t addr, uint32_t mapped_addr) {
        printf("Memory access error at address: 0x%08x (mapped to 0x%08x)\n", addr, mapped_addr);
        error_cause = MEM_ACCESS_ERROR;
        print_trap_state(-1);
    }

    void print_sim_reach_max() {
        printf("Simulation reached max cycles (%lu) in %s!\n", cycle_max, img_name.c_str());
        error_cause = OUT_OF_CYCLES;
        print_trap_state(-1);
    }

    void print_regs() const {
        printf("Registers:\n");
        for (int i = 0; i < 32; i++) {
            printf("%s: 0x%08x ", regs[i], top->gpr[i]);
            if ((i + 1) % 8 == 0) printf("\n");
        }
    }

    void print_statistics() const {
        printf("---------------------Simulation statistics----------------------\n");
        if (cycle_nr > 0 && inst_nr > 0) {
            printf("%s Simulation time: %.2f ms\n", img_name.c_str(), get_uptime() / 1000.0);
            printf("Cycles executed:%lu Instructions executed:%lu CPI: %.2f\n", 
                cycle_nr, inst_nr, (double)cycle_nr / inst_nr);
        }
        exit(0);
    }

    uint64_t print_mmio_read(uint32_t addr) const {
        if (addr == RTC_ADDR) {
            uint64_t now = get_time_internal();
            printf("CPP MMIO Read from RTC address: 0x%08x system time:%lu\n", addr, now);
            return now;
        } else {
            printf("MMIO Read from unknown address: 0x%08x\n", addr);
            return 0;
        }
    }

    void print_mmio_write(uint32_t addr, uint32_t data, uint8_t wmask = 0) const {
        if (addr == SERIAL_ADDR) {
            printf("CPP MMIO Write to SERIAL address: 0x%08x, data: 0x%08x, wmask: 0x%02x\n",
             addr, data, wmask);
            putchar(data & wmask);
        } else {
            printf("CPP MMIO Write to unknown address: 0x%08x\n", addr);
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