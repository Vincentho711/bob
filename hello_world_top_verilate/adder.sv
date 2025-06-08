module adder(
  input wire clk_i,
  input wire [7:0] a_i,
  input wire [7:0] b_i,
  output reg [8:0] c_o
);

always_ff @(posedge clk_i) begin
  c_o <= a_i + b_i;
end

endmodule  
