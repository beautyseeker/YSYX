// `include "defs_pkg.sv"
import defs_pkg::*;

module WBU #(parameter DATA_WIDTH = 32, ADDR_WIDTH = 18)
(
    // 来自EXU的结果
    input logic [DATA_WIDTH-1:0] alu_result,
    input logic [DATA_WIDTH-1:0] mem_load_data,
    input logic [ADDR_WIDTH-1:0] PC_current,
    // 来自IDU的控制信号和目的寄存器地址
    input WB_sel_e WB_sel,

    // 输出到寄存器堆的写回数据和写使能
    output logic [DATA_WIDTH-1:0] WB_data
);

    // 写回数据选择
    always_comb begin
        case (WB_sel)
            ALU_RES: WB_data = alu_result;
            MEM_LOAD: WB_data = mem_load_data;
            PC_INC: WB_data = $signed(PC_current + 4); // 这里假设ALU_result是当前PC，实际设计中可能需要调整
            default: WB_data = 'x; // 不应该发生
        endcase
        assert (WB_sel inside {ALU_RES, MEM_LOAD, PC_INC})
        else $error("Invalid WB_sel: %0d at time %t", WB_sel, $time);
    end
endmodule
