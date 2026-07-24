// `include "defs_pkg.sv"
import defs_pkg::*;

module WBU #(parameter XLEN = 32, ADDR_WIDTH = 18)
(
    // 来自EXU的结果
    input logic [XLEN-1:0] alu_result,
    input logic [XLEN-1:0] mem_load_data,
    input logic [XLEN-1:0] PC_current,
    input logic [XLEN-1:0] PC_rel_imm,
    input logic [XLEN-1:0] CSR_tvec,
    input logic [XLEN-1:0] CSR_epc,
    input logic [XLEN-1:0] instruction,
    input logic [XLEN-1:0] CSR_data,
    // 来自IDU的控制信号和目的寄存器地址
    input Ctrl_sig_t ctrl_sig,

    // 输出到寄存器堆的写回数据和写使能
    output logic [XLEN-1:0] WB_data,
    output logic [XLEN-1:0] PC_next
);

    logic branch_taken;
    logic [XLEN-1:0] PCInc4, rel_jmp_addr;
    assign PCInc4 = $unsigned(PC_current + 4);
    assign rel_jmp_addr = PC_current + PC_rel_imm;
    // 寄存器堆写回数据MUX
    always_comb begin
        case (ctrl_sig.WB_sel)
            ALU_RES: WB_data = alu_result;
            MEM_LOAD: WB_data = mem_load_data;
            PC_INC: WB_data = PCInc4;
            CSR: WB_data = CSR_data;
            default: WB_data = 'x; // 不应该发生
        endcase
        assert (ctrl_sig.WB_sel inside {ALU_RES, MEM_LOAD, PC_INC, CSR})
        else $error("Invalid WB_sel: %0d at time %t", ctrl_sig.WB_sel, $time);
    end

    always_comb begin : branch_decision  // 条件分支跳转标志
        if(ctrl_sig.jmp_en && ctrl_sig.PC_sel == PC_BRANCH) begin
            case (instruction[14:12]) // funct3
                3'b000: branch_taken = alu_result == 0; // BEQ
                3'b001: branch_taken = alu_result != 0; // BNE
                3'b100: branch_taken = (alu_result[0]); // BLT
                3'b101: branch_taken = (~alu_result[0]); // BGE
                3'b110: branch_taken = (alu_result[0]); // BLTU
                3'b111: branch_taken = (~alu_result[0]); // BGEU
                default: branch_taken = 1'b0;
            endcase
        end else begin
            branch_taken = 1'b0;
        end
    end

    always_comb begin : trap_addr
        if(ctrl_sig.jmp_en)
            case(ctrl_sig.PC_sel)
                PC_PLUS4:    PC_next = PCInc4;
                PC_BRANCH:   PC_next = branch_taken ? rel_jmp_addr : PCInc4;
                PC_JMP:      PC_next = instruction[3] ? rel_jmp_addr : (alu_result & ~1);
                PC_TRAP_ENT: PC_next = CSR_tvec;
                PC_TRAP_RET: PC_next = CSR_epc;
                default:     PC_next = PCInc4;
            endcase
        else
            PC_next = PCInc4;
    end

    // logic condition, non_condition;
    // assign condition = branch_taken && ctrl_sig.PC_sel == PC_BRANCH;
    // assign non_condition = ctrl_sig.PC_sel == PC_JMP;
    // assign o_flush = ctrl_sig.jmp_en && (condition || non_condition);

endmodule
