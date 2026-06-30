#ifndef __SIMULATOR_HPP__
#define __SIMULATOR_HPP__

#include "Vtop_TopMiniRV.h"
#include "isa.h"
#include <string>
#include "verilated.h"
#include "verilated_vcd_c.h"

// 仿真状态定义（保持与 SDB 同步）
enum SimState { RUNNING, STOP, ABORT, QUIT, END };

struct NPC_State {
    SimState state;
    vaddr_t halt_pc;
    uint32_t halt_ret;
    vaddr_t current_pc;
    vaddr_t next_pc;
    char logbuf[128];
};


struct CPU_Statistic
{
    uint64_t sim_tick;
    uint64_t cycle_nr;
    uint64_t inst_nr;
    uint64_t boot_time;
    uint64_t branch_cnt;
    uint64_t branch_miss;
    uint64_t jump_cnt;
    uint64_t load_cnt;
    uint64_t store_cnt;
    uint64_t ecall_cnt;
    uint64_t ebreak_cnt;
    uint64_t csr_cnt;
    uint64_t illegal_cnt;

    bool reset() {
        cycle_nr = 0;
        inst_nr = 0;
        boot_time = 0;
        branch_cnt = 0;
        branch_miss = 0;
        jump_cnt = 0;
        load_cnt = 0;
        store_cnt = 0;
        ecall_cnt = 0;
        ebreak_cnt = 0;
        csr_cnt = 0;
        illegal_cnt = 0;
        return true;
    }
};

struct CPU_Config {
    std::string img_path;
    std::string vcd_path;
    int cycle_max = 0;
    uint64_t load_inst_num = 0;

    // 标记为 inline，告诉编译器这是允许在多处定义的
    inline void parse(int argc, char **argv) {
        for (int i = 1; i < argc; ++i) {
            if (strncmp(argv[i], "/home", 5) == 0) {
                img_path = argv[i];
            } else if (strncmp(argv[i], "CYCLES=", 7) == 0) {
                cycle_max = atoi(argv[i] + 7);
            } else if (strncmp(argv[i], "VCD=", 4) == 0) {
                vcd_path = argv[i] + 4;
            }
        }

    }

    inline const char* get_filename() const {
        const char* filename = strrchr(img_path.c_str(), '/');
        // printf("Extracting filename from path: %s\n", img_path.c_str());
        if (filename) {
            // printf("Extracted filename: %s\n", filename + 1);
            return filename + 1; // 返回文件名部分
        } else {
            // printf("No path separator found, using entire string as filename: %s\n", img_path.c_str());
            return img_path.c_str(); // 如果没有路径分隔符，直接返回输入字符串
        }
    }

};

class InstTracer {
private:
    static const int IRING_SIZE = 16;
    char iring_buf[IRING_SIZE][256];
    int iring_head;
    const char* empty_str = "empty";
public:
    InstTracer(int size = IRING_SIZE) : iring_head(0) {
        for (int i = 0; i < IRING_SIZE; ++i) {
            strncpy(iring_buf[i], empty_str, sizeof(iring_buf[i]) - 1);
            iring_buf[i][sizeof(iring_buf[i]) - 1] = '\0';
        }
    }
    void disassemble(char *asm_str, int asm_size, uint64_t pc, uint8_t *code, int nbyte);
    void init_disasm();
    void push_irring(const char* log);
    void push_irring(vaddr_t pc);
    void print_irring();
};


class Simlator {
private:
    // 2. 内部仿真计数器
    CPU_Statistic* statistic;
    CPU_Config* config;
    SimState sim_state;

    void clock_tick(uint64_t n);

public:
    // 单例模式，方便 Bridge 层访问
    Vtop_TopMiniRV* top;
    DUT_data* dut_data;
    NPC_State* npc_state;
    VerilatedVcdC* tfp;
    IFDEF(CONFIG_ITRACE, InstTracer* itracer);
    static Simlator* instance;

    Simlator(Vtop_TopMiniRV* DUT);
    ~Simlator();

    // --- 仿真初始化接口 ---
    // 构造函数中完成仿真环境的初始化，包括加载镜像、设置初始状态等
    bool load_rom(const char* rom_path);
    bool load_ram(const char* ram_path);

    // --- 核心驱动接口 ---
    void execute(uint64_t n);      // 推动时钟翻转 n 次
    bool reset();
    void run();
    void init(int argc, char **argv);

    // --- 硬件状态访问接口 (Getter) ---
    // 这些接口供 Bridge 层调用，从而间接服务于 SDB 和 Trace
    vaddr_t get_pc() const { return top->PC_current; }
    word_t get_inst() const { return top->instruction; }
    word_t get_gpr(int idx) const;
    word_t get_gpr(const char *name) const;
    
    // --- 内存访问接口 ---
    // 供 SDB 扫描内存或 DiffTest 使用
    word_t mem_read(vaddr_t addr, int len);
    void mem_write(vaddr_t addr, int len, word_t data);

    // --- 仿真控制接口 ---
    SimState get_state() const { return sim_state; }
    void set_state(SimState s) { sim_state = s; }
    CPU_Statistic* get_statistic() const { return statistic; }
    const char* get_img_name() const { return config->get_filename(); }
    const char* get_img_path() const { return config->img_path.c_str(); }
    void init_DUT_state();
    void update_DUT_state();
    void init_NPC_state();
};

#endif
