module ps2_top_apb(
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

  input         ps2_clk,
  input         ps2_data
);

  wire clk = clock;
  // ysyxSoC：reset 高有效
  wire rst_n = ~reset;

  reg [9:0] buffer;
  reg [7:0] fifo [0:7];
  reg [2:0] w_ptr, r_ptr;
  reg [3:0] count;
  reg [2:0] ps2_clk_sync;
  reg       ready, overflow;

  // APB 读完成一拍 = 原参考设计的 nextdata_n 脉冲（读走一个字节）
  wire apb_rd = in_psel && in_penable && !in_pwrite && in_pready
                && (in_paddr[3:0] == 4'h0);

  always @(posedge clk) begin
    ps2_clk_sync <= {ps2_clk_sync[1:0], ps2_clk};
  end
  wire sampling = ps2_clk_sync[2] & ~ps2_clk_sync[1];

  always @(posedge clk) begin
    if (!rst_n) begin
      count    <= 4'd0;
      w_ptr    <= 3'd0;
      r_ptr    <= 3'd0;
      overflow <= 1'b0;
      ready    <= 1'b0;
    end else begin
      // 软件读走当前字节后再弹 FIFO（不要在空闲时自动 pop）
      if (ready && apb_rd) begin
        r_ptr <= r_ptr + 3'd1;
        if (w_ptr == (r_ptr + 3'd1))
          ready <= 1'b0;
      end

      if (sampling) begin
        if (count == 4'd10) begin
          if ((buffer[0] == 1'b0) && ps2_data && (^buffer[9:1])) begin
            fifo[w_ptr] <= buffer[8:1];
            w_ptr       <= w_ptr + 3'd1;
            ready       <= 1'b1;
            overflow    <= overflow | (r_ptr == (w_ptr + 3'd1));
          end
          count <= 4'd0;
        end else begin
          buffer[count] <= ps2_data;
          count         <= count + 4'd1;
        end
      end
    end
  end

  // 实验约定：有数据返回扫描码，否则 0；扫描码→AM 键码交给软件
  assign in_pready  = 1'b1;
  assign in_prdata  = ready ? {24'h0, fifo[r_ptr]} : 32'h0;
  assign in_pslverr = 1'b0;

  // ---------- debug：快照变化才打印 ----------
//   localparam PS2_DBG = 1;

//   wire [37:0] dbg_snap = {
//     ready, 2'b0,
//     w_ptr, r_ptr,
//     buffer[8:1], fifo[r_ptr],
//     in_prdata[7:0],
//     count, overflow
//   };

//   reg [37:0] dbg_snap_prev;
//   reg        dbg_snap_init;

//   always @(posedge clk) begin
//     if (!rst_n) begin
//       dbg_snap_prev <= 38'h0;
//       dbg_snap_init <= 1'b0;
//     end else if (PS2_DBG) begin
//       if (!dbg_snap_init || (dbg_snap != dbg_snap_prev)) begin
//         $display("[PS2] t=%0t ready=%0d prdata=%02x fifo[r]=%02x buf=%02x count=%0d w/r=%0d/%0d apb_rd=%0d",
//                  $time, ready, in_prdata[7:0], fifo[r_ptr], buffer[8:1],
//                  count, w_ptr, r_ptr, apb_rd);
//         dbg_snap_prev <= dbg_snap;
//         dbg_snap_init <= 1'b1;
//       end
//     end
//   end

  wire _unused = |in_pprot | |in_pstrb | |in_pwdata | |in_paddr[31:4] | overflow;

endmodule
