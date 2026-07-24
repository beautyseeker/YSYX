#include "Simlator.hpp"
#include "npc.h"
#include "isa.h"
#include <cstring>
#include <cstdlib>

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

Simlator::Simlator(DutTop* NPC) :
    top(NPC), sim_state(STOP) {
    statistic = new CPU_Statistic();
    statistic->reset();
    statistic->sim_tick = 0;
    config = new CPU_Config();
    dut_data = new DUT_data();
    npc_state = new NPC_State();
    IFDEF(CONFIG_ITRACE, itracer = new InstTracer());
    IFDEF(CONFIG_WAVE, {
        Verilated::traceEverOn(true);
        tfp = new VerilatedVcdC;
        top->trace(tfp, 99);
        config->vcd_path = std::getenv("VCD_FILE") ?
            std::getenv("VCD_FILE") : std::string(get_img_name()) + "waveform.vcd";
        tfp->open(config->vcd_path.c_str());
    });
    instance = this;
}

Simlator::~Simlator() {
    delete statistic;
    delete config;
    delete dut_data;
    delete npc_state;
    IFDEF(CONFIG_WAVE, {
        if (tfp) {
            tfp->close();
            delete tfp;
        }
    });
    IFDEF(CONFIG_ITRACE, delete itracer);
    instance = nullptr;
}

void Simlator::clock_tick(uint64_t n) {
    for (uint64_t i = 0; i < n; ++i) {
        top->clock = 1; top->eval();
        IFDEF(CONFIG_WAVE, if (tfp) tfp->dump(statistic->sim_tick));
        statistic->sim_tick++;
        top->clock = 0; top->eval();
        IFDEF(CONFIG_WAVE, if (tfp) tfp->dump(statistic->sim_tick));
        statistic->sim_tick++;
    }
}

void Simlator::init(int argc, char **argv) {
    config->parse(argc, argv);
    // 不在这里 reset：等 init_monitor→load_mrom 装好镜像后再复位
    init_DUT_state();
}

bool Simlator::load_rom(const char* rom_path) {
    (void)rom_path;
    return true;
}

bool Simlator::load_ram(const char* ram_path) {
    (void)ram_path;
    return true;
}

extern void difftest_step(vaddr_t npc_pc, vaddr_t npc_next_pc);
extern void wp_scan_wp();

void Simlator::execute(uint64_t n) {
    for (uint64_t i = 0; i < n && sim_state == RUNNING; ++i) {
        // fire 在提交沿之前为高：本拍上升沿才会写回 GPR / 更新 PC。
        // 必须先采样再 clock，提交后再做 DiffTest。
        bool inst_valid = !in_reset() && get_fire();
        vaddr_t commit_pc = get_pc();
        vaddr_t commit_next_pc = get_pc_next();
        uint32_t commit_inst = get_inst();

        clock_tick(1);
        statistic->cycle_nr++;

        if (inst_valid) {
            statistic->inst_nr++;
        }

        IFDEF(CONFIG_DIFFTEST, {
            if (inst_valid) {
                update_DUT_state();
                difftest_step(commit_pc, commit_next_pc);
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
                itracer->disassemble(asm_str, sizeof(asm_str), commit_pc, (uint8_t*)&commit_inst, 4);
                snprintf(npc_state->logbuf, sizeof(npc_state->logbuf),
                ANSI_FG_BLUE "[cycle=%lu] [inst_nr=%lu] [PC=0x%08x] inst: %08x %s\n" ANSI_NONE,
                statistic->cycle_nr, statistic->inst_nr, commit_pc, commit_inst, asm_str);
                printf("%s", npc_state->logbuf);
                itracer->push_irring(npc_state->logbuf);
            }
        });
        npc_state->current_pc = get_pc();
        npc_state->next_pc = get_pc_next();
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
        idx = (idx - 1 + IRING_SIZE) % IRING_SIZE;
        if (iring_buf[idx][0] != '\0') {
            Trace("Iring", ANSI_FG_YELLOW, "%s", iring_buf[idx]);
        }
    }
    printf(ANSI_FMT("--------------End of Instruction Ring Buffer-------------\n", ANSI_FG_CYAN));
}


extern uint64_t get_time_internal();
bool Simlator::reset() {
    sim_state = RUNNING;
    // SoC：clock + 高有效 reset；外部输入拉到确定值
    top->reset = 1;
    clock_tick(15);
    top->externalPins_gpio_in = 0;
    top->externalPins_ps2_clk = 0;
    top->externalPins_ps2_data = 0;
    top->externalPins_uart_rx = 1;
    top->reset = 0;
    clock_tick(15);
    statistic->reset();
    statistic->boot_time = get_time_internal();
    return true;
}

void Simlator::run() {
    if (sim_state == RUNNING) {
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
    return DUT_CPU_SIG(top->rootp, gpr)[idx];
}

word_t Simlator::get_gpr(const char *name) const {
    for (int i = 0; i < NR_GPR; i++) {
        if (strcmp(name, rv32_reg_name[i]) == 0) {
            return get_gpr(i);
        }
    }
    if (strcmp(name, "pc") == 0) {
        return get_pc();
    }
    SIMERROR("Invalid register name: %s\n", name);
    return 0xdeadbeef;
}

void Simlator::init_DUT_state() {
    memset(dut_data, 0, sizeof(DUT_data));
    dut_data->pc = get_pc();
}

void Simlator::update_DUT_state() {
    for (int i = 0; i < NR_GPR; i++) {
        dut_data->gpr[i] = get_gpr(i);
    }
    dut_data->pc = get_pc();
}

void Simlator::init_NPC_state() {
    npc_state->state = sim_state;
    npc_state->current_pc = get_pc();
    npc_state->next_pc = get_pc_next();
}

word_t Simlator::mem_read(vaddr_t addr, int len) {
    return vaddr_read(addr, len);
}

void Simlator::mem_write(vaddr_t addr, int len, word_t data) {
    extern void vaddr_write(vaddr_t addr, int len, word_t data);
    vaddr_write(addr, len, data);
}

word_t isa_reg_str2val(const char* name, bool *success) {
    if (Simlator::instance) {
        *success = true;
        uint32_t val = Simlator::instance->get_gpr(name);
        if (val == 0xdeadbeef) {
            *success = false;
        }
        return val;
    } else {
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
    } else {
        SIMERROR("Simulator instance is not initialized!");
    }
}

extern "C" {
    const char* get_img_path() {
        if (Simlator::instance) {
            const char *img_path = Simlator::instance->get_img_path();
            printf("Current loaded image: %s\n", img_path);
            return img_path;
        } else {
            SIMERROR("Simulator instance is not initialized!");
            return nullptr;
        }
    }
}
