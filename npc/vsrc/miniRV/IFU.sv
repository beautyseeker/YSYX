// `include "defs_pkg.sv"
import defs_pkg::*;

module IFU #(parameter DATA_WIDTH = 32, ADDR_WIDTH = 18, RESET_VEC = 32'h8000_0000)
(
    input logic                  clk,
    input logic                  rst_n,
    input logic [DATA_WIDTH-1:0] PC_next,
    input logic                  idu_ready,

    output logic [DATA_WIDTH-1:0] PC_current,
    output logic [DATA_WIDTH-1:0] instruction,
    output logic                  ifu_valid,
    AXI4_lite.Master              bus
);

    enum logic [1:0] {IDLE, WAIT_RD, HOLD} current, next;

    logic fire;
    assign fire = ifu_valid && idu_ready;

    always_comb begin
        next = current;
        unique case (current)
            IDLE: begin
                if (bus.ARvalid && bus.ARready)
                    next = WAIT_RD;
            end
            WAIT_RD: begin
                // 响应一到就成交，释放共享总线；指令进寄存器
                if (bus.Rvalid && bus.Rready)
                    next = HOLD;
            end
            HOLD: begin
                if (idu_ready)
                    next = IDLE;
            end
            default: next = IDLE;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) current <= IDLE;
        else        current <= next;
    end

    always_ff @(posedge clk) begin
        if (current == WAIT_RD && bus.Rvalid && bus.Rready)
            instruction <= bus.Rdata;
    end

    assign bus.ARaddr      = PC_current;
    assign bus.ARvalid     = current == IDLE;
    assign bus.Rready      = current == WAIT_RD;

    assign bus.AWaddr      = PC_current;
    assign bus.AWvalid     = 1'b0;
    assign bus.Wdata       = '0;
    assign bus.Wmask       = '0;
    assign bus.Wvalid      = 1'b0;
    assign bus.BrespReady  = 1'b1;

    // 只用寄存器态对外 valid，避免 respValid→译码→地址→Xbar 组合环
    assign ifu_valid   = (current == HOLD);

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            PC_current <= RESET_VEC;
        else if (fire)
            PC_current <= PC_next;
    end

endmodule
