#include "Simlator.hpp"
#include "npc.h"
#include "isa.h"

#ifdef CONFIG_RVE
const char *rv32_reg_name[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5"
};
#else
const char *rv32_reg_name[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};
#endif

Simlator* Simlator::instance = nullptr;

Simlator::Simlator(Vtop_TopMiniRV* NPC) : 
    top(NPC), sim_state(STOP) {
    // 初始化统计信息
    statistic = new CPU_Statistic();
    statistic->reset();
    config = new CPU_Config();
    dut_data = new DUT_data();
    npc_state = new NPC_State();
    IFDEF(CONFIG_ITRACE, itracer = new InstTracer());
    Verilated::traceEverOn(true);
    tfp = new VerilatedVcdC;
    top->trace(tfp, 99); // 追踪深度，99表示记录所有子模块
    config->vcd_path = std::getenv("VCD_FILE") ? 
    std::getenv("VCD_FILE") : std::string(get_img_name())+"waveform.vcd";
    tfp->open(config->vcd_path.c_str());
    instance = this; // 设置单例实例
}

Simlator::~Simlator() {
    delete statistic;
    delete config;
    delete dut_data;
    delete npc_state;
    tfp->close();
    delete tfp;
    IFDEF(CONFIG_ITRACE, delete itracer);
    instance = nullptr; // 清除单例实例
}

void Simlator::clock_tick(uint64_t n) {
    for(uint64_t i = 0; i < n; ++i) {
        top->clk = 1; top->eval();
        tfp->dump(statistic->sim_tick++);
        top->clk = 0; top->eval();
        tfp->dump(statistic->sim_tick++);
    }
}

void Simlator::init(int argc, char **argv) {
    config->parse(argc, argv); // 这里可以传入实际的命令行参数
    // top->trace(tfp, 99); // 追踪深度，99表示记录所有子模块
    // config->vcd_path = std::getenv("VCD_FILE") ? 
    // std::getenv("VCD_FILE") : std::string(get_img_name())+"waveform.vcd";
    // tfp->open(config->vcd_path.c_str());
    reset();
    init_DUT_state();
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

extern void difftest_step(vaddr_t npc_pc, vaddr_t npc_next_pc);
extern void wp_scan_wp();

void Simlator::execute(uint64_t n) {
    for (uint64_t i = 0; i < n && sim_state == RUNNING; ++i) {
        clock_tick(1);
        statistic->cycle_nr++;

        // SimpleBus IFU: 每条指令完成需要若干个周期 (IDLE(指令从ROM被取出且合法) + WAIT(基于访存延迟))
        // 只有 fire = 1 完成握手，指令才真正完成执行
        bool inst_valid = (top->rst_n != 0) && top->fire;

        if (inst_valid) {
            statistic->inst_nr++;
        }

        IFDEF(CONFIG_DIFFTEST, {
            if (inst_valid) {
                update_DUT_state();
                difftest_step(top->PC_current, top->PC_next);
            }
        });

        IFDEF(CONFIG_WATCHPOINT, {
            if (inst_valid) {
                wp_scan_wp();
            }
        });

        IFDEF(CONFIG_ITRACE, {
            if (inst_valid) {
                char asm_str[128];
                itracer->disassemble(asm_str, sizeof(asm_str), top->PC_current, (uint8_t*)&top->instruction, 4);
                snprintf(npc_state->logbuf, sizeof(npc_state->logbuf), 
                ANSI_FG_BLUE "[cycle=%lu] [inst_nr=%lu] [PC=0x%08x] inst: %08x %s\n" ANSI_NONE, 
                statistic->cycle_nr, statistic->inst_nr, top->PC_current, top->instruction, asm_str);
                printf("%s", npc_state->logbuf);
                itracer->push_irring(npc_state->logbuf);
            }
        });
        npc_state->current_pc = top->PC_current;
        npc_state->next_pc = top->PC_next;
    }
}

void InstTracer::push_irring(const char* log) {
    strncpy(iring_buf[iring_head], log, sizeof(iring_buf[0]) - 1);
    iring_buf[iring_head][sizeof(iring_buf[0]) - 1] = '\0';
    iring_head = (iring_head + 1) % IRING_SIZE;
}

void InstTracer::print_irring() {
    int idx = iring_head;
    printf(ANSI_FMT("--------------Instruction Ring Buffer (last %d instructions)-------------:\n", 
    ANSI_FG_CYAN), IRING_SIZE);
    for (int i = 0; i < IRING_SIZE; ++i) {
        idx = (idx - 1 + IRING_SIZE) % IRING_SIZE; // 逆序打印
        if (iring_buf[idx][0] != '\0') { // 只打印有效的日志
            Trace("Iring", ANSI_FG_YELLOW, "%s", iring_buf[idx]);
        }
    }
    printf(ANSI_FMT("--------------End of Instruction Ring Buffer-------------\n", ANSI_FG_CYAN));
}


extern uint64_t get_time_internal();
bool Simlator::reset() {
    sim_state = RUNNING;
    top->rst_n = 0;
    clock_tick(5);
    top->rst_n = 1;
    statistic->reset();
    statistic->boot_time = get_time_internal();
    return true;
}

void Simlator::run() {
    if(sim_state == RUNNING) {
    #if ALWAYS_RUN
        while (true) {
            execute(1);
        }
    #else
        execute(-1);
    #endif
    }
}

word_t Simlator::get_gpr(int idx) const {
    if (idx < 0 || idx >= NR_GPR) {
        SIMERROR("Invalid GPR index: %d\n", idx);
        return 0xdeadbeef;
    }
    return top->gpr[idx];
}

word_t Simlator::get_gpr(const char *name) const {
    for (int i = 0; i < NR_GPR; i++) {
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

void Simlator::init_DUT_state() {
    memset(dut_data, 0, sizeof(DUT_data));
    dut_data->pc = top->PC_current;
}

void Simlator::update_DUT_state() {
    for (int i = 0; i < NR_GPR; i++) {
        dut_data->gpr[i] = top->gpr[i];
    }
    dut_data->pc = top->PC_current;
}

void Simlator::init_NPC_state() {
    npc_state->state = sim_state;
    npc_state->current_pc = top->PC_current;
    npc_state->next_pc = top->PC_next;
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
        for (int i = 0; i < NR_GPR; i++) {
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
