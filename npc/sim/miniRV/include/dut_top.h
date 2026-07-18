#pragma once
// 顶层类型由 Makefile -DTOP_NAME / -DTOP_HEADER / -DTOP_ROOT_HEADER 注入
#define DUT_TOP_STRINGIFY(x) #x
#define DUT_TOP_TOSTRING(x) DUT_TOP_STRINGIFY(x)
#include DUT_TOP_TOSTRING(TOP_HEADER)
#include DUT_TOP_TOSTRING(TOP_ROOT_HEADER)

#ifndef TOP_NAME
#error "TOP_NAME must be defined by Makefile (e.g. -DTOP_NAME=VysyxSoCFull)"
#endif

typedef TOP_NAME DutTop;

// Verilator 把顶层模块名也编进层次：ysyxSoCFull.asic.cpu.cpu.*
// 以 build/obj/.../VysyxSoCFull___024root.h 为准
#define DUT_CPU_SIG(rootp, sig) \
    ((rootp)->ysyxSoCFull__DOT__asic__DOT__cpu__DOT__cpu__DOT__##sig)
