#include "Simlator.hpp"
#include "npc.h"
#include "isa.h"

const char *rv32_reg_name[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

Simlator* Simlator::instance = nullptr;

Simlator::Simlator(Vtop_TopMiniRV* NPC, int argc, char **argv) : 
    top(NPC), sim_state(STOP) {
    // 初始化统计信息
    statistic = new CPU_Statistic();
    statistic->reset();
    config = new CPU_Config();
    for(int i = 0; i < argc; ++i) {
        printf("Argument %d: %s\n", i, argv[i]);
    }
    config->parse(argc, argv);
    reset();
    instance = this; // 设置单例实例
}

Simlator::~Simlator() {
    delete statistic;
    delete config;
    instance = nullptr; // 清除单例实例
}


bool Simlator::load_rom(const char* rom_path) {
    // 这里可以添加加载 ROM 文件的逻辑，比如读取文件内容并写入仿真内存
    // 目前暂时不实现，直接返回 true 表示成功
    return true;
}

bool Simlator::load_ram(const char* ram_path) {
    // 这里可以添加加载 RAM 文件的逻辑，比如读取文件内容并写入仿真内存
    // 目前暂时不实现，直接返回 true 表示成功
    return true;
}

void Simlator::clock_step(uint64_t n) {
    for (uint64_t i = 0; i < n && sim_state == RUNNING; ++i) {
        statistic->inst_nr += (top->rst_n == 0) ? 0 : 1; // 如果处于复位状态，不增加指令计数

        // printf("[%ld] PC=0x%08x inst=0x%08x\n", 
        // statistic->inst_nr, top->PC_current, top->instruction);

        top->clk = 1; top->eval();
        // 这里可以做一些时钟上升沿的同步逻辑
        top->clk = 0; top->eval();
        statistic->cycle_nr++;
    }
}


extern uint64_t get_time_internal();
bool Simlator::reset() {
    sim_state = RUNNING;
    top->rst_n = 0;
    clock_step(2);
    top->rst_n = 1;
    statistic->reset();
    statistic->boot_time = get_time_internal();
    return true;
}

void Simlator::run() {
    if(sim_state == RUNNING) {
    #if ALWAYS_RUN
        while (true) {
            clock_step(1);
        }
    #else
        clock_step(-1);
    #endif
    }
}

word_t Simlator::get_gpr(int idx) const {
    if (idx < 0 || idx >= 32) {
        SIMERROR("Invalid GPR index: %d\n", idx);
        return 0xdeadbeef;
    }
    return top->gpr[idx];
}

word_t Simlator::get_gpr(const char *name) const {
    for (int i = 0; i < 32; i++) {
        if (strcmp(name, rv32_reg_name[i]) == 0) {
            return top->gpr[i];
        }
    }
    if(strcmp(name, "pc") == 0) {
        return top->PC_current;
    }
    SIMERROR("Invalid register name: %s\n", name);
    return 0xdeadbeef;
}

word_t isa_reg_str2val(const char* name, bool *success) {
    if(Simlator::instance) {
        *success = true;
        uint32_t val = Simlator::instance->get_gpr(name);
        if(val == 0xdeadbeef) {
            *success = false;
        }
        return val;
    } 
    else {
        SIMERROR("Simulator instance is not initialized!");
        *success = false;
        return 0xdeadbeef;
    }
}

void isa_reg_display() {
    if (Simlator::instance) {
        auto cpu = Simlator::instance;
        printf("\n------------------------- Register Dump -------------------------\n");
        for (int i = 0; i < 32; i++) {
            printf("%-4s= 0x%08x  ", rv32_reg_name[i], cpu->get_gpr(i));
            if ((i + 1) % 8 == 0) {
                printf("\n");
            }
        }
        printf("-----------------------------------------------------------------\n");
    } 
    else {
        SIMERROR("Simulator instance is not initialized!");
    }
}

extern "C" {
    const char* get_img_path() {
        if(Simlator::instance) {
            const char *img_path = Simlator::instance->get_img_path();
            printf("Current loaded image: %s\n", img_path);
            return img_path;
        } else {
            SIMERROR("Simulator instance is not initialized!");
            return nullptr;
        }
    }
}
