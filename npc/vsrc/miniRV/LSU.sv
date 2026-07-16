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

    AXI4_lite.Master          bus
);

    localparam BYTES_PER_WORD = XLEN / 8;
    localparam ALIGNED_WIDTH = $clog2(BYTES_PER_WORD);
    logic [ALIGNED_WIDTH-1:0] byte_offset;
    logic misaligned_access;
    assign byte_offset = addr[ALIGNED_WIDTH-1:0];

    enum logic [1:0] {RD_IDLE, RD_WAIT_RESP} rd_cur, rd_next;
    always_comb begin
        case(rd_cur)
            RD_IDLE: begin
                if(bus.ARvalid && bus.ARready) begin
                    rd_next = RD_WAIT_RESP;
                end
            end
            RD_WAIT_RESP: begin
                if(bus.Rvalid && bus.Rready) begin
                    rd_next = RD_IDLE;
                end
            end
            default: begin
                rd_next = RD_IDLE;
            end
        endcase
    end

    enum logic [1:0] {WR_IDLE, WR_WAIT_RESP} wr_cur, wr_next;
    always_comb begin
        case(wr_cur)
            WR_IDLE: begin
                if((bus.AWvalid && bus.AWready) | (bus.Wvalid && bus.Wready)) begin
                    wr_next = WR_WAIT_RESP;
                end
            end
            WR_WAIT_RESP: begin
                if(bus.BrespValid && bus.BrespReady) begin
                    wr_next = WR_IDLE;
                end
            end
            default: begin
                wr_next = WR_IDLE;
            end
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if(!rst_n) begin
            rd_cur <= RD_IDLE;
        end
        else begin
            rd_cur <= rd_next;
            wr_cur <= wr_next;
        end
    end

    assign bus.ARaddr = addr;
    assign bus.ARvalid = mem_read_en;
    assign bus.Rready = rd_cur == RD_WAIT_RESP;
    assign bus.AWaddr = addr;
    assign bus.AWvalid = mem_write_en;
    assign bus.Wdata = store_data << (byte_offset * 8);
    assign bus.Wvalid = mem_write_en;
    assign bus.BrespReady = wr_cur == WR_WAIT_RESP;
    assign lsu_ready = 
    (wr_cur == WR_WAIT_RESP && bus.BrespValid && bus.BrespReady)
    |(rd_cur == RD_WAIT_RESP && bus.Rvalid && bus.Rready);

    logic [7:0] target_byte;
    logic [15:0] target_half;
    // 数据切片与符号扩展, 取决于访存指令的mem_size和byte_offset
    always_comb begin : mask_and_store_assign
        bus.Wmask = 4'b0000;
        target_byte = 8'b0;
        target_half = 16'b0;
        unique case (mem_size)
            MEM_BYTE: begin
                unique case (byte_offset)
                    2'b00: begin bus.Wmask = 4'b0001; target_byte = bus.Rdata[7:0]; end
                    2'b01: begin bus.Wmask = 4'b0010; target_byte = bus.Rdata[15:8]; end
                    2'b10: begin bus.Wmask = 4'b0100; target_byte = bus.Rdata[23:16]; end
                    2'b11: begin bus.Wmask = 4'b1000; target_byte = bus.Rdata[31:24]; end
                endcase
                load_data = mem_sign ? 32'($signed(target_byte)) : 32'($unsigned(target_byte)); 
            end
            MEM_HALF: begin
                unique case (byte_offset[1])
                    1'b0: begin bus.Wmask = 4'b0011; target_half = bus.Rdata[15:0]; end
                    1'b1: begin bus.Wmask = 4'b1100; target_half = bus.Rdata[31:16]; end
                endcase
                load_data = mem_sign ? 32'($signed(target_half)) : 32'($unsigned(target_half)); 
            end
            MEM_WORD: begin
                bus.Wmask = 4'b1111;
                load_data = bus.Rdata[31:0];
            end
            default: begin
                bus.Wmask = 4'b1111;
                load_data = bus.Rdata;
            end
        endcase
    end

    always_comb begin : unaligned_check
        misaligned_access = 1'b0;

        if (mem_read_en || mem_write_en) begin
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
