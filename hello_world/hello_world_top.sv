module hello_world_top (
    input wire       clk,
    input wire [7:0] a,
    output reg [7:0] b
);

  reg [7:0] b;
  always_ff @(posedge clk) begin
    b <= a;
  end

endmodule
