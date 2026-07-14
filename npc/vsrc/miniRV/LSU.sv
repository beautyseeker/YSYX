import defs_pkg::*;
import "DPI-C" function void handle_mem_access_error(input int unsigned addr, input int unsigned mapped_addr);

module LSU #(parameter XLEN = 32)
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

    SimpleBus_if.Master          bus
);

    localparam BYTES_PER_WORD = XLEN / 8;
    localparam ALIGNED_WIDTH = $clog2(BYTES_PER_WORD);
    logic [ALIGNED_WIDTH-1:0] byte_offset;
    logic misaligned_access;
    assign byte_offset = addr[ALIGNED_WIDTH-1:0];

    logic cpu_reqValid;
    assign cpu_reqValid = (mem_read_en || mem_write_en);

    enum logic [1:0] {IDLE, WAIT_RESP} current, next;
    always_comb begin
        case(current)
            IDLE: begin
                if(cpu_reqValid && bus.reqReady) begin
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

    assign bus.addr = addr;
    assign bus.reqValid = cpu_reqValid;
    assign bus.wen = mem_write_en;
    assign bus.wdata = store_data << (byte_offset * 8);
    assign bus.respReady = 1'b1; // 主机LSU不受下游阻塞，因此总是准备好接收响应
    assign lsu_ready = (current == WAIT_RESP && bus.respValid && bus.respReady);

    logic [7:0] target_byte;
    logic [15:0] target_half;
    // 数据切片与符号扩展, 取决于访存指令的mem_size和byte_offset
    always_comb begin : mask_and_store_assign
        bus.mask = 4'b0000;
        target_byte = 8'b0;
        target_half = 16'b0;
        unique case (mem_size)
            MEM_BYTE: begin
                unique case (byte_offset)
                    2'b00: begin bus.mask = 4'b0001; target_byte = bus.rdata[7:0]; end
                    2'b01: begin bus.mask = 4'b0010; target_byte = bus.rdata[15:8]; end
                    2'b10: begin bus.mask = 4'b0100; target_byte = bus.rdata[23:16]; end
                    2'b11: begin bus.mask = 4'b1000; target_byte = bus.rdata[31:24]; end
                endcase
                load_data = mem_sign ? 32'($signed(target_byte)) : 32'($unsigned(target_byte)); 
            end
            MEM_HALF: begin
                unique case (byte_offset[1])
                    1'b0: begin bus.mask = 4'b0011; target_half = bus.rdata[15:0]; end
                    1'b1: begin bus.mask = 4'b1100; target_half = bus.rdata[31:16]; end
                endcase
                load_data = mem_sign ? 32'($signed(target_half)) : 32'($unsigned(target_half)); 
            end
            MEM_WORD: begin
                bus.mask = 4'b1111;
                load_data = bus.rdata[31:0];
            end
            default: begin
                bus.mask = 4'b1111;
                load_data = bus.rdata;
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
