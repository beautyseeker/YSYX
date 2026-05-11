// `include "defs_pkg.sv"
import defs_pkg::*;

module top_TopMiniRV #(parameter DATA_WIDTH = 32, ADDR_WIDTH = 27, REG_COUNT = 16, RESET_VEC = 32'h8000_0000)
(
    input logic                  clk,
    input logic                  rst_n,

    output logic [DATA_WIDTH-1:0] PC_current,
    output logic [DATA_WIDTH-1:0] PC_next,
    output logic [DATA_WIDTH-1:0] instruction,
    output logic [DATA_WIDTH-1:0] gpr [REG_COUNT-1:0], // 输出整个寄存器文件状态，便于调试
    output CSR_bundle_out csr_bundle
);
    // 模块实例化
    localparam REG_ADDR_WIDTH = $clog2(REG_COUNT); // 寄存器地址宽度，根据寄存器数量计算
    logic [REG_ADDR_WIDTH-1:0] Rs1_addr, Rs2_addr, Rd_addr;
    logic [DATA_WIDTH-1:0] Rs1_data, Rs2_data, Rd_data;
    logic [DATA_WIDTH-1:0] alu_result;
    logic ALU_zero;
    logic [DATA_WIDTH-1:0] mem_load_data;
    logic [DATA_WIDTH-1:0] imm_ext;
    logic [DATA_WIDTH-1:0] CSR_reg;


    Ctrl_sig_t ctrl_sig;
    except_cause if_exception, id_exception, ex_exception, mem_exception, exception;
    always_comb begin : exception_arbiter
        // 简单的优先级仲裁：IF > ID > EX > MEM
        if (if_exception != EXC_NONE) begin
            exception = if_exception;
        end else if (id_exception != EXC_NONE) begin
            exception = id_exception;
        end else if (ex_exception != EXC_NONE) begin
            exception = ex_exception;
        end else if (mem_exception != EXC_NONE) begin
            exception = mem_exception;
        end else begin
            exception = EXC_NONE;
        end
        
    end

    IFU #(.DATA_WIDTH(DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH), .RESET_VEC(RESET_VEC)) ifu (
        .clk(clk),
        .rst_n(rst_n),
        .ctrl_sig(ctrl_sig),
        .ALU_result(alu_result),
        .ALU_zero(ALU_zero),
        .PC_rel_imm(imm_ext),
        .CSR_tvec(csr_bundle.CSR_tvec), // 由CSRFile提供
        .CSR_epc(csr_bundle.CSR_epc), // 由CSRFile提供

        .PC_current(PC_current),
        .PC_next(PC_next),
        .instruction(instruction),
        .IF_exception(if_exception)
    );

    IDU #(.DATA_WIDTH(DATA_WIDTH), .REG_ADDR_WIDTH(REG_ADDR_WIDTH)) idu (
        .inst(instruction),
        .rs1_addr(Rs1_addr),
        .rs2_addr(Rs2_addr),
        .rd_addr(Rd_addr),
        .imm(imm_ext),
        .ctrl_sig(ctrl_sig),
        .ID_exception(id_exception)
    );

    RegisterFile #(.DATA_WIDTH(DATA_WIDTH), .REG_COUNT(REG_COUNT)) regfile (
        .clk(clk),
        .rst_n(rst_n),
        .rs1_addr(Rs1_addr),
        .rs2_addr(Rs2_addr),
        .rd_addr(Rd_addr),
        .rs1_data(Rs1_data),
        .rs2_data(Rs2_data),
        .write_data(Rd_data),
        .reg_write_en(ctrl_sig.reg_write_en),
        .gpr(gpr)
    );

    logic [DATA_WIDTH-1:0] alu_b;
    always_comb begin : ALU_b_src_sel
        case(ctrl_sig.ALU_b_src_sel)
            B_SRC_REG: alu_b = Rs2_data;
            B_SRC_IMM: alu_b = imm_ext;
            default: alu_b = 'x; // 不应该发生
        endcase
        assert (ctrl_sig.ALU_b_src_sel inside {B_SRC_REG, B_SRC_IMM})
        else $error("Invalid ALU_b_src_sel: %0d at time %t", ctrl_sig.ALU_b_src_sel, $time);
    end

    // alu_a仅在opcode为AUIPC时使用PC_current，否则为Rs1_data
    logic [DATA_WIDTH-1:0] alu_a;
    assign alu_a = ctrl_sig.ALU_a_src_sel == A_SRC_PC ? DATA_WIDTH'($signed(PC_current)) : Rs1_data;

    EXU #(.DATA_WIDTH(DATA_WIDTH)) exu (
        .alu_a(alu_a),
        .alu_b(alu_b),
        .ALU_op(ctrl_sig.ALU_op),
        .alu_zero(ALU_zero),
        .ALU_result(alu_result)
    );

    CSRFile #(.XLEN(DATA_WIDTH)) csrfile (
        .clk(clk),
        .rst_n(rst_n),
        .ctrl_sig(ctrl_sig),
        .csr_instruction(instruction),
        .PC_current(PC_current),
        .EXCPT_code(exception),
        .CSR_RS1(Rs1_data),
        .CSR_imm(imm_ext),
        .csr_write_en(ctrl_sig.WB_sel == CSR && ctrl_sig.reg_write_en),
        .csr_read_out(CSR_reg),
        .csr_bundle_out(csr_bundle)
    );

    LSU #(.DATA_WIDTH(DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH), .PMEM_BASE(RESET_VEC)) lsu (
        .clk(clk),
        .rst_n(rst_n),
        .addr(alu_result), // 地址由ALU计算得到
        .store_data(Rs2_data), // 存储数据来自寄存器
        .mem_size(ctrl_sig.mem_size), // 根据指令类型设置
        .mem_sign(ctrl_sig.mem_sign), // 根据指令类型设置
        .mem_write_en(ctrl_sig.mem_write_en),
        .mem_read_en(ctrl_sig.mem_read_en),
        .load_data(mem_load_data),
        .mem_exception(mem_exception)
    );


    WBU #(.XLEN(DATA_WIDTH), .ADDR_WIDTH(ADDR_WIDTH)) wbu (
        .alu_result(alu_result),
        .mem_load_data(mem_load_data),
        .WB_sel(ctrl_sig.WB_sel),
        .PC_current(PC_current),
        .CSR_data(CSR_reg),
        .WB_data(Rd_data)
    );
endmodule
