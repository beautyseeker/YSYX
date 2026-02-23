`include "defs_pkg.sv"
import defs_pkg::*;

module IDU #(parameter DATA_WIDTH = 32)
(
    input logic [31:0] inst,

    // 寄存器地址输出
    output logic [4:0] rs1_addr,
    output logic [4:0] rs2_addr,
    output logic [4:0] rd_addr,

    // 控制信号输出
    output logic [DATA_WIDTH-1:0] imm, // 经过扩展的最终立即数
    output Ctrl_sig_t ctrl_sig
);

localparam logic ENABLE = 1'b1;
localparam logic DISABLE = 1'b0;
localparam Ctrl_sig_t DEFAULT_CTRL_SIG = '{
    ALU_op: ALU_COPY_B,
    reg_write_en: DISABLE,
    mem_read_en: DISABLE,
    mem_write_en: DISABLE,
    mem_to_reg: DISABLE,
    branch_en: DISABLE,
    PC_sel: PC_PLUS4,
    ALU_src_sel: ALU_SRC_REG,
    mem_sign: MEM_SIGNED,
    mem_size: MEM_WORD
};

    logic [6:0] opcode;
    logic [2:0] funct3;
    logic [6:0] funct7;
    logic [11:0] immI;
    logic [20:0] immJ;
    logic [11:0] immS;
    logic [12:0] immB;
    logic [19:0] immU;
    // 指令字段解析
    assign opcode   = inst[6:0];
    assign rd_addr  = inst[11:7];
    assign funct3   = inst[14:12];
    assign rs1_addr = inst[19:15];
    assign rs2_addr = inst[24:20];
    assign funct7   = inst[31:25];

    // 立即数生成（以 I-type 为例，其他类型需要根据 opcode 进行区分）
    assign immI = inst[31:20];
    assign immS = {inst[31:25], inst[11:7]};
    assign immB = {inst[31], inst[7], inst[30:25], inst[11:8], 1'b0};
    assign immU = inst[31:12];
    assign immJ = {inst[31], inst[19:12], inst[20], inst[30:21], 1'b0};


    always_comb begin : ImmGen
        case(opcode)
            7'b0010011, 7'b0000011, 7'b1100111: 
                imm = DATA_WIDTH'($signed(immI)); // I-type
            7'b0100011: 
                imm = DATA_WIDTH'($signed(immS)); // S-type
            7'b1100011: 
                imm = DATA_WIDTH'($signed(immB)); // B-type
            7'b0110111, 7'b0010111:
                imm = {immU, {DATA_WIDTH-20{1'b0}}}; // U-type
            7'b1101111: 
                imm = DATA_WIDTH'($signed(immJ)); // J-type
            default:
                imm = {DATA_WIDTH{1'b0}};
        endcase
    end

    always_comb begin : CtrlGen
        ctrl_sig = DEFAULT_CTRL_SIG;
        case(opcode)
            7'b0110011: begin // R-type
                ctrl_sig.reg_write_en = ENABLE;
                case({funct7, funct3})
                    10'b0000000_000: ctrl_sig.ALU_op = ALU_ADD; // ADD
                    10'b0100000_000: ctrl_sig.ALU_op = ALU_SUB; // SUB
                    10'b0000000_111: ctrl_sig.ALU_op = ALU_AND; // AND
                    10'b0000000_110: ctrl_sig.ALU_op = ALU_OR; // OR
                    10'b0000000_001: ctrl_sig.ALU_op = ALU_SLL; // SLL
                    10'b0000000_101: ctrl_sig.ALU_op = ALU_SRL; // SRL
                    10'b0100000_101: ctrl_sig.ALU_op = ALU_SRA; // SRA
                    10'b0000000_010: ctrl_sig.ALU_op = ALU_SLT; // SLT
                    10'b0000000_011: ctrl_sig.ALU_op = ALU_SLTU; // SLTU
                    10'b0000000_100: ctrl_sig.ALU_op = ALU_XOR; // XOR
                    default:         ctrl_sig = 'x; // INVALID
                endcase
            end
            7'b0010011: begin // I-type (ALU immediate)
                ctrl_sig.reg_write_en = ENABLE;
                ctrl_sig.ALU_src_sel = ALU_SRC_IMM;
                case(funct3)
                    3'b000: ctrl_sig.ALU_op = ALU_ADD; // ADDI
                    3'b010: ctrl_sig.ALU_op = ALU_SLT; // SLTI
                    3'b011: ctrl_sig.ALU_op = ALU_SLTU; // SLTIU
                    3'b100: ctrl_sig.ALU_op = ALU_XOR; // XORI
                    3'b110: ctrl_sig.ALU_op = ALU_OR; // ORI
                    3'b111: ctrl_sig.ALU_op = ALU_AND; // ANDI
                    3'b001: ctrl_sig.ALU_op = ALU_SLL;  // SLLI
                    3'b101: ctrl_sig.ALU_op = (funct7 == 7'b0000000) ? ALU_SRL : ALU_SRA; // SRLI/SRAI
                    default:ctrl_sig = 'x; // INVALID
                endcase
            end
            7'b0000011: begin // I-type (load)
                ctrl_sig.reg_write_en = ENABLE;
                ctrl_sig.mem_read_en = ENABLE;
                ctrl_sig.mem_to_reg = ENABLE;
                ctrl_sig.ALU_src_sel = ALU_SRC_IMM; // imm
                ctrl_sig.ALU_op = ALU_ADD; // ADD for address calculation
                case(funct3)
                    3'b000: ctrl_sig.mem_size = MEM_BYTE; // LB
                    3'b001: ctrl_sig.mem_size = MEM_HALF; // LH
                    3'b010: ctrl_sig.mem_size = MEM_WORD; // LW
                    3'b100: 
                        begin ctrl_sig.mem_size = MEM_BYTE; ctrl_sig.mem_sign = MEM_UNSIGNED; end // LBU
                    3'b101: 
                        begin ctrl_sig.mem_size = MEM_HALF; ctrl_sig.mem_sign = MEM_UNSIGNED; end // LHU
                    default: 
                        begin ctrl_sig = 'x; end // INVALID
                endcase
            end
            7'b1100111: begin // I-type (JALR)
                ctrl_sig.reg_write_en = ENABLE;
                ctrl_sig.ALU_src_sel = ALU_SRC_IMM; // imm
                ctrl_sig.ALU_op = ALU_ADD; // PC + imm
                ctrl_sig.PC_sel = PC_JALR;
            end
            7'b0100011: begin // S-type (store)
                ctrl_sig.mem_write_en = ENABLE;
                ctrl_sig.ALU_src_sel = ALU_SRC_IMM; // imm
                ctrl_sig.ALU_op = ALU_ADD; // ADD for address calculation
                case(funct3)
                    3'b000: ctrl_sig.mem_size = MEM_BYTE; // SB
                    3'b001: ctrl_sig.mem_size = MEM_HALF; // SH
                    3'b010: ctrl_sig.mem_size = MEM_WORD; // SW
                    default: ctrl_sig = 'x; // INVALID
                endcase
            end
            7'b1100011: begin // B-type (branch)
                ctrl_sig.branch_en = ENABLE;
                ctrl_sig.ALU_src_sel = ALU_SRC_REG; // rs2
                ctrl_sig.PC_sel = PC_BRANCH;
                case(funct3)
                    3'b000: ctrl_sig.ALU_op = ALU_SUB; // BEQ
                    3'b001: ctrl_sig.ALU_op = ALU_SUB; // BNE
                    3'b100: ctrl_sig.ALU_op = ALU_SLT; // BLT
                    3'b101: ctrl_sig.ALU_op = ALU_SLT; // BGE
                    3'b110: ctrl_sig.ALU_op = ALU_SLTU; // BLTU
                    3'b111: ctrl_sig.ALU_op = ALU_SLTU; // BGEU
                    default:ctrl_sig = 'x; // INVALID
                endcase
            end
            7'b0110111: begin // U-type (LUI)
                ctrl_sig.reg_write_en = ENABLE;
                ctrl_sig.ALU_src_sel = ALU_SRC_IMM; // imm
                ctrl_sig.ALU_op = ALU_COPY_B; // 直接透传立即数
            end
            7'b0010111: begin // U-type (AUIPC)
                ctrl_sig.reg_write_en = ENABLE;
                ctrl_sig.ALU_src_sel = ALU_SRC_IMM; // PC + imm
                ctrl_sig.ALU_op = ALU_ADD; // PC + imm
            end
            7'b1101111: begin // J-type (JAL)
                ctrl_sig.reg_write_en = ENABLE;
                ctrl_sig.ALU_src_sel = ALU_SRC_IMM; // PC + imm
                ctrl_sig.ALU_op = ALU_ADD; // PC + imm
                ctrl_sig.PC_sel = PC_BRANCH;
                ctrl_sig.branch_en = ENABLE;
            end

            7'b1110011: begin // SYSTEM (ECALL/EBREAK)
                //此处将调用DPI-C函数来处理系统调用实现停机
            end
            default: begin
                ctrl_sig = 'x; // INVALID instruction
            end
        endcase
    end
endmodule
