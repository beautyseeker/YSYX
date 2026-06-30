// `include "defs_pkg.sv"
import defs_pkg::*;
import "DPI-C" function void handle_mem_access_error(input int unsigned addr, input int unsigned mapped_addr);
import "DPI-C" function longint unsigned mmio_read(input int unsigned addr);
import "DPI-C" function void mmio_write(input int unsigned addr, input int data, input byte mask);

module LSU #(parameter DATA_WIDTH = 32, ADDR_WIDTH = 27, PMEM_BASE = 32'h8000_0000)
// RAM地址空间32bit * 2^18 = 1MB,访存地址4字节对齐
(
    input logic                  clk,
    input logic                  rst_n,
    input logic [DATA_WIDTH-1:0] addr/* verilator public */,
    input logic [DATA_WIDTH-1:0] store_data,
    input mem_size_e             mem_size,
    input mem_sign_e             mem_sign,
    input logic                  mem_write_en,
    input logic                  mem_read_en,
    input logic                  ifu_valid,

    output logic [DATA_WIDTH-1:0] load_data,
    output logic                 lsu_ready,
    output except_cause            mem_exception
);

    localparam BYTES_PER_WORD = DATA_WIDTH / 8;
    localparam ALIGNED_WIDTH = $clog2(BYTES_PER_WORD);
    localparam PMEM_SIZE /*verilator public*/ = 1 << ADDR_WIDTH;
    localparam CONFIG_BASE /*verilator public*/ = PMEM_BASE;
    localparam SERIAL_ADDR = 32'h1000_0000; // 串口MMIO地址
    localparam RTC_ADDR = 32'h1000_0048; // 定时器MMIO地址

    logic [DATA_WIDTH-1:0] mapped_addr;
    assign mapped_addr = addr - PMEM_BASE; // 将访问地址映射到内存地址空间

    logic [ADDR_WIDTH-1-ALIGNED_WIDTH:0] word_idx;
    logic [7:0] byte_data;
    logic [15:0] half_data;
    logic [31:0] word_data;
    logic [ALIGNED_WIDTH-1:0] byte_offset;
    logic misaligned_access;

    logic addr_in_mem;
    logic addr_in_IO;
    assign addr_in_mem = (addr >= PMEM_BASE) && (addr < PMEM_BASE + PMEM_SIZE);
    assign addr_in_IO = addr inside {SERIAL_ADDR, RTC_ADDR, RTC_ADDR+BYTES_PER_WORD};
    assign byte_offset = mapped_addr[ALIGNED_WIDTH-1:0];
    assign word_idx = mapped_addr[ADDR_WIDTH-1:ALIGNED_WIDTH]; // 4字节对齐地址

    always_comb begin : access_check
        misaligned_access = 1'b0;
        if (mem_read_en || mem_write_en) begin
            assert(mem_size inside {MEM_BYTE, MEM_HALF, MEM_WORD})
            else $error("Invalid mem_size: %0d at time %t", mem_size, $time);
            assert(mem_sign inside {MEM_SIGNED, MEM_UNSIGNED})
            else $error("Invalid mem_sign: %0d at time %t", mem_sign, $time);
            case (mem_size)
                MEM_BYTE: begin
                    // Byte access is always aligned
                end
                MEM_HALF: begin
                    if (byte_offset[0] != 1'b0) begin
                        misaligned_access = 1'b1;
                        $warning("Misaligned half-word access");
                        handle_mem_access_error(addr, mapped_addr);
                    end
                end
                MEM_WORD: begin
                    if (byte_offset != 2'b00) begin
                        misaligned_access = 1'b1;
                        $warning("Misaligned word access");
                        handle_mem_access_error(addr, mapped_addr);
                    end
                end
                default: begin
                    misaligned_access = 1'b0;
                end
            endcase
            if (!addr_in_mem && !addr_in_IO) begin
                $warning("OUT OF MEM");
                handle_mem_access_error(addr, mapped_addr);
            end
        end
    end

    always_comb begin : exception_gen
        mem_exception = EXC_NONE;
        if (misaligned_access) begin
            mem_exception = EXC_ACCESS_MISALIGNED;
        end else if (!addr_in_mem && !addr_in_IO) begin
            mem_exception = EXC_ACCESS_OUT_OF_RANGE;
        end else begin
            mem_exception = EXC_NONE;
        end
    end


logic respValid;
logic [DATA_WIDTH-1:0] rdata;
logic [3:0] byte_mask;
logic [7:0] target_byte;
logic [15:0] target_half;

always_comb begin : mask_and_store_gen
    byte_mask = 4'b0000;
    target_byte = 8'b0;
    target_half = 16'b0;
    case(mem_size)
        MEM_BYTE: begin
            case(byte_offset)
                2'b00: begin byte_mask = 4'b0001; target_byte = rdata[7:0]; end
                2'b01: begin byte_mask = 4'b0010; target_byte = rdata[15:8]; end
                2'b10: begin byte_mask = 4'b0100; target_byte = rdata[23:16]; end
                2'b11: begin byte_mask = 4'b1000; target_byte = rdata[31:24]; end
            endcase
            load_data = mem_sign ? 32'($signed(target_byte)) : 32'($unsigned(target_byte)); 
        end
        MEM_HALF: begin
            case(byte_offset[1])
                1'b0: begin byte_mask = 4'b0011; target_half = rdata[15:0]; end
                1'b1: begin byte_mask = 4'b1100; target_half = rdata[31:16]; end
            endcase
            load_data = mem_sign ? 32'($signed(target_half)) : 32'($unsigned(target_half)); 
        end
        MEM_WORD: begin
            byte_mask = 4'b1111;
            load_data = rdata[31:0];
        end
        default: begin
            byte_mask = 4'b1111;
            load_data = rdata;
        end
    endcase
end

    // 连接到 RAM 例化口
    RAM #(.DATA_WIDTH(DATA_WIDTH), .SIZE(1<<(ADDR_WIDTH-ALIGNED_WIDTH))) ram (
        .clk(clk),
        .rst_n(rst_n),
        .addr(word_idx),
        .reqValid((mem_read_en || mem_write_en)),
        .wen(mem_write_en),
        .wdata(store_data),
        .mask(byte_mask),
        .rdata(rdata),
        .respValid(respValid)
    );
    assign lsu_ready = respValid;

    // logic [63:0] uptime;
    //异步读取数据
    // always_comb begin : mem_read
    //     if(addr == RTC_ADDR || addr == RTC_ADDR + BYTES_PER_WORD) begin
    //         uptime = mmio_read(addr);
    //         case (mem_size)
    //             MEM_WORD: load_data = (addr == RTC_ADDR) ? uptime[31:0] : uptime[63:32];
    //             default: load_data = 'x;
    //         endcase
    //     end
    // end

    always_ff @(posedge clk) begin : mem_write
        if (mem_write_en) begin
            if (addr == SERIAL_ADDR) begin
                mmio_write(addr, store_data, 8'hff);
        end
    end
end

endmodule
