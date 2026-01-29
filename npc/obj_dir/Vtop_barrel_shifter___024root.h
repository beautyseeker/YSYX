// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vtop_barrel_shifter.h for the primary calling header

#ifndef VERILATED_VTOP_BARREL_SHIFTER___024ROOT_H_
#define VERILATED_VTOP_BARREL_SHIFTER___024ROOT_H_  // guard

#include "verilated.h"


class Vtop_barrel_shifter__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vtop_barrel_shifter___024root final {
  public:

    // DESIGN SPECIFIC STATE
    VL_IN8(clk,0,0);
    VL_IN8(rst,0,0);
    VL_IN8(din,7,0);
    VL_OUT8(dout,7,0);
    CData/*7:0*/ top_barrel_shifter__DOT__dout_reg;
    CData/*0:0*/ __VstlFirstIteration;
    CData/*0:0*/ __Vtrigprevexpr___TOP__clk__0;
    CData/*0:0*/ __Vtrigprevexpr___TOP__rst__0;
    IData/*31:0*/ __VactIterCount;
    VlUnpacked<QData/*63:0*/, 1> __VstlTriggered;
    VlUnpacked<QData/*63:0*/, 1> __VactTriggered;
    VlUnpacked<QData/*63:0*/, 1> __VnbaTriggered;

    // INTERNAL VARIABLES
    Vtop_barrel_shifter__Syms* vlSymsp;
    const char* vlNamep;

    // CONSTRUCTORS
    Vtop_barrel_shifter___024root(Vtop_barrel_shifter__Syms* symsp, const char* namep);
    ~Vtop_barrel_shifter___024root();
    VL_UNCOPYABLE(Vtop_barrel_shifter___024root);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
