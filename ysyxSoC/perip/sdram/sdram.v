`timescale 1ns / 1ps

// MT48LC16M16A2 behavioral model (ysyx SoC)
// 4 bank x 8192 row x 512 col x 16bit
// Mode: CL / BL from LOAD_MODE (controller uses CL=2, BL=2)
// PRECHARGE / REFRESH: close-row bookkeeping / NOP
// Not intended for FPGA synthesis — simulation particle only.

module sdram(
  input        clk,
  input        cke,
  input        cs,
  input        ras,
  input        cas,
  input        we,
  input [12:0] a,
  input [ 1:0] ba,
  input [ 1:0] dqm,
  inout [15:0] dq
);

  // ---------------- memory array (halfword) ----------------
  reg [15:0] mem [0:(1<<24)-1];

  // ---------------- dq drive ----------------
  reg        dout_en;
  reg [15:0] dout;
  assign dq = dout_en ? dout : 16'hzzzz;

  // ---------------- commands {cs,ras,cas,we} ----------------
  wire [3:0] cmd = {cs, ras, cas, we};
  localparam CMD_NOP       = 4'b0111;
  localparam CMD_ACTIVE    = 4'b0011;
  localparam CMD_READ      = 4'b0101;
  localparam CMD_WRITE     = 4'b0100;
  localparam CMD_PRECHARGE = 4'b0010;
  localparam CMD_REFRESH   = 4'b0001;
  localparam CMD_LOAD_MODE = 4'b0000;

  // ---------------- mode / bank / burst ----------------
  reg [2:0] cas_latency; // a[6:4], expect 3'b010
  reg [2:0] burst_len;   // a[2:0], expect 3'b001 → length 2

  reg        bank_open [0:3];
  reg [12:0] bank_row  [0:3];

  // write burst (2nd beat usually arrives with NOP)
  reg        wr_burst;
  reg [1:0]  wr_bank;
  reg [12:0] wr_row;
  reg [8:0]  wr_col;

  reg [2:0]  rd_wait;
  reg [1:0]  rd_nleft;
  reg [1:0]  rd_bank;
  reg [12:0] rd_row;
  reg [8:0]  rd_col;

  integer i;
  initial begin
    dout_en     = 1'b0;
    dout        = 16'h0;
    cas_latency = 3'b010;
    burst_len   = 3'b001;
    wr_burst    = 1'b0;
    rd_wait     = 3'd0;
    rd_nleft    = 2'd0;
    for (i = 0; i < 4; i = i + 1) begin
      bank_open[i] = 1'b0;
      bank_row[i]  = 13'h0;
    end
  end

  always @(posedge clk) begin
    if (!cke) begin
      dout_en  <= 1'b0;
      wr_burst <= 1'b0;
      rd_wait  <= 3'd0;
      rd_nleft <= 2'd0;
    end else begin
      // default: release bus (read path may override below)
      dout_en <= 1'b0;

      // ---------- write burst beat 1 (often cmd == NOP) ----------
      if (wr_burst) begin
        if (!dqm[0]) mem[{wr_bank, wr_row, wr_col + 9'd1}][7:0]  <= dq[7:0];
        if (!dqm[1]) mem[{wr_bank, wr_row, wr_col + 9'd1}][15:8] <= dq[15:8];
        wr_burst <= 1'b0;
      end

      // ---------- read data output ----------
      if (rd_wait != 3'd0) begin
        if (rd_wait == 3'd1 && rd_nleft != 2'd0) begin
          dout     <= mem[{rd_bank, rd_row, rd_col}];
          dout_en  <= 1'b1;
          rd_col   <= rd_col + 9'd1;
          rd_nleft <= rd_nleft - 2'd1;
        end
        rd_wait <= rd_wait - 3'd1;
      end
      else if (rd_nleft != 2'd0) begin
        dout     <= mem[{rd_bank, rd_row, rd_col}];
        dout_en  <= 1'b1;
        rd_col   <= rd_col + 9'd1;
        rd_nleft <= rd_nleft - 2'd1;
      end

      // ---------- command decode ----------
      case (cmd)
        CMD_LOAD_MODE: begin
          burst_len   <= a[2:0];
          cas_latency <= a[6:4];
        end

        CMD_ACTIVE: begin
          bank_open[ba] <= 1'b1;
          bank_row[ba]  <= a[12:0];
        end

        CMD_PRECHARGE: begin
          if (a[10]) begin
            bank_open[0] <= 1'b0;
            bank_open[1] <= 1'b0;
            bank_open[2] <= 1'b0;
            bank_open[3] <= 1'b0;
          end else begin
            bank_open[ba] <= 1'b0;
          end
        end

        CMD_REFRESH: begin
          // nop for this lab
        end

        CMD_WRITE: begin
          if (bank_open[ba]) begin
            if (!dqm[0]) mem[{ba, bank_row[ba], a[8:0]}][7:0]  <= dq[7:0];
            if (!dqm[1]) mem[{ba, bank_row[ba], a[8:0]}][15:8] <= dq[15:8];
            if (burst_len == 3'b001) begin
              wr_burst <= 1'b1;
              wr_bank  <= ba;
              wr_row   <= bank_row[ba];
              wr_col   <= a[8:0];
            end
          end
        end

        CMD_READ: begin
          if (bank_open[ba]) begin
            rd_bank  <= ba;
            rd_row   <= bank_row[ba];
            rd_col   <= a[8:0];
            rd_wait  <= 3'd1;
            rd_nleft <= (burst_len == 3'b001) ? 2'd2 : 2'd1;
          end
        end

        default: ;
      endcase
    end
  end

endmodule
