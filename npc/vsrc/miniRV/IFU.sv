// `include "defs_pkg.sv"
import defs_pkg::*;

module IFU #(parameter DATA_WIDTH = 32, ADDR_WIDTH = 18, RESET_VEC = 32'h8000_0000)
(
    input logic                  clk,
    input logic                  rst_n,
    input Ctrl_sig_t             ctrl_sig,
    input logic [DATA_WIDTH-1:0] ALU_result,
    input logic                  ALU_zero,
    input logic [DATA_WIDTH-1:0] PC_rel_imm,
    input logic [DATA_WIDTH-1:0] CSR_tvec,
    input logic [DATA_WIDTH-1:0] CSR_epc,
    input logic                  idu_ready,

    output logic [DATA_WIDTH-1:0] PC_current,
    output logic [DATA_WIDTH-1:0] PC_next,
    output logic [DATA_WIDTH-1:0] instruction,
    output logic                  ifu_valid,
    output except_cause            IF_exception
);

    logic is_jal;
    logic [DATA_WIDTH-1:0] PCInc4;
    assign PCInc4 = PC_current + 4;
    assign is_jal = instruction[3];
    logic branch_taken;

    typedef enum logic [1:0] {
        IDLE,
        WAIT
    } IFU_state_t;

    IFU_state_t current, next;

    always_comb begin
        case (current)
            IDLE: next = WAIT;
            WAIT: next = (idu_ready) ? IDLE : WAIT;
            default: next = IDLE;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            current <= IDLE;
        end else begin
            current <= next;
        end
    end

    assign ifu_valid = (current == WAIT);

    always_comb begin
        if(ctrl_sig.jmp_en && ctrl_sig.PC_sel == PC_BRANCH) begin
            case (instruction[14:12]) // funct3
                3'b000: branch_taken = ALU_zero; // BEQ
                3'b001: branch_taken = ~ALU_zero; // BNE
                3'b100: branch_taken = (ALU_result[0]); // BLT:  SLT==1 → src1 < src2 (signed)
                3'b101: branch_taken = (~ALU_result[0]); // BGE:  SLT==0 → src1 >= src2 (signed)
                3'b110: branch_taken = (ALU_result[0]); // BLTU: SLTU==1 → src1 < src2 (unsigned)
                3'b111: branch_taken = (~ALU_result[0]); // BGEU: SLTU==0 → src1 >= src2 (unsigned)
                default: branch_taken = 1'b0; // 不合法的分支指令，默认不跳转
            endcase
        end else begin
            branch_taken = 1'b0;
        end
    end

    always_comb begin : PC_next_sel
        if(ctrl_sig.jmp_en)
            case(ctrl_sig.PC_sel)
                PC_PLUS4:   PC_next = PCInc4;
                PC_BRANCH:  PC_next = branch_taken ? PC_current + PC_rel_imm : PCInc4; // 分支跳转
                PC_JMP:     PC_next = is_jal ? PC_current + PC_rel_imm : ALU_result & ~1; // 无条件跳转
                PC_TRAP_ENT: PC_next = CSR_tvec;
                PC_TRAP_RET: PC_next = CSR_epc;
                default: PC_next = PCInc4;
            endcase
        else
            PC_next = PCInc4; // 默认顺序执行
    end
    // PC寄存器
    always_ff @( posedge clk, negedge rst_n ) begin : PC_update
        if(!rst_n) begin
            PC_current <= RESET_VEC;
        end else if(ifu_valid && idu_ready) begin
            PC_current <= PC_next;
        end
    end

    localparam BYTES_PER_WORD = DATA_WIDTH / 8;
    localparam ALIGNED_WIDTH = $clog2(BYTES_PER_WORD);
    localparam ROM_DEPTH = 1 << (ADDR_WIDTH - ALIGNED_WIDTH); 

    logic fetch_exception;
    logic [ADDR_WIDTH-ALIGNED_WIDTH-1:0] word_idx;
    logic [DATA_WIDTH-1:0] mapped_addr;
    assign mapped_addr = PC_current - RESET_VEC; // 将访问地址映射到ROM

    always_comb begin : fetch
        fetch_exception = (mapped_addr[ALIGNED_WIDTH-1:0] != 0); // 检查是否4字节对齐
        if(fetch_exception) begin
            word_idx = 0;
            IF_exception = EXC_INST_MISALIGNED;
        end else begin
            word_idx = mapped_addr[ADDR_WIDTH-1:ALIGNED_WIDTH]; // 4字节对齐地址
            IF_exception = EXC_NONE;
        end
    end

    ROM #(.DATA_WIDTH(DATA_WIDTH), .SIZE(ROM_DEPTH)) rom (
        .clk(clk),
        .rst_n(rst_n),
        .raddr(word_idx),
        .rdata(instruction)
    );
// 确保 PC 永远是 4 字节对齐的（除非你有异常处理）
property p_pc_aligned;
    @(posedge clk) (rst_n) |-> (PC_current[ALIGNED_WIDTH-1:0] == 0);
endproperty
assert property (p_pc_aligned) else $error("PC Misaligned at time %t", $time);

endmodule
