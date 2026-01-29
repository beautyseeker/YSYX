// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Symbol table internal header
//
// Internal details; most calling programs do not need this header,
// unless using verilator public meta comments.

#ifndef VERILATED_VTOP_BARREL_SHIFTER__SYMS_H_
#define VERILATED_VTOP_BARREL_SHIFTER__SYMS_H_  // guard

#include "verilated.h"

// INCLUDE MODEL CLASS

#include "Vtop_barrel_shifter.h"

// INCLUDE MODULE CLASSES
#include "Vtop_barrel_shifter___024root.h"

// SYMS CLASS (contains all model state)
class alignas(VL_CACHE_LINE_BYTES) Vtop_barrel_shifter__Syms final : public VerilatedSyms {
  public:
    // INTERNAL STATE
    Vtop_barrel_shifter* const __Vm_modelp;
    bool __Vm_activity = false;  ///< Used by trace routines to determine change occurred
    uint32_t __Vm_baseCode = 0;  ///< Used by trace routines when tracing multiple models
    VlDeleter __Vm_deleter;
    bool __Vm_didInit = false;

    // MODULE INSTANCE STATE
    Vtop_barrel_shifter___024root  TOP;

    // CONSTRUCTORS
    Vtop_barrel_shifter__Syms(VerilatedContext* contextp, const char* namep, Vtop_barrel_shifter* modelp);
    ~Vtop_barrel_shifter__Syms();

    // METHODS
    const char* name() const { return TOP.vlNamep; }
};

#endif  // guard
