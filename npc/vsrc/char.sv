module top_char (
    input         clk,
    input         rst,
    input  logic        ps2_clk,
    input  logic        ps2_data,

    output  logic          hsync,
    output  logic          vsync,
    output  logic          vga_blank_n,
    output  logic [7:0]    vga_r,
    output  logic [7:0]    vga_g,
    output  logic [7:0]    vga_b
);

    // VGA 控制模块
    logic [9:0]    h_addr;
    logic [9:0]    v_addr;
    logic [23:0]   vga_data_in;
    logic [7:0]    keycode;

    vga_ctrl vga_display (
        .pclk      (clk),
        .reset_n     (rst),

        .hsync     (hsync),
        .vsync     (vsync),
        .valid     (vga_blank_n),
        .vga_data  (vga_data_in),

        .h_addr    (h_addr),
        .v_addr    (v_addr),
        .vga_r     (vga_r),
        .vga_g     (vga_g),
        .vga_b     (vga_b)
    );

    // 字符显示模块
    // 在屏幕中央显示字符 'A' (ASCII 65)
    localparam CHAR_X = 10; // 640 / 32 / 2
    localparam CHAR_Y = 3;  // 240 / 16
    logic [7:0] mapping_rom[255:0];
    logic [7:0] ASCIICode;
    initial begin
        $readmemh("./resource/keycode_to_ASCII.mem", mapping_rom);
    end

    assign ASCIICode = mapping_rom[keycode];
    
    ps2_keyboard u_ps2_keyboard (
        .clk        (clk),
        .resetn     (rst),
        .ps2_clk    (ps2_clk),
        .ps2_data   (ps2_data),
        .keycode    (keycode),
        .press_cnt  (),
        .released   ()
    );
    initial begin
        $monitor("ps2data:%h, Keycode: %h, ASCII: %h", ps2_data, keycode, ASCIICode);
    end

    show_char u_show_char (
        .clk         (clk),
        .reset       (rst),
        .pix_x       (h_addr),
        .pix_y       (v_addr),
        .char_x      (CHAR_X),
        .char_y      (CHAR_Y),
        .ascii_code  (ASCIICode), // 固定显示字符 'A'
        .rgb_out     (vga_data_in)
    );
    
endmodule

module show_char(
    input         clk,
    input         reset,

    input  [9:0]  pix_x,  
    input  [9:0]  pix_y,  

    input  [4:0]  char_x,  // 缩放后横向只有 640/32 = 20 个位
    input  [5:0]  char_y,  // 缩放后纵向只有 480/64 = 7.5 个位
    input  [7:0]  ascii_code,

    output [23:0] rgb_out
    );

    // 字模ROM保持不变 (依然是 8x16 的原始数据)
    logic [7:0] font_rom [4095 : 0]; 
    initial begin
        $readmemh("./resource/char_rom.hex", font_rom);
    end

    // --- 关键修改点：缩放逻辑 ---

    // 1. 相对偏移计算 (每个字模像素占用 4x4 物理像素)
    // 我们取 pix_y[5:2] 而不是 [3:0]，相当于 pix_y / 4
    wire [3:0] v_offset = pix_y[5:2]; 
    wire [2:0] h_offset = pix_x[4:2];

    // 2. 区域判定
    // 现在的字符宽度是 32 (2^5)，高度是 64 (2^6)
    // pix_x[9:5] 代表当前处于第几个 32-pixel 宽的格子
    // pix_y[9:6] 代表当前处于第几个 64-pixel 高的格子
    wire in_box = (pix_x[9:5] ==  char_x) 
               && (pix_y[9:6] == char_y[3:0]);

    // 3. 寻址：依然取原始字模行
    wire [7:0] line_tex = font_rom[{ascii_code, v_offset}];

    // 4. 裁决
    wire char_on = in_box && line_tex[3'd7 - h_offset];

    // 5. 输出
    assign rgb_out = char_on ? 24'hffffff : 24'h000000;

endmodule
