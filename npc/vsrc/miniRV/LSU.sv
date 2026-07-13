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
    localparam PMEM_SIZE = 1 << ADDR_WIDTH;
    localparam SERIAL_ADDR = 32'h1000_0000; // 串口MMIO地址
    localparam RTC_ADDR = 32'h1000_0048; // 定时器MMIO地址

    logic [DATA_WIDTH-1:0] mapped_addr;
    assign mapped_addr = addr - PMEM_BASE; // 将访问地址映射到内存地址空间

    logic [ADDR_WIDTH-1-ALIGNED_WIDTH:0] word_idx;
    logic [ALIGNED_WIDTH-1:0] byte_offset;
    logic misaligned_access;

    logic addr_in_mem;
    logic addr_in_IO;
    assign addr_in_mem = (addr >= PMEM_BASE) && (addr < (PMEM_BASE + PMEM_SIZE));
    assign addr_in_IO = addr inside {SERIAL_ADDR, RTC_ADDR, RTC_ADDR+BYTES_PER_WORD};
    assign byte_offset = mapped_addr[ALIGNED_WIDTH-1:0];
    assign word_idx = mapped_addr[ADDR_WIDTH-1:ALIGNED_WIDTH]; // 4字节对齐地址

    logic mem_access_valid;
    assign mem_access_valid = (mem_read_en || mem_write_en);

    always_comb begin : access_check
        misaligned_access = 1'b0;
        mem_exception = EXC_NONE;

        if (mem_access_valid) begin
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
                // LSU 用 byte_mask 已能正确完成非对齐访存，仅记录异常码供 trap 使用，不 abort 仿真
                $warning("MISALIGNED access at addr=0x%08x mapped=0x%08x", addr, mapped_addr);
                mem_exception = EXC_ACCESS_MISALIGNED;
                handle_mem_access_error(addr, mapped_addr);
            end else if (!addr_in_mem && !addr_in_IO) begin
                $warning("OUT OF MEM at addr=0x%08x mapped=0x%08x", addr, mapped_addr);
                mem_exception = EXC_ACCESS_OUT_OF_RANGE;
                handle_mem_access_error(addr, mapped_addr);
            end
        end
    end

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

    logic reqValid;
    assign reqValid = (mem_read_en || mem_write_en);
    assign lsu_ready = reqValid && respValid;

    logic [DATA_WIDTH-1:0] MEM [0:1<<(ADDR_WIDTH-ALIGNED_WIDTH)-1];
    logic [DATA_WIDTH-1:0] full_mask;
    assign full_mask = {
        {8{byte_mask[3]}}, 
        {8{byte_mask[2]}}, 
        {8{byte_mask[1]}}, 
        {8{byte_mask[0]}}
    };

    initial begin
        string path = get_img_path();
        $display("RAM initialized from: %s", path);
        $readmemh(path, MEM, 0);
    end

    logic [3:0] cnt;
    localparam LATENCY = 4'd2;
    enum logic [1:0] {IDLE, BUSY, RESP} current, next;

    always_ff @(posedge clk or negedge rst_n) begin
        if(!rst_n) begin
            cnt <= 0;
        end
        else begin
            cnt <= (current == BUSY) ? cnt + 1 : 0;
        end
    end

    always_comb begin
        case(current)
            IDLE: begin
                if(reqValid) begin
                    next = BUSY;
                end
            end
            BUSY: begin
                if(cnt == LATENCY-1) begin
                    next = RESP;
                end
            end
            RESP: begin
                if(respReady)
                    next = IDLE;
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

    logic respValid, reqReady, respReady;
    assign respValid = (current == RESP);
    assign reqReady = (current == IDLE);
    assign respReady = 1;

    logic [DATA_WIDTH-1:0] wdata;
    logic [63:0] rtc64;
    assign wdata = store_data << (byte_offset * 8);

    always_ff @(posedge clk) begin
        if(current == IDLE && reqValid) begin
            if(mem_write_en) begin
                if(addr_in_mem)
                    MEM[word_idx] <= (MEM[word_idx] & ~full_mask) | (wdata & full_mask);
                else if(addr_in_IO)
                    mmio_write(addr, wdata, 8'hff);
                else
                    $error("RAM write: addr=0x%h, wdata=0x%h, mask=0x%h", addr, wdata, byte_mask);
                    
            end
            else begin
                if(addr_in_mem)
                    rdata <= MEM[word_idx];
                else if(addr_in_IO)
                    case(addr)
                        RTC_ADDR: begin
                            rtc64 = mmio_read(RTC_ADDR);
                            rdata <= rtc64[31:0];
                        end
                        RTC_ADDR+BYTES_PER_WORD: begin
                            rdata <= rtc64[63:32];
                        end
                        default: $error("RAM read: addr=0x%h, rdata=0x%h", addr, rdata);
                    endcase
                    
                else
                    $error("RAM read: addr=0x%h, rdata=0x%h", addr, rdata);
            end
        end
    end

endmodule
