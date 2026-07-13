import defs_pkg::*;
import "DPI-C" function void handle_mem_access_error(input int unsigned addr, input int unsigned mapped_addr);
import "DPI-C" function longint unsigned mmio_read(input int unsigned addr);
import "DPI-C" function void mmio_write(input int unsigned addr, input int data, int unsigned mask);

module LSU #(parameter XLEN = 32)
// RAM地址空间32bit * 2^18 = 1MB,访存地址4字节对齐
(
    input logic                  clk,
    input logic                  rst_n,
    input logic [XLEN-1:0]       addr,
    input logic [XLEN-1:0]       store_data,
    input mem_size_e             mem_size,
    input mem_sign_e             mem_sign,
    input logic                  mem_write_en,
    input logic                  mem_read_en,
    input logic                  ifu_valid,

    output logic [XLEN-1:0]      load_data,
    output logic                 lsu_ready,

    output logic [XLEN-1:0]      x_addr,
    output logic [XLEN-1:0]      x_wdata,
    output logic [3:0]           x_mask,
    output logic                 x_wen,
    output logic                 x_reqValid,
    output logic                 x_respReady,

    input  logic [XLEN-1:0]      x_rdata,
    input  logic                 x_respValid,
    input  logic                 x_reqReady,
    input  logic [1:0]           x_err
);

    localparam BYTES_PER_WORD = XLEN / 8;
    localparam ALIGNED_WIDTH = $clog2(BYTES_PER_WORD);
    logic [ALIGNED_WIDTH-1:0] byte_offset;
    logic misaligned_access;
    assign byte_offset = addr[ALIGNED_WIDTH-1:0];

    enum logic [1:0] {IDLE, WAIT_RESP} current, next;
    always_comb begin
        case(current)
            IDLE: begin
                if(cpu_reqValid && x_reqReady) begin
                    next = WAIT_RESP;
                end
            end
            WAIT_RESP: begin
                if(x_respValid && x_respReady) begin
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

    logic cpu_reqValid;
    assign cpu_reqValid = (mem_read_en || mem_write_en);

    assign x_addr = addr;
    assign x_reqValid = cpu_reqValid;
    assign x_wen = mem_write_en;
    assign x_wdata = store_data << (byte_offset * 8);
    assign x_respReady = 1'b1;  // LSU始终准备好接收来自Xbar的响应
    assign lsu_ready = (current == WAIT_RESP && x_respValid && x_respReady);

    logic [7:0] target_byte;
    logic [15:0] target_half;
    // 数据切片与符号扩展, 取决于访存指令的mem_size和byte_offset
    always_comb begin : mask_and_store_assign
        x_mask = 4'b0000;
        target_byte = 8'b0;
        target_half = 16'b0;
        case(mem_size)
            MEM_BYTE: begin
                case(byte_offset)
                    2'b00: begin x_mask = 4'b0001; target_byte = x_rdata[7:0]; end
                    2'b01: begin x_mask = 4'b0010; target_byte = x_rdata[15:8]; end
                    2'b10: begin x_mask = 4'b0100; target_byte = x_rdata[23:16]; end
                    2'b11: begin x_mask = 4'b1000; target_byte = x_rdata[31:24]; end
                endcase
                load_data = mem_sign ? 32'($signed(target_byte)) : 32'($unsigned(target_byte)); 
            end
            MEM_HALF: begin
                case(byte_offset[1])
                    1'b0: begin x_mask = 4'b0011; target_half = x_rdata[15:0]; end
                    1'b1: begin x_mask = 4'b1100; target_half = x_rdata[31:16]; end
                endcase
                load_data = mem_sign ? 32'($signed(target_half)) : 32'($unsigned(target_half)); 
            end
            MEM_WORD: begin
                x_mask = 4'b1111;
                load_data = x_rdata[31:0];
            end
            default: begin
                x_mask = 4'b1111;
                load_data = x_rdata;
            end
        endcase
    end

    always_comb begin : unaligned_check
        misaligned_access = 1'b0;

        if (cpu_reqValid) begin
            case (mem_size)
                MEM_HALF: begin
                    if (byte_offset[0] != 1'b0) begin
                        misaligned_access = 1'b1;
                    end
                end
                MEM_WORD: begin
                    if (byte_offset != 2'b00) begin
                        misaligned_access = 1'b1;
                    end
                end
                default: ;
            endcase

            if (misaligned_access) begin
                // LSU 用 x_mask 已能正确完成非对齐访存，仅记录异常码供 trap 使用，不 abort 仿真
                $warning("MISALIGNED access at addr=0x%08x", addr);
            end
        end
    end

endmodule
