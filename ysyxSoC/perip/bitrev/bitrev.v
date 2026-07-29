module bitrev (
  input  sck,
  input  ss,
  input  mosi,
  output miso
);
  wire reset = ss;
  wire clk = sck;

  reg [7:0] data;
  reg [3:0] cnt;
  always @(posedge clk, posedge reset) begin
    if(reset) begin
      data <= 8'b0;
      cnt <= 0;
    end
    else begin
      cnt <= cnt + 1;
      if(cnt < 8) data <= {data[6:0], mosi};
      else data <= {1'b0, data[7:1]};
    end
  end

  assign miso = (reset || cnt < 8) ? 1'b1 : data[0];

endmodule
