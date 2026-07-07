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
    output logic                  ifu_valid
);

    localparam BYTES_PER_WORD = DATA_WIDTH / 8;
    localparam ALIGNED_WIDTH = $clog2(BYTES_PER_WORD);
    localparam ROM_DEPTH = 1 << (ADDR_WIDTH - ALIGNED_WIDTH);

    logic [ADDR_WIDTH-ALIGNED_WIDTH-1:0] word_idx;
    logic [DATA_WIDTH-1:0] mapped_addr;
    assign word_idx = mapped_addr[ADDR_WIDTH-1:ALIGNED_WIDTH];

    assign mapped_addr = PC_current - RESET_VEC; // 取指请求地址映射到ROM

    logic [3:0] LFSR;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            LFSR <= 4'b1; // 非零初始状态
        end else begin
            LFSR <= {LFSR[2:0], LFSR[3] ^ LFSR[2]};
        end
    end

    enum logic [1:0] {IDLE, WAIT} current, next;
    logic instValid;
    assign instValid = (LFSR > 4'b0010);

    always_comb begin : state_logic
        next = current;
        case(current)
            IDLE: begin  // 取指中
                if(instValid) 
                    next = WAIT;
            end
            WAIT: begin  // 取指成功待响应
                if(idu_ready) begin
                    next = IDLE;
                end
            end
            default: begin
                next = IDLE;
            end
        endcase
    end

    always_ff @(posedge clk, negedge rst_n) begin : state_ff
        if(!rst_n)
            current <= IDLE;
        else
            current <= next;
    end

    logic fire;
    assign ifu_valid = (current == WAIT); // 指令已取出，并等待下游处理，就始终拉高指令合法信号
    assign fire = ifu_valid && idu_ready; // 只有在指令合法且下游准备好时，才认为指令retire

    always_ff @(posedge clk, negedge rst_n) begin : IF_pipeline
        if(!rst_n) begin
            PC_current <= RESET_VEC;
        end else if (fire) begin
            PC_current <= PC_next;
        end
    end

    logic [DATA_WIDTH-1:0] ROM [0:ROM_DEPTH-1];
    initial begin
        string path = get_img_path();
        $display("ROM initialized from: %s", path);
        $readmemh(path, ROM, 0);
    end
    always_ff @(posedge clk, negedge rst_n) begin : IF_inst_fetch
        if(!rst_n)
            instruction <= 32'h0000_0013;
        else begin
            if(instValid)
                instruction <= ROM[word_idx];
        end
    end

    property p_pc_aligned;
        @(posedge clk) (rst_n && ifu_valid) |-> (PC_current[ALIGNED_WIDTH-1:0] == 0);
    endproperty
    assert property (p_pc_aligned) else $error("PC Misaligned at time %t", $time);

endmodule
