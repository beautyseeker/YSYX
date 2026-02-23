`include "defs_pkg.sv"
import defs_pkg::*;

module LSU #(parameter DATA_WIDTH = 32, ADDR_WIDTH = 18)
// RAM地址空间32bit * 2^18 = 1MB,访存地址4字节对齐
(
    input logic                  clk,
    input logic                  rst_n,
    input logic [DATA_WIDTH-1:0] addr,
    input logic [DATA_WIDTH-1:0] store_data,
    input mem_size_e             mem_size,
    input mem_sign_e             mem_sign,
    input logic                  mem_write_en,
    input logic                  mem_read_en,

    output logic [DATA_WIDTH-1:0] load_data,
    output exception_t            mem_exception
);
//异步读取数据
    localparam BYTES_PER_WORD = DATA_WIDTH / 8;
    localparam ALIGNED_WIDTH = $clog2(BYTES_PER_WORD);
    logic [DATA_WIDTH-1:0] MEM [2**(ADDR_WIDTH)-1:0];

    logic [ADDR_WIDTH-1:0] word_idx;
    logic [7:0] byte_data;
    logic [15:0] half_data;
    logic [31:0] word_data;
    logic [ALIGNED_WIDTH-1:0] byte_offset;
    logic misaligned_access;
    logic addr_out_of_range;
    assign byte_offset = addr[ALIGNED_WIDTH-1:0];
    assign word_idx = addr[ADDR_WIDTH-1+ALIGNED_WIDTH:ALIGNED_WIDTH]; // 4字节对齐地址

    always_comb begin : aligned_check
        misaligned_access = 1'b0;
        addr_out_of_range = 1'b0;
        if (mem_read_en || mem_write_en) begin
            case (mem_size)
                MEM_BYTE: begin
                    // Byte access is always aligned
                end
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
                default: begin
                    misaligned_access = 1'b0;
                end
            endcase
            if (word_idx >= 2**(ADDR_WIDTH-ALIGNED_WIDTH)) begin
                addr_out_of_range = 1'b1;
            end
        end
    end

    always_comb begin : exception_gen
        mem_exception = EXC_NONE;
        if (misaligned_access) begin
            mem_exception = EXC_ACCESS_MISALIGNED;
        end else if (addr_out_of_range) begin
            mem_exception = EXC_ACCESS_OUT_OF_RANGE;
        end else begin
            mem_exception = EXC_NONE;
        end
    end

    always_comb begin : mem_read
        if (mem_read_en && !misaligned_access && !addr_out_of_range) begin
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
                    word_data = MEM[word_idx];
                    load_data = word_data;
                end
                default: load_data = 'x;
            endcase
        end else begin
            load_data = 'x;
        end
    end

//同步写入数据
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            $readmemh("data_mem_init.hex", MEM); // 从文件初始化内存
        end else if (mem_write_en && !misaligned_access && !addr_out_of_range) begin
            case (mem_size)
                MEM_BYTE: begin
                    MEM[word_idx] <= (MEM[word_idx] & ~(8'hFF << (byte_offset * 8))) | (store_data[7:0] << (byte_offset * 8));
                end
                MEM_HALF: begin
                    MEM[word_idx] <= (MEM[word_idx] & ~(16'hFFFF << (byte_offset * 8))) | (store_data[15:0] << (byte_offset * 8));
                end
                MEM_WORD: begin
                    MEM[word_idx] <= store_data;
                end
                default: MEM[word_idx] <= {DATA_WIDTH{1'bx}};
            endcase
        end
    end

endmodule
