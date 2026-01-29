// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vtop_barrel_shifter.h for the primary calling header

#include "Vtop_barrel_shifter__pch.h"

void Vtop_barrel_shifter___024root___ctor_var_reset(Vtop_barrel_shifter___024root* vlSelf);

Vtop_barrel_shifter___024root::Vtop_barrel_shifter___024root(Vtop_barrel_shifter__Syms* symsp, const char* namep)
 {
    vlSymsp = symsp;
    vlNamep = strdup(namep);
    // Reset structure values
    Vtop_barrel_shifter___024root___ctor_var_reset(this);
}

void Vtop_barrel_shifter___024root::__Vconfigure(bool first) {
    (void)first;  // Prevent unused variable warning
}

Vtop_barrel_shifter___024root::~Vtop_barrel_shifter___024root() {
    VL_DO_DANGLING(std::free(const_cast<char*>(vlNamep)), vlNamep);
}
