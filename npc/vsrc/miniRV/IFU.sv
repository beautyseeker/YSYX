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

    enum logic [1:0] {IDLE, WAIT_RESP} current, next;
    always_comb begin
        case(current)
            IDLE: begin
                if(bus.reqReady && bus.reqValid) begin
                    next = WAIT_RESP;
                end
            end
            WAIT_RESP: begin
                if(bus.respValid && bus.respReady) begin
                    next = IDLE;
                end
            end
            default: begin
                next = IDLE;
            end
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if(!rst_n) begin
            current <= IDLE;
        end
        else begin
            current <= next;
        end
    end

    assign bus.addr = PC_current;
    assign bus.wen = 1'b0;
    assign bus.wdata = '0;
    assign bus.mask = '0;
    assign bus.reqValid = current == IDLE;
    assign bus.respReady = idu_ready;
    
    assign instruction = bus.rdata;


    logic fire;
    assign ifu_valid = (current == WAIT_RESP) && bus.respValid;
    assign fire = ifu_valid && idu_ready;

    always_ff @(posedge clk, negedge rst_n) begin : IF_pipeline
        if(!rst_n) begin
            PC_current <= RESET_VEC;
        end else if (fire) begin
            PC_current <= PC_next;
        end
    end

endmodule
