// GPIO APB controller
// 0x0: LED  out[15:0]
// 0x4: SW   in [15:0] (read-only)
// 0x8: SEG  每 4bit 一只数码管（共 8 只）
// 0xc: reserved
module gpio_top_apb(
  input         clock,
  input         reset,
  input  [31:0] in_paddr,
  input         in_psel,
  input         in_penable,
  input  [2:0]  in_pprot,
  input         in_pwrite,
  input  [31:0] in_pwdata,
  input  [3:0]  in_pstrb,
  output        in_pready,
  output [31:0] in_prdata,
  output        in_pslverr,

  output [15:0] gpio_out,
  input  [15:0] gpio_in,
  output [7:0]  gpio_seg_0,
  output [7:0]  gpio_seg_1,
  output [7:0]  gpio_seg_2,
  output [7:0]  gpio_seg_3,
  output [7:0]  gpio_seg_4,
  output [7:0]  gpio_seg_5,
  output [7:0]  gpio_seg_6,
  output [7:0]  gpio_seg_7
);

  localparam LED = 4'h0;
  localparam SW = 4'h4;
  localparam SEGS = 4'h8;

  // NVBoard 七段为低电平点亮：内部先得共阳码再取反
  function automatic [7:0] hex2seg(input [3:0] hex);
    reg [7:0] segs;
    begin
      case (hex)
        4'h0: segs = 8'b11111101;
        4'h1: segs = 8'b01100000;
        4'h2: segs = 8'b11011010;
        4'h3: segs = 8'b11110010;
        4'h4: segs = 8'b01100110;
        4'h5: segs = 8'b10110110;
        4'h6: segs = 8'b10111110;
        4'h7: segs = 8'b11100000;
        4'h8: segs = 8'b11111110;
        4'h9: segs = 8'b11110110;
        4'ha: segs = 8'b11101110;
        4'hb: segs = 8'b00111110;
        4'hc: segs = 8'b10011100;
        4'hd: segs = 8'b01111010;
        4'he: segs = 8'b10011110;
        4'hf: segs = 8'b10001110;
        default: segs = 8'b00000000;
      endcase
      hex2seg = ~segs;
    end
  endfunction

  wire        fire   = in_psel && in_penable;
  wire [3:0]  offset = in_paddr[3:0];

  assign in_pready  = 1'b1;
  assign in_pslverr = 1'b0;

  reg [15:0] led_r;
  reg [31:0] seg_r;

  always @(posedge clock) begin
    if (reset) begin
      led_r <= 16'h0;
      seg_r <= 32'h0;
    end else if (fire && in_pwrite) begin
      case (offset)
        LED: begin
          if (in_pstrb[0]) led_r[ 7:0] <= in_pwdata[ 7:0];
          if (in_pstrb[1]) led_r[15:8] <= in_pwdata[15:8];
        end
        SEGS: begin
          if (in_pstrb[0]) seg_r[ 7: 0] <= in_pwdata[ 7: 0];
          if (in_pstrb[1]) seg_r[15: 8] <= in_pwdata[15: 8];
          if (in_pstrb[2]) seg_r[23:16] <= in_pwdata[23:16];
          if (in_pstrb[3]) seg_r[31:24] <= in_pwdata[31:24];
        end
        default: ;
      endcase
    end
  end

  assign gpio_out   = led_r;
  assign gpio_seg_0 = hex2seg(seg_r[ 3: 0]);
  assign gpio_seg_1 = hex2seg(seg_r[ 7: 4]);
  assign gpio_seg_2 = hex2seg(seg_r[11: 8]);
  assign gpio_seg_3 = hex2seg(seg_r[15:12]);
  assign gpio_seg_4 = hex2seg(seg_r[19:16]);
  assign gpio_seg_5 = hex2seg(seg_r[23:20]);
  assign gpio_seg_6 = hex2seg(seg_r[27:24]);
  assign gpio_seg_7 = hex2seg(seg_r[31:28]);

  reg [31:0] rdata;
  always @(*) begin
    case (offset)
      LED:    rdata = {16'h0, led_r};
      SW:    rdata = {16'h0, gpio_in};
      SEGS:    rdata = seg_r;
      default: rdata = 32'h0;
    endcase
  end
  assign in_prdata = rdata;

  wire _unused = |in_pprot;

endmodule
