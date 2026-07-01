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
    logic branch_taken;

    localparam BYTES_PER_WORD = DATA_WIDTH / 8;
    localparam ALIGNED_WIDTH = $clog2(BYTES_PER_WORD);
    localparam ROM_DEPTH = 1 << (ADDR_WIDTH - ALIGNED_WIDTH);

    logic fetch_exception;
    logic [ADDR_WIDTH-ALIGNED_WIDTH-1:0] word_idx;
    logic [DATA_WIDTH-1:0] mapped_addr;

    assign PCInc4 = PC_current + 4;
    assign is_jal = instruction[3];

    assign mapped_addr = PC_current - RESET_VEC; // 取指请求地址映射到ROM

    always_comb begin : branch_decision
        if(ctrl_sig.jmp_en && ctrl_sig.PC_sel == PC_BRANCH) begin
            case (instruction[14:12]) // funct3
                3'b000: branch_taken = ALU_zero; // BEQ
                3'b001: branch_taken = ~ALU_zero; // BNE
                3'b100: branch_taken = (ALU_result[0]); // BLT
                3'b101: branch_taken = (~ALU_result[0]); // BGE
                3'b110: branch_taken = (ALU_result[0]); // BLTU
                3'b111: branch_taken = (~ALU_result[0]); // BGEU
                default: branch_taken = 1'b0;
            endcase
        end else begin
            branch_taken = 1'b0;
        end
    end

    always_comb begin : PC_next_sel
        if(ctrl_sig.jmp_en)
            case(ctrl_sig.PC_sel)
                PC_PLUS4:    PC_next = PCInc4;
                PC_BRANCH:   PC_next = branch_taken ? PC_current + PC_rel_imm : PCInc4;
                PC_JMP:      PC_next = is_jal ? PC_current + PC_rel_imm : (ALU_result & ~1);
                PC_TRAP_ENT: PC_next = CSR_tvec;
                PC_TRAP_RET: PC_next = CSR_epc;
                default:     PC_next = PCInc4;
            endcase
        else
            PC_next = PCInc4;
    end

    always_comb begin : fetch_addr_decode
        fetch_exception = (mapped_addr[ALIGNED_WIDTH-1:0] != 0);
        if(fetch_exception) begin
            word_idx = '0;
            IF_exception = EXC_INST_MISALIGNED;
        end else begin
            word_idx = mapped_addr[ADDR_WIDTH-1:ALIGNED_WIDTH];
            IF_exception = EXC_NONE;
        end
    end

    enum logic [1:0] {FETCH, WAIT, DONE} current, next;
    logic instValid;

    always_comb begin : state_logic
        next = current;
        case(current)
            FETCH: begin  // 取指中
                if(instValid) next = WAIT;
            end
            WAIT: begin  // 取指成功待响应
                if(idu_ready) begin
                    next = DONE;
                end
            end
            DONE:  // 取指成功已响应，握手成功
                next = FETCH;
            default: begin
                next = FETCH;
            end
        endcase
    end

    always_ff @(posedge clk, negedge rst_n) begin : state_ff
        if(!rst_n)
            current <= FETCH;
        else
            current <= next;
    end

    logic fire;
    assign ifu_valid = instValid && (current == WAIT); // 只要指令被取出，且等待下游处理，就始终拉高指令合法信号
    assign fire = current == DONE;  // 合法的指令信号得到了下游响应，拉高握手信号

    always_ff @(posedge clk, negedge rst_n) begin : IF_pipeline
        if(!rst_n) begin
            PC_current <= RESET_VEC;
        end else if (fire) begin
            PC_current <= PC_next;
        end
    end

    ROM #(.DATA_WIDTH(DATA_WIDTH), .SIZE(ROM_DEPTH)) rom (
        .clk(clk),
        .rst_n(rst_n),
        .addrValid(1),
        .raddr(word_idx),
        .rdata(instruction),
        .instValid(instValid)
    );

    property p_pc_aligned;
        @(posedge clk) (rst_n && ifu_valid) |-> (PC_current[ALIGNED_WIDTH-1:0] == 0);
    endproperty
    assert property (p_pc_aligned) else $error("PC Misaligned at time %t", $time);

endmodule
