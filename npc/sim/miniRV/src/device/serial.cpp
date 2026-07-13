#include "npc.h"
#include <sys/time.h>
#include <stdio.h>
#include "Simlator.hpp"

enum MMIO_ADDR {
    SERIAL_ADDR = 0x10000000,
    RTC_ADDR = 0x10000048
};

uint64_t get_time_internal() {
    struct timeval now;
    gettimeofday(&now, NULL);
    uint64_t us = now.tv_sec * 1000000 + now.tv_usec;
    return us;
}

uint64_t get_uptime() {
    if (Simlator::instance == nullptr) return 0;
    uint64_t boot_time = Simlator::instance->get_statistic()->boot_time;
    if (boot_time == 0) return 0;
    uint64_t now = get_time_internal();
    return now - boot_time;
}

extern "C" {
    uint64_t mmio_read(uint32_t addr) {
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

    void mmio_write(uint32_t addr, int data, uint32_t wmask) {
        if (addr == SERIAL_ADDR) {
            IOLOG("CPP MMIO Write to SERIAL address: 0x%08x, data: 0x%08x, wmask: 0x%02x\n",
                addr, data, wmask);
            putchar(data & wmask);
        } else {
            SIMERROR("CPP MMIO Write to unknown address: 0x%08x\n", addr);
        }
    }
}
