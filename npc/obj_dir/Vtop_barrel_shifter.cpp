// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Model implementation (design independent parts)

#include "Vtop_barrel_shifter__pch.h"
#include "verilated_vcd_c.h"

//============================================================
// Constructors

Vtop_barrel_shifter::Vtop_barrel_shifter(VerilatedContext* _vcontextp__, const char* _vcname__)
    : VerilatedModel{*_vcontextp__}
    , vlSymsp{new Vtop_barrel_shifter__Syms(contextp(), _vcname__, this)}
    , clk{vlSymsp->TOP.clk}
    , rst{vlSymsp->TOP.rst}
    , din{vlSymsp->TOP.din}
    , dout{vlSymsp->TOP.dout}
    , rootp{&(vlSymsp->TOP)}
{
    // Register model with the context
    contextp()->addModel(this);
    contextp()->traceBaseModelCbAdd(
        [this](VerilatedTraceBaseC* tfp, int levels, int options) { traceBaseModel(tfp, levels, options); });
}

Vtop_barrel_shifter::Vtop_barrel_shifter(const char* _vcname__)
    : Vtop_barrel_shifter(Verilated::threadContextp(), _vcname__)
{
}

//============================================================
// Destructor

Vtop_barrel_shifter::~Vtop_barrel_shifter() {
    delete vlSymsp;
}

//============================================================
// Evaluation function

#ifdef VL_DEBUG
void Vtop_barrel_shifter___024root___eval_debug_assertions(Vtop_barrel_shifter___024root* vlSelf);
#endif  // VL_DEBUG
void Vtop_barrel_shifter___024root___eval_static(Vtop_barrel_shifter___024root* vlSelf);
void Vtop_barrel_shifter___024root___eval_initial(Vtop_barrel_shifter___024root* vlSelf);
void Vtop_barrel_shifter___024root___eval_settle(Vtop_barrel_shifter___024root* vlSelf);
void Vtop_barrel_shifter___024root___eval(Vtop_barrel_shifter___024root* vlSelf);

void Vtop_barrel_shifter::eval_step() {
    VL_DEBUG_IF(VL_DBG_MSGF("+++++TOP Evaluate Vtop_barrel_shifter::eval_step\n"); );
#ifdef VL_DEBUG
    // Debug assertions
    Vtop_barrel_shifter___024root___eval_debug_assertions(&(vlSymsp->TOP));
#endif  // VL_DEBUG
    vlSymsp->__Vm_activity = true;
    vlSymsp->__Vm_deleter.deleteAll();
    if (VL_UNLIKELY(!vlSymsp->__Vm_didInit)) {
        vlSymsp->__Vm_didInit = true;
        VL_DEBUG_IF(VL_DBG_MSGF("+ Initial\n"););
        Vtop_barrel_shifter___024root___eval_static(&(vlSymsp->TOP));
        Vtop_barrel_shifter___024root___eval_initial(&(vlSymsp->TOP));
        Vtop_barrel_shifter___024root___eval_settle(&(vlSymsp->TOP));
    }
    VL_DEBUG_IF(VL_DBG_MSGF("+ Eval\n"););
    Vtop_barrel_shifter___024root___eval(&(vlSymsp->TOP));
    // Evaluate cleanup
    Verilated::endOfEval(vlSymsp->__Vm_evalMsgQp);
}

//============================================================
// Events and timing
bool Vtop_barrel_shifter::eventsPending() { return false; }

uint64_t Vtop_barrel_shifter::nextTimeSlot() {
    VL_FATAL_MT(__FILE__, __LINE__, "", "No delays in the design");
    return 0;
}

//============================================================
// Utilities

const char* Vtop_barrel_shifter::name() const {
    return vlSymsp->name();
}

//============================================================
// Invoke final blocks

void Vtop_barrel_shifter___024root___eval_final(Vtop_barrel_shifter___024root* vlSelf);

VL_ATTR_COLD void Vtop_barrel_shifter::final() {
    Vtop_barrel_shifter___024root___eval_final(&(vlSymsp->TOP));
}

//============================================================
// Implementations of abstract methods from VerilatedModel

const char* Vtop_barrel_shifter::hierName() const { return vlSymsp->name(); }
const char* Vtop_barrel_shifter::modelName() const { return "Vtop_barrel_shifter"; }
unsigned Vtop_barrel_shifter::threads() const { return 1; }
void Vtop_barrel_shifter::prepareClone() const { contextp()->prepareClone(); }
void Vtop_barrel_shifter::atClone() const {
    contextp()->threadPoolpOnClone();
}
std::unique_ptr<VerilatedTraceConfig> Vtop_barrel_shifter::traceConfig() const {
    return std::unique_ptr<VerilatedTraceConfig>{new VerilatedTraceConfig{false, false, false}};
};

//============================================================
// Trace configuration

void Vtop_barrel_shifter___024root__trace_decl_types(VerilatedVcd* tracep);

void Vtop_barrel_shifter___024root__trace_init_top(Vtop_barrel_shifter___024root* vlSelf, VerilatedVcd* tracep);

VL_ATTR_COLD static void trace_init(void* voidSelf, VerilatedVcd* tracep, uint32_t code) {
    // Callback from tracep->open()
    Vtop_barrel_shifter___024root* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<Vtop_barrel_shifter___024root*>(voidSelf);
    Vtop_barrel_shifter__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    if (!vlSymsp->_vm_contextp__->calcUnusedSigs()) {
        VL_FATAL_MT(__FILE__, __LINE__, __FILE__,
            "Turning on wave traces requires Verilated::traceEverOn(true) call before time 0.");
    }
    vlSymsp->__Vm_baseCode = code;
    tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);
    Vtop_barrel_shifter___024root__trace_decl_types(tracep);
    Vtop_barrel_shifter___024root__trace_init_top(vlSelf, tracep);
    tracep->popPrefix();
}

VL_ATTR_COLD void Vtop_barrel_shifter___024root__trace_register(Vtop_barrel_shifter___024root* vlSelf, VerilatedVcd* tracep);

VL_ATTR_COLD void Vtop_barrel_shifter::traceBaseModel(VerilatedTraceBaseC* tfp, int levels, int options) {
    (void)levels; (void)options;
    VerilatedVcdC* const stfp = dynamic_cast<VerilatedVcdC*>(tfp);
    if (VL_UNLIKELY(!stfp)) {
        vl_fatal(__FILE__, __LINE__, __FILE__,"'Vtop_barrel_shifter::trace()' called on non-VerilatedVcdC object;"
            " use --trace-fst with VerilatedFst object, and --trace-vcd with VerilatedVcd object");
    }
    stfp->spTrace()->addModel(this);
    stfp->spTrace()->addInitCb(&trace_init, &(vlSymsp->TOP));
    Vtop_barrel_shifter___024root__trace_register(&(vlSymsp->TOP), stfp->spTrace());
}
