module dual_port_ram
  #(parameter int DATA_WIDTH /*verilator public*/ = 32,
    parameter int ADDR_WIDTH /*verilator public*/ = 3
  ) (
    input wire       wr_clk_i,
    input wire       wr_en_i,
    input wire  [ADDR_WIDTH-1:0] wr_addr_i,
    input wire  [DATA_WIDTH-1:0] wr_data_i,
    input wire  [ADDR_WIDTH-1:0] rd_addr_i,
    output wire  [DATA_WIDTH-1:0] rd_data_o
);
    localparam int DEPTH = 1 << ADDR_WIDTH;
    logic [DATA_WIDTH-1:0] mem [DEPTH];

    // read-before-write is deliberate
    assign rd_data_o = mem[rd_addr_i];

    always_ff @(posedge wr_clk_i) begin
        if (wr_en_i) begin
`ifndef SYNTHESIS
            // Immediate assertion: fires if write address exceeds allocated range.
            assert (int'(wr_addr_i) < DEPTH)
                else $error("dual_port_ram: write address 0x%0h out of range (DEPTH=%0d)",
                            wr_addr_i, DEPTH);
`endif
            mem[wr_addr_i] <= wr_data_i;
        end
    end

`ifndef SYNTHESIS
    // Concurrent assertion: fires if write address exceeds allocated range.
    assert property (@(posedge wr_clk_i) wr_en_i |-> (int'(wr_addr_i) < DEPTH))
        else $error("dual_port_ram: concurrent assertion — write address 0x%0h out of range",
                    wr_addr_i);
`endif

endmodule
