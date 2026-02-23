`include "defs_pkg.sv"
import defs_pkg::*;

module WBU #(parameter DATA_WIDTH = 32)
(
    // 来自EXU的结果
    input logic [DATA_WIDTH-1:0] alu_result,
    input logic [DATA_WIDTH-1:0] mem_load_data,
    // 来自IDU的控制信号和目的寄存器地址
    input Ctrl_sig_t ctrl_sig,

    // 输出到寄存器堆的写回数据和写使能
    output logic [DATA_WIDTH-1:0] rd_data_out,
);

    // 写回数据选择
    always_comb begin
        if (ctrl_sig.mem_to_reg) begin
            rd_data_out = mem_load_data; // 来自内存的数据
        end else begin
            rd_data_out = alu_result; // 来自ALU的结果
        end
    end
endmodule
