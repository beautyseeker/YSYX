module sCPU 
(
    input  logic        clk,
    input  logic        rst,
    output logic [7:0]  out_seg0,
    output logic [7:0]  out_seg1
);
    typedef enum logic [1:0] {
        OP_ADD  = 2'b00,
        OP_OUT  = 2'b01, // 驱动数码管
        OP_LI   = 2'b10, // 立即数加载
        OP_BNE  = 2'b11  // 不相等则跳转
    } op_code_e;

    // -------------------------------------------------------------------------
    // 1. 参数与内部信号定义 (有序分组)
    // -------------------------------------------------------------------------
    logic [7:0] inst_rom [0:15];
    logic [7:0] regfile  [0:3];
    logic [3:0] pc;

    // 指令解析信号
    logic [7:0] inst;
    op_code_e   op;
    logic [1:0] rd, rs1, rs2;
    logic [3:0] imm, b_addr;

    // 控制与数据通路信号
    logic       wr_en;
    logic [7:0] wr_data;
    logic [7:0] seg_data;

    // -------------------------------------------------------------------------
    // 2. 指令获取与解码 (Instruction Fetch & Decode)
    // -------------------------------------------------------------------------
    assign inst   = inst_rom[pc];
    assign op     = op_code_e'(inst[7:6]); // 强制类型转换，增加可读性
    assign rd     = inst[5:4];
    assign rs1    = inst[3:2];
    assign rs2    = inst[1:0];
    assign imm    = inst[3:0];
    assign b_addr = inst[5:2];

    // -------------------------------------------------------------------------
    // 3. 控制逻辑 (Execution Control)
    // -------------------------------------------------------------------------
    always_comb begin
        // 默认值，防止生成 latch
        wr_en    = 1'b0;
        wr_data  = 8'b0;
        
        case (op)
            OP_ADD: begin
                wr_en   = 1'b1;
                wr_data = regfile[rs1] + regfile[rs2];
            end
            OP_LI: begin
                wr_en   = 1'b1;
                wr_data = {4'b0, imm};
            end
            OP_OUT: begin
                // OUT指令不写回寄存器，只更新数码管寄存器
                wr_en   = 1'b0;
            end
            OP_BNE: begin
                wr_en   = 1'b0;
            end
            default: ;
        endcase
    end

    // -------------------------------------------------------------------------
    // 4. 寄存器堆与程序计数器更新 (Sequential Logic)
    // -------------------------------------------------------------------------
    // PC 更新逻辑
    always_ff @(posedge clk or negedge rst) begin
        if (!rst) begin
            pc <= 4'b0;
        end else begin
            if (op == OP_BNE && regfile[rs2] != regfile[0])
                pc <= b_addr;
            else
                pc <= pc + 1'b1;
        end
    end

    // 寄存器堆更新
    always_ff @(posedge clk or negedge rst) begin
        if (!rst) begin
            for (int i = 0; i < 4; i++) begin
                regfile[i] <= 8'b0;
            end
        end else if (wr_en) begin
            regfile[rd] <= wr_data;
        end
    end

    // 数码管数据寄存器 (解耦显示逻辑与核心运算)
    always_ff @(posedge clk or negedge rst) begin
        if (!rst) begin
            seg_data <= 8'h0;
        end else if (op == OP_OUT) begin
            seg_data <= regfile[rs1];
        end
    end

    // -------------------------------------------------------------------------
    // 5. 模块例化 (Instantiation) - 使用 .* 自动连线
    // -------------------------------------------------------------------------
    seg7_encoder_en u_seg0 (
        .en (1'b1),
        .in (seg_data[3:0]),
        .out(out_seg0)
    );

    seg7_encoder_en u_seg1 (
        .en (1'b1),
        .in (seg_data[7:4]),
        .out(out_seg1)
    );

    // -------------------------------------------------------------------------
    // 6. 仿真与监控 (Debug/Verification)
    // -------------------------------------------------------------------------
    `ifndef SYNTHESIS
        initial $readmemb("./resource/inst_rom.bin", inst_rom);
        initial begin
            $monitor("PC=%d | inst=%b | op=%b | r0=%d | r1=%d | r2=%d | r3=%d",
            pc, inst, op, regfile[0], regfile[1], regfile[2], regfile[3]);
        end
        // 命名断言：确保地址不会超出ROM范围
        a_pc_range: assert property (@(posedge clk) pc < 15);

    `endif

endmodule

module top_sCPU 
(
    input  logic        clk,
    input  logic        rst,
    output logic [7:0]  out_seg0,
    output logic [7:0]  out_seg1
);
    sCPU u_sCPU (
        .*
    );
endmodule