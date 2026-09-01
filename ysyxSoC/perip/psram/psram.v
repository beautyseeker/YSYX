`timescale 1ns / 1ps

// IS66WVS4M8ALL PSRAM behavioral model
// - SPI: Enter QPI (35h) on SI; legacy 1-4-4 EBh/38h kept for reference
// - QPI: 4-4-4 EBh / 38h (cmd also on 4 lines), Exit QPI (F5h)
// Timing aligned to EF_PSRAM_CTRL.v after QPI conversion.
//
// Critical: read dout must be REGISTERED on posedge sck and held through the
// sck-high half-cycle. The controller samples din while sck==1.

module psram(
  input       sck,
  input       ce_n,
  inout [3:0] dio
);

  wire [3:0] din = dio;

  reg [3:0] dout;
  reg       dout_en;

  assign dio[0] = dout_en ? dout[0] : 1'bz;
  assign dio[1] = dout_en ? dout[1] : 1'bz;
  assign dio[2] = dout_en ? dout[2] : 1'bz;
  assign dio[3] = dout_en ? dout[3] : 1'bz;

  // 16MB byte array (24-bit address); SoC maps 4MB at 0x80000000
  reg [7:0] mem [0:(1<<24)-1];

  reg        qpi_mode; // 0: SPI, 1: QPI (sticky across CE# pulses)
  reg [7:0]  cnt;
  reg [7:0]  cmd;
  reg [23:0] addr;
  reg [31:0] rdata;
  reg [3:0]  wnib;

  wire is_read  = (cmd == 8'heb);
  wire is_write = (cmd == 8'h38);

  // QPI beat map (4-4-4): cmd 0-1, addr 2-7, (rd) dummy 8-13, data from 14
  //                        (wr) data from 8
  // SPI beat map (1-4-4):  cmd 0-7, addr 8-13, (rd) dummy 14-19, data from 20
  //                        (wr) data from 14
  wire [7:0] addr_lo   = qpi_mode ? 8'd2  : 8'd8;
  wire [7:0] addr_hi   = qpi_mode ? 8'd7  : 8'd13;
  wire [7:0] data_base   = qpi_mode ? 8'd8  : 8'd14; // write data / read dummy start
  wire [7:0] rd_data_lo  = data_base + 8'd6;         // first data beat (after 6 dummy)
  wire [7:0] rd_data_hi  = rd_data_lo + 8'd7;        // 4 bytes × 2 nibbles

  wire [7:0] rd_nib  = cnt - rd_data_lo;
  wire [1:0] rd_byte = rd_nib[2:1];
  wire       rd_hi   = ~rd_nib[0]; // even beat → high nibble first

  reg [7:0] rd_byte_val;
  always @(*) begin
    case (rd_byte)
      2'd0: rd_byte_val = rdata[7:0];
      2'd1: rd_byte_val = rdata[15:8];
      2'd2: rd_byte_val = rdata[23:16];
      2'd3: rd_byte_val = rdata[31:24];
      default: rd_byte_val = 8'h0;
    endcase
  end

  wire [7:0]  wtmp       = cnt - data_base;
  wire [1:0]  write_off  = wtmp[2:1];
  wire [23:0] write_addr = addr + {22'b0, write_off};

  // Mode is sticky; CE# only ends the current transaction
  always @(posedge sck or posedge ce_n) begin
    if (ce_n) begin
      // Enter/Exit QPI take effect when CE# rises after the command byte
      if (!qpi_mode && cmd == 8'h35)
        qpi_mode <= 1'b1;
      else if (qpi_mode && cmd == 8'hf5)
        qpi_mode <= 1'b0;

      cnt     <= 8'd0;
      cmd     <= 8'd0;
      addr    <= 24'd0;
      dout    <= 4'h0;
      dout_en <= 1'b0;
    end else begin
      // ---------- command ----------
      if (!qpi_mode) begin
        // SPI: 8 beats on dio[0]
        if (cnt <= 8'd7)
          cmd <= {cmd[6:0], din[0]};
      end else begin
        // QPI: 2 beats × 4 bit (first nibble → cmd[7:4])
        if (cnt <= 8'd1)
          cmd <= {cmd[3:0], din[3:0]};
      end

      // ---------- address (quad, both modes) ----------
      if (cnt >= addr_lo && cnt <= addr_hi)
        addr <= {addr[19:0], din[3:0]};

      // ---------- EBh: preload at start of dummy ----------
      if (cnt == data_base && is_read)
        rdata <= {mem[addr + 3], mem[addr + 2], mem[addr + 1], mem[addr]};

      // ---------- EBh: drive read data (registered for sck-high sample) ----------
      if (is_read && cnt >= data_base && cnt <= rd_data_hi) begin
        dout_en <= 1'b1;
        if (cnt >= rd_data_lo)
          dout <= rd_hi ? rd_byte_val[7:4] : rd_byte_val[3:0];
        else
          dout <= 4'h0; // dummy
      end else begin
        dout_en <= 1'b0;
        dout    <= 4'h0;
      end

      // ---------- 38h: write, high nibble then low ----------
      if (is_write && cnt >= data_base && cnt <= (data_base + 8'd7)) begin
        if (~cnt[0])
          wnib <= din[3:0];
        else
          mem[write_addr] <= {wnib, din[3:0]};
      end

      cnt <= cnt + 8'd1;
    end
  end

  // Cold reset of mode: no rst pin on particle; power-on default via initial
  initial qpi_mode = 1'b0;

endmodule
