// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Tracing implementation internals

#include "verilated_vcd_c.h"
#include "Vtop_barrel_shifter__Syms.h"


void Vtop_barrel_shifter___024root__trace_chg_0_sub_0(Vtop_barrel_shifter___024root* vlSelf, VerilatedVcd::Buffer* bufp);

void Vtop_barrel_shifter___024root__trace_chg_0(void* voidSelf, VerilatedVcd::Buffer* bufp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop_barrel_shifter___024root__trace_chg_0\n"); );
    // Body
    Vtop_barrel_shifter___024root* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<Vtop_barrel_shifter___024root*>(voidSelf);
    Vtop_barrel_shifter__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;
    Vtop_barrel_shifter___024root__trace_chg_0_sub_0((&vlSymsp->TOP), bufp);
}

void Vtop_barrel_shifter___024root__trace_chg_0_sub_0(Vtop_barrel_shifter___024root* vlSelf, VerilatedVcd::Buffer* bufp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop_barrel_shifter___024root__trace_chg_0_sub_0\n"); );
    Vtop_barrel_shifter__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    uint32_t* const oldp VL_ATTR_UNUSED = bufp->oldp(vlSymsp->__Vm_baseCode + 1);
    bufp->chgBit(oldp+0,(vlSelfRef.clk));
    bufp->chgBit(oldp+1,(vlSelfRef.rst));
    bufp->chgCData(oldp+2,(vlSelfRef.din),8);
    bufp->chgCData(oldp+3,(vlSelfRef.dout),8);
    bufp->chgCData(oldp+4,(vlSelfRef.top_barrel_shifter__DOT__dout_reg),8);
    bufp->chgCData(oldp+5,((0x000000ffU & ((IData)(vlSelfRef.top_barrel_shifter__DOT__dout_reg) 
                                           >> 1U))),8);
}

void Vtop_barrel_shifter___024root__trace_cleanup(void* voidSelf, VerilatedVcd* /*unused*/) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop_barrel_shifter___024root__trace_cleanup\n"); );
    // Locals
    VlUnpacked<CData/*0:0*/, 1> __Vm_traceActivity;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        __Vm_traceActivity[__Vi0] = 0;
    }
    // Body
    Vtop_barrel_shifter___024root* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<Vtop_barrel_shifter___024root*>(voidSelf);
    Vtop_barrel_shifter__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    vlSymsp->__Vm_activity = false;
    __Vm_traceActivity[0U] = 0U;
}
