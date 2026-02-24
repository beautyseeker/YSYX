package defs_pkg;
    // 1. 先定义基础枚举类型
    typedef enum logic [3:0] {
        ALU_ADD    = 4'b0000,
        ALU_SUB    = 4'b1000,
        ALU_SLL    = 4'b0001,
        ALU_SLT    = 4'b0010,
        ALU_SLTU   = 4'b0011,
        ALU_XOR    = 4'b0100,
        ALU_SRL    = 4'b0101,
        ALU_SRA    = 4'b1101,
        ALU_OR     = 4'b0110,
        ALU_AND    = 4'b0111,
        ALU_COPY_B = 4'b1111 
    } alu_op_t;

    typedef enum logic[1:0] {
        B_SRC_REG = 2'b00,
        B_SRC_IMM = 2'b01,
        B_SRC_PC  = 2'b10
    } ALU_b_src_sel_e;

    typedef enum logic [1:0] {
        A_SRC_REG = 2'b00,
        A_SRC_IMM = 2'b01,
        A_SRC_PC  = 2'b10
    } ALU_a_src_sel_e;

    typedef enum logic {
        MEM_SIGNED   = 1'b1,
        MEM_UNSIGNED = 1'b0
    } mem_sign_e;

    typedef enum logic [1:0] {
        MEM_BYTE     = 2'b00,
        MEM_HALF     = 2'b01,
        MEM_WORD     = 2'b10
    } mem_size_e;

    typedef enum logic [1:0] { 
        ALU_RES = 2'b00,
        MEM_LOAD = 2'b01,
        PC_INC = 2'b10
     } WB_sel_e;

    typedef enum logic [2:0] {
        PC_PLUS4  = 3'b000,
        PC_BRANCH = 3'b001,
        PC_JMP   = 3'b010,
        PC_EXCEPT = 3'b011
    } PC_sel_e;

    typedef enum logic [3:0] { 
        EXC_INST_MISALIGNED = 4'd0,
        EXC_INST_FAULT      = 4'd1,
        EXC_ILLEGAL_INST    = 4'd2,
        EXC_BREAKPOINT      = 4'd3,
        EXC_ACCESS_MISALIGNED = 4'd4,
        EXC_ACCESS_OUT_OF_RANGE = 4'd5,
        EXC_ECALL_M         = 4'd11,
        EXC_NONE            = 4'd15  // 自定义：无异常
    } exception_t;

    // 2. 最后定义引用了上述类型的结构体
    typedef struct packed {
        alu_op_t        ALU_op;      
        logic           reg_write_en;
        logic           mem_read_en; 
        logic           mem_write_en;
        WB_sel_e        WB_sel;
        logic           jmp_en;   
        PC_sel_e        PC_sel; 
        ALU_a_src_sel_e   ALU_a_src_sel;     
        ALU_b_src_sel_e   ALU_b_src_sel;
        mem_sign_e      mem_sign;
        mem_size_e      mem_size;
    } Ctrl_sig_t; 

    // 添加默认内存初始化文件路径
    parameter string RAM_INIT_FILE_DEFAULT = "./resource/addi.hex";
    parameter string ROM_INIT_FILE_DEFAULT = "./resource/addi.hex";
endpackage
