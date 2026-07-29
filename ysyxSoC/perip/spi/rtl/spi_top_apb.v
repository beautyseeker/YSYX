// define this macro to enable fast behavior simulation
// for flash by skipping SPI transfers
// `define FAST_FLASH

module spi_top_apb #(
  parameter flash_addr_start = 32'h30000000,
  parameter flash_addr_end   = 32'h3fffffff,
  parameter spi_ss_num       = 8
) (
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

  output                  spi_sck,
  output [spi_ss_num-1:0] spi_ss,
  output                  spi_mosi,
  input                   spi_miso,
  output                  spi_irq_out
);

`ifdef FAST_FLASH

wire [31:0] data;
parameter invalid_cmd = 8'h0;
flash_cmd flash_cmd_i(
  .clock(clock),
  .valid(in_psel && !in_penable),
  .cmd(in_pwrite ? invalid_cmd : 8'h03),
  .addr({8'b0, in_paddr[23:2], 2'b0}),
  .data(data)
);
assign spi_sck    = 1'b0;
assign spi_ss     = 8'b0;
assign spi_mosi   = 1'b1;
assign spi_irq_out= 1'b0;
assign in_pslverr = 1'b0;
assign in_pready  = in_penable && in_psel && !in_pwrite;
assign in_prdata  = data[31:0];

`else

  localparam [4:0] ADR_TX0     = 5'h00;
  localparam [4:0] ADR_TX1     = 5'h04;
  localparam [4:0] ADR_CTRL    = 5'h10;
  localparam [4:0] ADR_DIVIDER = 5'h14;
  localparam [4:0] ADR_SS      = 5'h18;

  // 与软件 spi.c / flash_read 一致
  localparam [31:0] SPI_DIV_VAL  = 32'd1;
  localparam [31:0] SPI_SS_FLASH = 32'h1;
  localparam [31:0] SPI_CTRL_CFG = (32'h1 << 13) | 32'd64; // ASS | CHAR_LEN(64)
  localparam [31:0] SPI_CTRL_GO  = SPI_CTRL_CFG | (32'h1 << 8);

  localparam [3:0]
    S_IDLE  = 4'd0,
    S_WDIV  = 4'd1,
    S_WSS   = 4'd2,
    S_WTX1  = 4'd3,
    S_WTX0  = 4'd4,
    S_WCTRL = 4'd5,
    S_WGO   = 4'd6,
    S_POLL  = 4'd7,
    S_RRX   = 4'd8,
    S_RESP  = 4'd9,
    S_GAP   = 4'd10;

  wire is_flash = (in_paddr >= flash_addr_start) && (in_paddr <= flash_addr_end);
  wire flash_wr_req = in_psel && in_penable && is_flash && in_pwrite;
  wire flash_rd_fire = in_psel && is_flash && !in_pwrite;

  reg  [3:0]  state, state_n;
  reg  [3:0]  state_after_gap;
  reg  [23:0] flash_off;
  reg  [31:0] xip_rdata;
  reg         seen_go;

  wire [4:0]  wb_adr;
  wire [31:0] wb_dat_i, wb_dat_o;
  wire [3:0]  wb_sel;
  wire        wb_we, wb_stb, wb_cyc, wb_ack, wb_err;

  reg  [4:0]  xip_adr;
  reg  [31:0] xip_wdat;
  reg         xip_we, xip_req;

  wire xip_busy   = (state != S_IDLE);
  wire apb_spi_sel = in_psel && !is_flash;

  assign wb_adr   = xip_busy ? xip_adr  : in_paddr[4:0];
  assign wb_dat_i = xip_busy ? xip_wdat : in_pwdata;
  assign wb_sel   = xip_busy ? 4'hF     : in_pstrb;
  assign wb_we    = xip_busy ? xip_we   : in_pwrite;
  assign wb_stb   = xip_busy ? xip_req  : apb_spi_sel;
  assign wb_cyc   = xip_busy ? xip_req  : (in_penable && apb_spi_sel);

  assign in_pready  = flash_wr_req ? 1'b1 :
                      is_flash     ? (state == S_RESP) :
                                     wb_ack;
  assign in_prdata  = is_flash ? xip_rdata : wb_dat_o;
  assign in_pslverr = flash_wr_req;

  // gap 结束后的下一状态（组合，仅在 ack 当拍被锁进 state_after_gap）
  reg [3:0] gap_dest;
  always @(*) begin
    gap_dest = S_IDLE;
    case (state)
      S_WDIV:  gap_dest = S_WSS;
      S_WSS:   gap_dest = S_WTX1;
      S_WTX1:  gap_dest = S_WTX0;
      S_WTX0:  gap_dest = S_WCTRL;
      S_WCTRL: gap_dest = S_WGO;
      S_WGO:   gap_dest = S_POLL;
      S_POLL:  gap_dest = (seen_go && !wb_dat_o[8]) ? S_RRX : S_POLL;
      S_RRX:   gap_dest = S_RESP;
      default: gap_dest = S_IDLE;
    endcase
  end

  always @(*) begin
    state_n  = state;
    xip_adr  = 5'h0;
    xip_wdat = 32'h0;
    xip_we   = 1'b0;
    xip_req  = 1'b0;

    case (state)
      S_IDLE: if (flash_rd_fire) state_n = S_WDIV;

      S_WDIV: begin
        xip_req = 1'b1; xip_we = 1'b1;
        xip_adr = ADR_DIVIDER; xip_wdat = SPI_DIV_VAL;
        if (wb_ack) state_n = S_GAP;
      end
      S_WSS: begin
        xip_req = 1'b1; xip_we = 1'b1;
        xip_adr = ADR_SS; xip_wdat = SPI_SS_FLASH;
        if (wb_ack) state_n = S_GAP;
      end
      S_WTX1: begin
        xip_req = 1'b1; xip_we = 1'b1;
        xip_adr = ADR_TX1; xip_wdat = {8'h03, flash_off};
        if (wb_ack) state_n = S_GAP;
      end
      S_WTX0: begin
        xip_req = 1'b1; xip_we = 1'b1;
        xip_adr = ADR_TX0; xip_wdat = 32'h0;
        if (wb_ack) state_n = S_GAP;
      end
      S_WCTRL: begin
        xip_req = 1'b1; xip_we = 1'b1;
        xip_adr = ADR_CTRL; xip_wdat = SPI_CTRL_CFG;
        if (wb_ack) state_n = S_GAP;
      end
      S_WGO: begin
        xip_req = 1'b1; xip_we = 1'b1;
        xip_adr = ADR_CTRL; xip_wdat = SPI_CTRL_GO;
        if (wb_ack) state_n = S_GAP;
      end
      S_POLL: begin
        xip_req = 1'b1; xip_we = 1'b0;
        xip_adr = ADR_CTRL;
        if (wb_ack) state_n = S_GAP;
      end
      S_RRX: begin
        xip_req = 1'b1; xip_we = 1'b0;
        xip_adr = ADR_TX0;
        if (wb_ack) state_n = S_GAP;
      end

      S_GAP: state_n = state_after_gap;

      S_RESP: if (in_psel && in_penable) state_n = S_IDLE;

      default: state_n = S_IDLE;
    endcase
  end

  always @(posedge clock) begin
    if (reset) begin
      state           <= S_IDLE;
      state_after_gap <= S_IDLE;
      flash_off       <= 24'h0;
      xip_rdata       <= 32'h0;
      seen_go         <= 1'b0;
    end else begin
      state <= state_n;

      if (state == S_IDLE && flash_rd_fire)
        flash_off <= in_paddr[23:0];

      // ack 当拍锁存 gap 返回目标
      if (wb_ack && state != S_GAP && state != S_IDLE && state != S_RESP)
        state_after_gap <= gap_dest;

      // 必须先读到过 GO=1，再等 GO=0，防止假完成
      if (state == S_IDLE || (state == S_WGO && wb_ack))
        seen_go <= 1'b0;
      else if (state == S_POLL && wb_ack && wb_dat_o[8])
        seen_go <= 1'b1;

      if (state == S_RRX && wb_ack)
        xip_rdata <= {wb_dat_o[7:0], wb_dat_o[15:8], wb_dat_o[23:16], wb_dat_o[31:24]};
    end

    if (!reset && flash_wr_req) begin
      $fwrite(32'h80000002, "XIP: write to flash 0x%08x rejected\n", in_paddr);
      $fatal;
    end
  end

  spi_top u0_spi_top (
    .wb_clk_i(clock),
    .wb_rst_i(reset),
    .wb_adr_i(wb_adr),
    .wb_dat_i(wb_dat_i),
    .wb_dat_o(wb_dat_o),
    .wb_sel_i(wb_sel),
    .wb_we_i (wb_we),
    .wb_stb_i(wb_stb),
    .wb_cyc_i(wb_cyc),
    .wb_ack_o(wb_ack),
    .wb_err_o(wb_err),
    .wb_int_o(spi_irq_out),
    .ss_pad_o(spi_ss),
    .sclk_pad_o(spi_sck),
    .mosi_pad_o(spi_mosi),
    .miso_pad_i(spi_miso)
  );

  wire _unused = &{1'b0, in_pprot, wb_err};

`endif // FAST_FLASH

endmodule
