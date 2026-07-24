#include "common.h"
#include "monitor.h"
#include "Simlator.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>

static uint8_t* pmem = nullptr;

// ysyxSoC MROM：0x20000000，大小 4KB；内容由 load_mrom(.bin) 填入，DPI mrom_read 读出
constexpr uint32_t MROM_BASE = 0x20000000;
constexpr size_t   MROM_SIZE = 0x1000;
static uint8_t mrom[MROM_SIZE];

constexpr uint32_t SRAM_BASE = 0x0f000000;
constexpr size_t   SRAM_SIZE = 0x2000;

bool in_mrom(paddr_t addr) {
    return addr >= MROM_BASE && addr < MROM_BASE + MROM_SIZE;
}
bool in_sram(paddr_t addr) {
    return addr >= SRAM_BASE && addr < SRAM_BASE + SRAM_SIZE;
}

extern "C" void mrom_read(int32_t addr, int32_t *data);

uint8_t* guest_to_host(paddr_t paddr) {
    Assert(in_sram(paddr), "guest_to_host: addr 0x%08x not in SRAM", (uint32_t)paddr);
    return pmem + (paddr - SRAM_BASE);
}
paddr_t host_to_guest(uint8_t *haddr) { return haddr - pmem + SRAM_BASE; }

/** 按 guest 物理地址读：MROM / SRAM；供 sdb `x` 与 DiffTest 使用（非 RTL 总线） */
word_t paddr_read(paddr_t addr, int len) {
    if (in_mrom(addr)) {
        Assert((addr & 3u) == 0 && len == 4,
               "MROM host read must be word-aligned, addr=0x%08x len=%d", (uint32_t)addr, len);
        int32_t data;
        mrom_read((int32_t)addr, &data);
        return (word_t)data;
    }
    if (in_sram(addr)) {
        switch (len) {
            case 1: return *(uint8_t  *)(pmem + addr - SRAM_BASE);
            case 2: return *(uint16_t *)(pmem + addr - SRAM_BASE);
            case 4: return *(uint32_t *)(pmem + addr - SRAM_BASE);
            IFDEF(CONFIG_ISA64, case 8: return *(uint64_t *)(pmem + addr - SRAM_BASE));
            default: MUXDEF(CONFIG_RT_CHECK, assert(0), return 0);
        }
    }
    Assert(0, "paddr_read: unmapped addr 0x%08x", (uint32_t)addr);
    return 0;
}

word_t pmem_read(paddr_t addr, int len) {
    return paddr_read(addr, len);
}

void pmem_write(paddr_t addr, int len, word_t data) {
    Assert(in_sram(addr), "pmem_write: addr 0x%08x not in SRAM", (uint32_t)addr);
    switch (len) {
        case 1: *(uint8_t  *)(pmem + addr - SRAM_BASE) = data; return;
        case 2: *(uint16_t *)(pmem + addr - SRAM_BASE) = data; return;
        case 4: *(uint32_t *)(pmem + addr - SRAM_BASE) = data; return;
        IFDEF(CONFIG_ISA64, case 8: *(uint64_t *)(pmem + addr - SRAM_BASE) = data; return);
        IFDEF(CONFIG_RT_CHECK, default: assert(0));
    }
}

/** 把 char-test.bin 装入 mrom[]；超出 4KB 截断。path==nullptr 则清零。 */
long load_mrom(const char *path) {
    memset(mrom, 0, sizeof(mrom));
    if (path == nullptr) {
        Log("MROM: no image, filled with zeros");
        return 0;
    }

    FILE *fp = fopen(path, "rb");
    Assert(fp, "Can not open MROM image '%s'", path);

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    long n = std::min<long>(size, (long)MROM_SIZE);
    Assert(fread(mrom, 1, n, fp) == (size_t)n, "fread MROM failed");
    fclose(fp);

    Log("MROM loaded from %s, %ld bytes -> [0x%08x, 0x%08x)",
        path, n, MROM_BASE, MROM_BASE + (uint32_t)n);
    return n;
}

void init_mem() {
    if (pmem == nullptr) {
        pmem = (uint8_t*)malloc(SRAM_SIZE);
        Assert(pmem, "Cannot allocate host SRAM mirror, size = %zu", SRAM_SIZE);
        memset(pmem, 0, SRAM_SIZE);
    }
    Log("SRAM mirror [" FMT_PADDR ", " FMT_PADDR "]",
        (paddr_t)SRAM_BASE, (paddr_t)(SRAM_BASE + SRAM_SIZE - 1));
    Log("pmem host ptr = %p (malloc %zu)", pmem, SRAM_SIZE);
    Log("Memory Trace: %s", MUXDEF(CONFIG_MTRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
}

extern void print_trap_state(Simlator* cpu, int state);

extern "C" {
    void handle_mem_access_error(uint32_t addr, uint32_t mapped_addr) {
        auto cpu = Simlator::instance;
        print_trap_state(cpu, -1);
        SIMERROR("%s Memory access error at address: 0x%08x, mapped address: 0x%08x\n",
                cpu->get_img_name(), addr, mapped_addr);
        cpu->set_state(SimState::ABORT);
        exit(-1);
    }

    void handle_sys_brk() {
        auto cpu = Simlator::instance;
        print_trap_state(cpu, cpu->get_gpr(10));
        cpu->set_state(SimState::END);
    }

    // SoC DPI：后续按讲义接 flash/mrom 镜像；先保证可链接、不立刻 assert
    void flash_read(int32_t addr, int32_t *data) {
        // flash XIP 地址空间在 SoC 侧；此处暂返回 0（nop），避免未实现就 fatal
        *data = 0;
        (void)addr;
        assert(0);
    }

    // 按字返回小端内容（与 AXI RDATA 一致）
    void mrom_read(int32_t addr, int32_t *data) {
        uint32_t off = (uint32_t)addr - MROM_BASE;
        Assert(off < MROM_SIZE && (off & 3u) == 0,
               "mrom_read bad addr 0x%08x", (uint32_t)addr);
        memcpy(data, &mrom[off], sizeof(int32_t));
    }
}
