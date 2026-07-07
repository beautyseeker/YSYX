// `include "defs_pkg.sv"
import defs_pkg::*;

module EXU #(parameter DATA_WIDTH = 32)
(

    input logic [DATA_WIDTH-1:0] alu_a,
    input logic [DATA_WIDTH-1:0] alu_b,
    input alu_op_t ALU_op,

    output logic alu_zero,
    output logic [DATA_WIDTH-1:0] ALU_result
);

    localparam SHIFT_WIDTH = $clog2(DATA_WIDTH);
    always_comb begin
        case (ALU_op)
            ALU_ADD:    ALU_result = alu_a + alu_b;
            ALU_SUB:    ALU_result = alu_a - alu_b;
            ALU_SLL:    ALU_result = alu_a << alu_b[SHIFT_WIDTH-1:0];
            ALU_SLT:    ALU_result = ($signed(alu_a) < $signed(alu_b)) ? 1 : 0;
            ALU_SLTU:   ALU_result = (alu_a < alu_b) ? 1 : 0;
            ALU_XOR:    ALU_result = alu_a ^ alu_b;
            ALU_SRL:    ALU_result = alu_a >> alu_b[SHIFT_WIDTH-1:0];
            ALU_SRA:    ALU_result = $signed(alu_a) >>> alu_b[SHIFT_WIDTH-1:0];
            ALU_OR:     ALU_result = alu_a | alu_b;
            ALU_AND:    ALU_result = alu_a & alu_b;
            ALU_COPY_B: ALU_result = alu_b;
            default:    ALU_result = 'x;
        endcase
        assert (
            ALU_op inside {
                ALU_ADD, ALU_SUB, ALU_SLL, ALU_SLT, ALU_SLTU,
                ALU_XOR, ALU_SRL, ALU_SRA, ALU_OR, ALU_AND, ALU_COPY_B
            }
        ) else $error("ALU_op invalid: %0d at time %t", ALU_op, $time);
    end
    
    assign alu_zero = (ALU_result == 0);

    // always_comb begin : branch_decision  // 条件分支跳转标志
    //     if(ctrl_sig.jmp_en && ctrl_sig.PC_sel == PC_BRANCH) begin
    //         case (instruction[14:12]) // funct3
    //             3'b000: branch_taken = alu_zero; // BEQ
    //             3'b001: branch_taken = ~alu_zero; // BNE
    //             3'b100: branch_taken = (ALU_result[0]); // BLT
    //             3'b101: branch_taken = (~ALU_result[0]); // BGE
    //             3'b110: branch_taken = (ALU_result[0]); // BLTU
    //             3'b111: branch_taken = (~ALU_result[0]); // BGEU
    //             default: branch_taken = 1'b0;
    //         endcase
    //     end else begin
    //         branch_taken = 1'b0;
    //     end
    // end

    // logic [DATA_WIDTH-1:0] rel_jmp_addr;
    // assign rel_jmp_addr = PC_current + PC_rel_imm;
    // always_comb begin : jump_addr
    //     case(ctrl_sig.PC_sel)
    //         PC_BRANCH: PC_jmp = branch_taken ? rel_jmp_addr : PC_current + 4;
    //         PC_JMP:    PC_jmp = is_jal ? rel_jmp_addr : (ALU_result & ~1);
    //         default:   PC_jmp = PC_current + 4;
    //     endcase
    // end

endmodule
