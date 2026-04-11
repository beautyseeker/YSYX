// `include "defs_pkg.sv"
import defs_pkg::*;
import "DPI-C" function void handle_mem_access_error(input int unsigned addr, input int unsigned mapped_addr);
import "DPI-C" function longint unsigned mmio_read(input int unsigned addr);
import "DPI-C" function void mmio_write(input int unsigned addr, input int data, input byte wmask);
import "DPI-C" function void register_pmem_args(input int unsigned mem_head[], input int unsigned mem_size, input int unsigned mem_base);

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

    output logic [DATA_WIDTH-1:0] load_data,
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
    logic [DATA_WIDTH-1:0] MEM [0:PMEM_SIZE-1] /* verilator public*/ ; 

    logic [ADDR_WIDTH-1:0] word_idx;
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
    assign word_idx = mapped_addr[ADDR_WIDTH-1+ALIGNED_WIDTH:ALIGNED_WIDTH]; // 4字节对齐地址

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
                        $error("Misaligned half-word access at address %h at time %t", addr, $time);
                    end
                end
                MEM_WORD: begin
                    if (byte_offset != 2'b00) begin
                        misaligned_access = 1'b1;
                        $error("Misaligned word access at address %h at time %t", addr, $time);
                    end
                end
                default: begin
                    misaligned_access = 1'b0;
                end
            endcase
            if (!addr_in_mem && !addr_in_IO) begin
                $warning("Address: %h out of range[%h, %h]  address mapped address: %h at time %t",
                 addr, PMEM_BASE, PMEM_BASE + PMEM_SIZE - 1, mapped_addr, $time);
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

    logic [63:0] uptime;
    //异步读取数据
    always_comb begin : mem_read
        if (mem_read_en) begin
            if(!misaligned_access && addr_in_mem) begin
                case (mem_size)
                    MEM_BYTE: begin
                        byte_data = MEM[word_idx][(byte_offset * 8) +: 8];
                        load_data = mem_sign ? {{24{byte_data[7]}}, byte_data} : {24'b0, byte_data};
                    end
                    MEM_HALF: begin
                        half_data = MEM[word_idx][(byte_offset * 8) +: 16];
                        load_data = mem_sign ? {{16{half_data[15]}}, half_data} : {16'b0, half_data};
                    end
                    MEM_WORD: begin
                        word_data = MEM[word_idx][byte_offset * 8 +: 32];
                        load_data = word_data;
                    end
                    default: load_data = 'x;
                endcase
            end
            if(addr == RTC_ADDR || addr == RTC_ADDR + BYTES_PER_WORD) begin
                uptime = mmio_read(addr);
                // $display("SV MMIO Read from RTC:addr=0x%h data=0x%h", addr, uptime);
                case (mem_size)
                    MEM_WORD: load_data = (addr == RTC_ADDR) ? uptime[31:0] : uptime[63:32];
                    default: load_data = 'x;
                endcase
            end
        end

        else begin
            load_data = 'x;
        end
    end

    //同步写入数据
    // 根据最低位byte_offset和mem_size计算写掩码，确保只修改目标字节/半字/字
    logic [DATA_WIDTH-1:0] byte_mask, half_mask;
    assign byte_mask = ({{(DATA_WIDTH-8){1'b0}}, 8'hFF}) << (byte_offset * 8);
    assign half_mask = ({{(DATA_WIDTH-16){1'b0}}, 16'hFFFF}) << (byte_offset * 8);

    initial begin
        static string path = get_img_path();
        if (path == "") begin
            path = RAM_FILE_DEFAULT;
        end
        $display("RAM initialized from: %s", path);
        $readmemh(path, MEM);
        // 硬件启动瞬间，把 MEM 的首地址发给 C++
        register_pmem_args(MEM, PMEM_SIZE, PMEM_BASE);
    end

    always_ff @(posedge clk or negedge rst_n) begin : mem_write
        if (!rst_n) begin
            // 复位时清空内存（可选，根据需求决定是否需要）
        end 
        else if (mem_write_en) begin
            if(!misaligned_access && addr_in_mem) begin
                case (mem_size)
                    MEM_BYTE: begin
                        MEM[word_idx] <= (MEM[word_idx] & ~byte_mask)
                                    | (({{(DATA_WIDTH-8){1'b0}}, store_data[7:0]} << (byte_offset * 8)) & byte_mask);
                    end
                    MEM_HALF: begin
                        MEM[word_idx] <= (MEM[word_idx] & ~half_mask)
                                    | (({{(DATA_WIDTH-16){1'b0}}, store_data[15:0]} << (byte_offset * 8)) & half_mask);
                    end
                    MEM_WORD: begin
                        MEM[word_idx] <= store_data;
                    end
                    default: MEM[word_idx] <= {DATA_WIDTH{1'bx}};
                endcase
            end
            if (addr == SERIAL_ADDR) begin
                // $display("SV MMIO Write to SERIAL: data=%h, byte_mask=%h", store_data, byte_mask);
                mmio_write(addr, store_data, byte_mask[7:0]);
        end
    end
end

endmodule
