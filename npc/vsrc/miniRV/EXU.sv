`include "defs_pkg.sv"
import defs_pkg::*;

module EXU #(parameter DATA_WIDTH = 32)
(

    input logic [DATA_WIDTH-1:0] alu_a,
    input logic [DATA_WIDTH-1:0] alu_b,
    input alu_op_t ALU_op,

    output logic alu_zero,
    output logic [DATA_WIDTH-1:0] ALU_result
);

    always_comb begin
        case (ALU_op)
            ALU_ADD:    ALU_result = alu_a + alu_b;
            ALU_SUB:    ALU_result = alu_a - alu_b;
            ALU_SLL:    ALU_result = alu_a << alu_b[4:0];
            ALU_SLT:    ALU_result = ($signed(alu_a) < $signed(alu_b)) ? 1 : 0;
            ALU_SLTU:   ALU_result = (alu_a < alu_b) ? 1 : 0;
            ALU_XOR:    ALU_result = alu_a ^ alu_b;
            ALU_SRL:    ALU_result = alu_a >> alu_b[4:0];
            ALU_SRA:    ALU_result = $signed(alu_a) >>> alu_b[4:0];
            ALU_OR:     ALU_result = alu_a | alu_b;
            ALU_AND:    ALU_result = alu_a & alu_b;
            ALU_COPY_B: ALU_result = alu_b;
            default:    ALU_result = 'x;
        endcase
    end
    
    assign alu_zero = (ALU_result == 0);

endmodule
