// `include "defs_pkg.sv"
import defs_pkg::*;

module IFU #(parameter DATA_WIDTH = 32, ADDR_WIDTH = 18)
(
    input logic                  clk,
    input logic                  rst_n,
    input logic [ADDR_WIDTH-1:0] PC_next,

    output logic [ADDR_WIDTH-1:0] PC_current,
    output logic [DATA_WIDTH-1:0] instruction,
    output exception_t exception
);
    // PC寄存器
    always_ff @( posedge clk, negedge rst_n ) begin : PC_reg
        if(!rst_n) begin
            PC_current <= 0;
        end else begin
            PC_current <= PC_next;
        end
    end

    localparam BYTES_PER_WORD = DATA_WIDTH / 8;
    localparam ALIGNED_WIDTH = $clog2(BYTES_PER_WORD);
    localparam ROM_DEPTH = 1 << (ADDR_WIDTH - ALIGNED_WIDTH); 
    logic [DATA_WIDTH-1:0] ROM [0:ROM_DEPTH-1];
    initial begin
        static string path = get_init_file("rom_file", ROM_FILE_DEFAULT);
        $display("ROM initialized from: %s", path);
        $readmemh(path, ROM);
    end

    logic fetch_exception;
    logic [ADDR_WIDTH-ALIGNED_WIDTH-1:0] word_idx;

    always_comb begin : fetch
        fetch_exception = (PC_current[ALIGNED_WIDTH-1:0] != 0); // 检查是否4字节对齐
        if(fetch_exception) begin
            word_idx = 0;
            instruction = 32'h00000013; // NOP指令
            exception = EXC_INST_MISALIGNED;
        end else begin
            word_idx = PC_current[ADDR_WIDTH-1:ALIGNED_WIDTH]; // 4字节对齐地址
            instruction = ROM[word_idx];
            exception = EXC_NONE;
        end
    end

// 确保 PC 永远是 4 字节对齐的（除非你有异常处理）
property p_pc_aligned;
    @(posedge clk) (rst_n) |-> (PC_current[ALIGNED_WIDTH-1:0] == 0);
endproperty
assert property (p_pc_aligned) else $error("PC Misaligned at time %t", $time);

endmodule
