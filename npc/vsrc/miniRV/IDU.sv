
// `include "defs_pkg.sv"
import defs_pkg::*;
import "DPI-C" function void handle_sys_brk();

module IDU #(parameter DATA_WIDTH = 32, REG_ADDR_WIDTH = 5)
(
    input logic                  clk,
    input logic                  rst_n,
    input logic                  lsu_ready,
    input logic [31:0]           inst,
    input logic                  ifu_valid,

    // 寄存器地址输出
    output logic [REG_ADDR_WIDTH-1:0] rs1_addr,
    output logic [REG_ADDR_WIDTH-1:0] rs2_addr,
    output logic [REG_ADDR_WIDTH-1:0] rd_addr,

    // 控制信号输出
    output logic [DATA_WIDTH-1:0] imm, // 经过扩展的最终立即数
    output Ctrl_sig_t ctrl_sig,
    output logic idu_ready
);

// inst为非访存指令时,idu_ready = 1'b1，当前周期就完成译码并就绪
// inst为访存指令时,idu_ready = lsu_ready，等待LSU完成访存操作

    logic is_access_mem;
    assign is_access_mem = ifu_valid && inst[6:0] inside {7'b0000011, 7'b0100011};
    assign idu_ready = is_access_mem ? lsu_ready : 1'b1;

localparam logic ENABLE = 1'b1;
localparam logic DISABLE = 1'b0;
localparam Ctrl_sig_t DEFAULT_CTRL_SIG = '{
    ALU_op: ALU_ADD,
    reg_write_en: DISABLE,
    mem_read_en: DISABLE,
    mem_write_en: DISABLE,
    WB_sel: ALU_RES,
    jmp_en: DISABLE,
    PC_sel: PC_PLUS4,
    ALU_a_src_sel: A_SRC_REG,
    ALU_b_src_sel: B_SRC_REG,
    mem_sign: MEM_SIGNED,
    mem_size: MEM_WORD,
    EXCPT_code: EXC_NONE
};

    logic [6:0] opcode;
    logic [2:0] funct3;
    logic [6:0] funct7;
    logic [11:0] immI;
    logic [20:0] immJ;
    logic [11:0] immS;
    logic [12:0] immB;
    logic [19:0] immU;
    logic [4:0]  immCSR;
    // 指令字段解析
    assign opcode   = inst[6:0];
    assign rd_addr  = inst[7+REG_ADDR_WIDTH-1:7];
    assign funct3   = inst[14:12];
    assign rs1_addr = inst[15+REG_ADDR_WIDTH-1:15];
    assign rs2_addr = inst[20+REG_ADDR_WIDTH-1:20];
    assign funct7   = inst[31:25];

    // 立即数生成（以 I-type 为例，其他类型需要根据 opcode 进行区分）
    assign immI = inst[31:20];
    assign immS = {inst[31:25], inst[11:7]};
    assign immB = {inst[31], inst[7], inst[30:25], inst[11:8], 1'b0};
    assign immU = inst[31:12];
    assign immJ = {inst[31], inst[19:12], inst[20], inst[30:21], 1'b0};
    assign immCSR = inst[19:15];

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
            7'b1110011:
                imm = {{DATA_WIDTH-5{1'b0}}, immCSR}; // SYSTEM
            default:
                imm = {DATA_WIDTH{1'b0}};
        endcase
    end

    always_comb begin : CtrlGen
        ctrl_sig = DEFAULT_CTRL_SIG;
        case(opcode)
            7'b0110011: begin : reg_op
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
            7'b0010011: begin : reg_imm_op
                ctrl_sig.reg_write_en = ENABLE;
                ctrl_sig.ALU_b_src_sel = B_SRC_IMM;
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
            7'b0000011: begin : mem_load
                ctrl_sig.reg_write_en = ENABLE;
                ctrl_sig.mem_read_en = ENABLE;
                ctrl_sig.WB_sel = MEM_LOAD;
                ctrl_sig.ALU_b_src_sel = B_SRC_IMM; // imm
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
            7'b1100111: begin : non_conditional_short_jmp_JALR
                ctrl_sig.reg_write_en = ENABLE;
                ctrl_sig.WB_sel = PC_INC;
                ctrl_sig.jmp_en = ENABLE;
                ctrl_sig.PC_sel = PC_JMP;
                ctrl_sig.ALU_b_src_sel = B_SRC_IMM;
                ctrl_sig.ALU_op = ALU_ADD;  // JALR需要用到ALU来计算目标地址
            end
            7'b0100011: begin : mem_store
                ctrl_sig.mem_write_en = ENABLE;
                ctrl_sig.ALU_b_src_sel = B_SRC_IMM; // imm
                ctrl_sig.ALU_op = ALU_ADD; // ADD for address calculation
                case(funct3)
                    3'b000: ctrl_sig.mem_size = MEM_BYTE; // SB
                    3'b001: ctrl_sig.mem_size = MEM_HALF; // SH
                    3'b010: ctrl_sig.mem_size = MEM_WORD; // SW
                    default: ctrl_sig = 'x; // INVALID
                endcase
            end
            7'b1100011: begin : conditional_jmp
                ctrl_sig.jmp_en = ENABLE;
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
            7'b0110111: begin : LUI
                ctrl_sig.reg_write_en = ENABLE;
                ctrl_sig.ALU_b_src_sel = B_SRC_IMM; // imm
                ctrl_sig.ALU_op = ALU_COPY_B; // 直接透传立即数
            end
            7'b0010111: begin : AUIPC
                ctrl_sig.reg_write_en = ENABLE;
                ctrl_sig.ALU_a_src_sel = A_SRC_PC;
                ctrl_sig.ALU_b_src_sel = B_SRC_IMM;
                ctrl_sig.ALU_op = ALU_ADD;
            end
            7'b1101111: begin : non_conditional_long_jmp_JAL
                ctrl_sig.reg_write_en = ENABLE;
                ctrl_sig.WB_sel = PC_INC;
                ctrl_sig.jmp_en = ENABLE;
                ctrl_sig.PC_sel = PC_JMP;
            end

            7'b1110011: begin : SYSTEM
                case(funct3)
                    3'b000: begin // ECALL or EBREAK
                        if (inst == 32'h00000073) begin // ECALL
                            ctrl_sig.PC_sel = PC_TRAP_ENT;
                            ctrl_sig.jmp_en = ENABLE;
                            ctrl_sig.EXCPT_code = EXC_ECALL_M;
                        end else if (inst == 32'h00100073) begin // EBREAK
                            $display("EBREAK encountered at time %t. Simulation will stop.", $time);
                            handle_sys_brk();
                            $finish;
                        end
                            else if(inst == 32'h30200073) begin // MRET
                                ctrl_sig.jmp_en = ENABLE;
                                ctrl_sig.PC_sel = PC_TRAP_RET;
                            end
                        else begin
                            ctrl_sig = 'x; // INVALID SYSTEM instruction
                        end
                    end
                    3'b001, 3'b010, 3'b011, 3'b101, 3'b110, 3'b111: begin 
                        ctrl_sig.reg_write_en = ENABLE;
                        ctrl_sig.WB_sel = CSR;
                    end
                    default: ctrl_sig = 'x; // INVALID SYSTEM instruction (CSR instructions not implemented)
                endcase

            end
            default: begin
                ctrl_sig = 'x; // INVALID instruction
            end
        endcase
        if(!ifu_valid) begin
            ctrl_sig = DEFAULT_CTRL_SIG;
        end
    end
endmodule
