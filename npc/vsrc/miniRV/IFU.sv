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
    SimpleBus_if.Master           bus
);

    enum logic [1:0] {IDLE, WAIT_RESP, HOLD} current, next;

    logic fire;
    assign fire = ifu_valid && idu_ready;

    always_comb begin
        next = current;
        unique case (current)
            IDLE: begin
                if (bus.reqValid && bus.reqReady)
                    next = WAIT_RESP;
            end
            WAIT_RESP: begin
                // 响应一到就成交，释放共享总线；指令进寄存器
                if (bus.respValid && bus.respReady)
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
        if (current == WAIT_RESP && bus.respValid && bus.respReady)
            instruction <= bus.rdata;
    end

    assign bus.addr      = PC_current;
    assign bus.wen       = 1'b0;
    assign bus.wdata     = '0;
    assign bus.mask      = '0;
    assign bus.reqValid  = (current == IDLE);
    assign bus.respReady = (current == WAIT_RESP);

    // 只用寄存器态对外 valid，避免 respValid→译码→地址→Xbar 组合环
    assign ifu_valid   = (current == HOLD);

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            PC_current <= RESET_VEC;
        else if (fire)
            PC_current <= PC_next;
    end

endmodule
