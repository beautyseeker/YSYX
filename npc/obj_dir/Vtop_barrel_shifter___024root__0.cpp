// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vtop_barrel_shifter.h for the primary calling header

#include "Vtop_barrel_shifter__pch.h"

#ifdef VL_DEBUG
VL_ATTR_COLD void Vtop_barrel_shifter___024root___dump_triggers__act(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag);
#endif  // VL_DEBUG

void Vtop_barrel_shifter___024root___eval_triggers__act(Vtop_barrel_shifter___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop_barrel_shifter___024root___eval_triggers__act\n"); );
    Vtop_barrel_shifter__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__VactTriggered[0U] = (QData)((IData)(
                                                    ((((~ (IData)(vlSelfRef.rst)) 
                                                       & (IData)(vlSelfRef.__Vtrigprevexpr___TOP__rst__0)) 
                                                      << 1U) 
                                                     | ((IData)(vlSelfRef.clk) 
                                                        & (~ (IData)(vlSelfRef.__Vtrigprevexpr___TOP__clk__0))))));
    vlSelfRef.__Vtrigprevexpr___TOP__clk__0 = vlSelfRef.clk;
    vlSelfRef.__Vtrigprevexpr___TOP__rst__0 = vlSelfRef.rst;
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vtop_barrel_shifter___024root___dump_triggers__act(vlSelfRef.__VactTriggered, "act"s);
    }
#endif
}

bool Vtop_barrel_shifter___024root___trigger_anySet__act(const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop_barrel_shifter___024root___trigger_anySet__act\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        if (in[n]) {
            return (1U);
        }
        n = ((IData)(1U) + n);
    } while ((1U > n));
    return (0U);
}

void Vtop_barrel_shifter___024root___nba_sequent__TOP__0(Vtop_barrel_shifter___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop_barrel_shifter___024root___nba_sequent__TOP__0\n"); );
    Vtop_barrel_shifter__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.top_barrel_shifter__DOT__dout_reg = ((IData)(vlSelfRef.rst)
                                                    ? 
                                                   ((0x00000080U 
                                                     & (VL_REDXOR_8(
                                                                    (0x1dU 
                                                                     & (IData)(vlSelfRef.top_barrel_shifter__DOT__dout_reg))) 
                                                        << 7U)) 
                                                    | (0x0000007fU 
                                                       & ((IData)(vlSelfRef.top_barrel_shifter__DOT__dout_reg) 
                                                          >> 1U)))
                                                    : 
                                                   ((0U 
                                                     == (IData)(vlSelfRef.din))
                                                     ? 0xacU
                                                     : (IData)(vlSelfRef.din)));
    vlSelfRef.dout = vlSelfRef.top_barrel_shifter__DOT__dout_reg;
}

void Vtop_barrel_shifter___024root___eval_nba(Vtop_barrel_shifter___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop_barrel_shifter___024root___eval_nba\n"); );
    Vtop_barrel_shifter__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if ((3ULL & vlSelfRef.__VnbaTriggered[0U])) {
        Vtop_barrel_shifter___024root___nba_sequent__TOP__0(vlSelf);
    }
}

void Vtop_barrel_shifter___024root___trigger_orInto__act(VlUnpacked<QData/*63:0*/, 1> &out, const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop_barrel_shifter___024root___trigger_orInto__act\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        out[n] = (out[n] | in[n]);
        n = ((IData)(1U) + n);
    } while ((1U > n));
}

bool Vtop_barrel_shifter___024root___eval_phase__act(Vtop_barrel_shifter___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop_barrel_shifter___024root___eval_phase__act\n"); );
    Vtop_barrel_shifter__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    Vtop_barrel_shifter___024root___eval_triggers__act(vlSelf);
    Vtop_barrel_shifter___024root___trigger_orInto__act(vlSelfRef.__VnbaTriggered, vlSelfRef.__VactTriggered);
    return (0U);
}

void Vtop_barrel_shifter___024root___trigger_clear__act(VlUnpacked<QData/*63:0*/, 1> &out) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop_barrel_shifter___024root___trigger_clear__act\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        out[n] = 0ULL;
        n = ((IData)(1U) + n);
    } while ((1U > n));
}

bool Vtop_barrel_shifter___024root___eval_phase__nba(Vtop_barrel_shifter___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop_barrel_shifter___024root___eval_phase__nba\n"); );
    Vtop_barrel_shifter__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __VnbaExecute;
    // Body
    __VnbaExecute = Vtop_barrel_shifter___024root___trigger_anySet__act(vlSelfRef.__VnbaTriggered);
    if (__VnbaExecute) {
        Vtop_barrel_shifter___024root___eval_nba(vlSelf);
        Vtop_barrel_shifter___024root___trigger_clear__act(vlSelfRef.__VnbaTriggered);
    }
    return (__VnbaExecute);
}

void Vtop_barrel_shifter___024root___eval(Vtop_barrel_shifter___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop_barrel_shifter___024root___eval\n"); );
    Vtop_barrel_shifter__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    IData/*31:0*/ __VnbaIterCount;
    // Body
    __VnbaIterCount = 0U;
    do {
        if (VL_UNLIKELY(((0x00000064U < __VnbaIterCount)))) {
#ifdef VL_DEBUG
            Vtop_barrel_shifter___024root___dump_triggers__act(vlSelfRef.__VnbaTriggered, "nba"s);
#endif
            VL_FATAL_MT("vsrc/barrel_shifter.sv", 24, "", "DIDNOTCONVERGE: NBA region did not converge after 100 tries");
        }
        __VnbaIterCount = ((IData)(1U) + __VnbaIterCount);
        vlSelfRef.__VactIterCount = 0U;
        do {
            if (VL_UNLIKELY(((0x00000064U < vlSelfRef.__VactIterCount)))) {
#ifdef VL_DEBUG
                Vtop_barrel_shifter___024root___dump_triggers__act(vlSelfRef.__VactTriggered, "act"s);
#endif
                VL_FATAL_MT("vsrc/barrel_shifter.sv", 24, "", "DIDNOTCONVERGE: Active region did not converge after 100 tries");
            }
            vlSelfRef.__VactIterCount = ((IData)(1U) 
                                         + vlSelfRef.__VactIterCount);
        } while (Vtop_barrel_shifter___024root___eval_phase__act(vlSelf));
    } while (Vtop_barrel_shifter___024root___eval_phase__nba(vlSelf));
}

#ifdef VL_DEBUG
void Vtop_barrel_shifter___024root___eval_debug_assertions(Vtop_barrel_shifter___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop_barrel_shifter___024root___eval_debug_assertions\n"); );
    Vtop_barrel_shifter__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if (VL_UNLIKELY(((vlSelfRef.clk & 0xfeU)))) {
        Verilated::overWidthError("clk");
    }
    if (VL_UNLIKELY(((vlSelfRef.rst & 0xfeU)))) {
        Verilated::overWidthError("rst");
    }
}
#endif  // VL_DEBUG
