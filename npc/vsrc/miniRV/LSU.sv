// `include "defs_pkg.sv"
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

    localparam BYTES_PER_WORD = DATA_WIDTH / 8;
    localparam ALIGNED_WIDTH = $clog2(BYTES_PER_WORD);
    logic [DATA_WIDTH-1:0] MEM [2**(ADDR_WIDTH)-1:0];

    logic [ADDR_WIDTH-1:0] word_idx;
    logic [DATA_WIDTH/4-1:0] byte_data;
    logic [DATA_WIDTH/2-1:0] half_data;
    logic [DATA_WIDTH-1:0] word_data;
    logic [ALIGNED_WIDTH-1:0] byte_offset;
    logic misaligned_access;
    logic addr_out_of_range;
    assign byte_offset = addr[ALIGNED_WIDTH-1:0];
    assign word_idx = addr[ADDR_WIDTH-1+ALIGNED_WIDTH:ALIGNED_WIDTH]; // 4字节对齐地址

    always_comb begin : access_check
        misaligned_access = 1'b0;
        addr_out_of_range = 1'b0;
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
            if (word_idx >= 2**(ADDR_WIDTH-ALIGNED_WIDTH)) begin
                addr_out_of_range = 1'b1;
                $error("Address out of range at address %h at time %t", addr, $time);
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

    //异步读取数据
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
    // 生成全位宽的字节和半字掩码
    logic [DATA_WIDTH-1:0] byte_mask, half_mask;
    assign byte_mask = ({{(DATA_WIDTH-8){1'b0}}, 8'hFF}) << (byte_offset * 8);
    assign half_mask = ({{(DATA_WIDTH-16){1'b0}}, 16'hFFFF}) << (byte_offset * 8);

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            $readmemh("./resource/addi.hex", MEM); // 从文件初始化内存
        end else if (mem_write_en && !misaligned_access && !addr_out_of_range) begin
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
    end

endmodule
