import defs_pkg::*;

module CSRFile #(parameter XLEN=32)
(
    input logic clk,
    input logic rst_n,
    input Ctrl_sig_t ctrl_sig,
    input logic [XLEN-1:0] csr_instruction,
    input logic [XLEN-1:0] CSR_RS1,
    input logic [XLEN-1:0] CSR_imm,
    input logic [XLEN-1:0] PC_current,
    input logic [XLEN-1:0] EXCPT_code,
    input logic csr_write_en,

    output logic [XLEN-1:0] csr_read_out,
    output CSR_bundle_out csr_bundle_out
);
    // 定义一些常用CSR寄存器地址
    localparam CSR_MCYCLE = 12'hB00; // 机器周期计数器（只读）
    localparam CSR_MCYCLEH = 12'hB80; // 机器周期计数器高位（只读）
    localparam CSR_MSTATUS = 12'h300;
    localparam CSR_MTVEC   = 12'h305;
    localparam CSR_MEPC    = 12'h341;
    localparam CSR_MCAUSE  = 12'h342;
    localparam CSR_MVENDORID = 12'hF11; // 供应商ID（只读，返回0）
    localparam CSR_MARCHID   = 12'hF12; // 体系结构ID（只读，返回0）

    // 定义CSR寄存器
    logic [XLEN-1:0] mstatus, mtvec, mepc, mcause, mcycle, mcycleh, mvendorid, marchid;

     // 供应商ID和体系结构ID固定为0，表示这是一个简单的实现
    assign mvendorid = 32'd19960816;
    assign marchid = 32'hdeadbeef;

    logic [XLEN-1:0] CSR_new;
    logic [XLEN-1:0] CSR_old; 
    logic [11:0] csr_addr;
    logic [2:0] CSR_op;

    assign CSR_op = csr_instruction[14:12];
    assign csr_addr = csr_instruction[31:20];
    assign CSR_old = csr_read_out;

    `define DUMP_CSR(name) \
        $display("%-10s = 0x%h", `"name`", name);

    function void dump_all();
        `DUMP_CSR(mstatus)
        `DUMP_CSR(mtvec)
        `DUMP_CSR(mepc)
        `DUMP_CSR(mcause)
        $display("-------------PC:0x%08h instruction = 0x%08h-------------", 
        PC_current, csr_instruction);
    endfunction

    always_comb begin : CSR_ALU
        case(CSR_op)
            3'b001: CSR_new = CSR_RS1; // CSRRW
            3'b010: CSR_new = CSR_old | CSR_RS1; // CSRRS
            3'b011: CSR_new = CSR_old & ~CSR_RS1; // CSRRC
            3'b101: CSR_new = CSR_imm; // CSRRWI
            3'b110: CSR_new = CSR_old | CSR_imm; // CSRRSI
            3'b111: CSR_new = CSR_old & ~CSR_imm; // CSRRCI
            default: CSR_new = 'x;
        endcase
    end

    // CSR读操作（组合逻辑）
    always_comb begin : CSR_read
        case (csr_addr)
            CSR_MCYCLE:    csr_read_out = mcycle;
            CSR_MCYCLEH:   csr_read_out = mcycleh;
            CSR_MSTATUS:   csr_read_out = mstatus;
            CSR_MTVEC:     csr_read_out = mtvec;
            CSR_MEPC:      csr_read_out = mepc;
            CSR_MCAUSE:    csr_read_out = mcause;
            CSR_MARCHID:   csr_read_out = marchid;
            CSR_MVENDORID: csr_read_out = mvendorid;
            default:       csr_read_out = 'x; // 不应该发生
        endcase
        // assert (csr_addr inside {CSR_MSTATUS, CSR_MTVEC, CSR_MEPC, CSR_MCAUSE, 
        // CSR_MCYCLE, CSR_MCYCLEH, CSR_MVENDORID, CSR_MARCHID})
        // else $error("Invalid CSR address: %0h at time %t", csr_addr, $time);
    end

    assign csr_bundle_out = '{
        CSR_tvec: mtvec, 
        CSR_epc: mepc, 
        CSR_mstatus: mstatus, 
        CSR_mcause: mcause, 
        CSR_mcycle: {mcycleh, mcycle}, 
        CSR_mvendorid: mvendorid, 
        CSR_marchid: marchid
    };

    // CSR写操作（时序逻辑）
    always_ff @(posedge clk or negedge rst_n) begin : CSR_write
        if (!rst_n) begin
            {mstatus, mtvec, mepc, mcause} <= '0;
        end
        else begin
            if(ctrl_sig.PC_sel == PC_TRAP_ENT) begin : internal_trap
                mepc <= PC_current; // 保存异常发生时的PC
                mcause <= EXCPT_code; // 保存异常原因
                mstatus[7] <= mstatus[3]; // 将MIE位保存到MPIE
                mstatus <= mstatus & ~32'h8; // 设置MIE位为0关闭中断，屏蔽后续非高优先级中断
                // dump_all();
                // $display("hard trap occurred at PC=0x%08h with cause=0x%08h\n", PC_current, EXCPT_code);
            end 
            else if (ctrl_sig.PC_sel == PC_TRAP_RET) begin
                mstatus[3] <= mstatus[7]; // 恢复MIE位
                mstatus <= mstatus | 32'h8; // 恢复MIE位开中断，返回正常执行
            end
            else if (csr_write_en) begin : CSR_inst_write
                case (csr_addr)
                    CSR_MSTATUS: mstatus <= CSR_new;
                    CSR_MTVEC:   mtvec   <= CSR_new;
                    CSR_MEPC:    mepc    <= CSR_new;
                    CSR_MCAUSE:  mcause  <= CSR_new;
                    default: /* 不应该发生，忽略写入 */ ;
                endcase
                // dump_all();
                // $display("CSR write: addr=0x%03h data=0x%08h\n", csr_addr, CSR_new);    
            end
        end 
    end

    always_ff @(posedge clk or negedge rst_n ) begin : cycle_counter
        if (!rst_n) begin
            mcycle <= '0;
            mcycleh <= '0;
        end else begin
            {mcycleh, mcycle} <= {mcycleh, mcycle} + 1; // 64位周期计数器
        end
    end
endmodule
