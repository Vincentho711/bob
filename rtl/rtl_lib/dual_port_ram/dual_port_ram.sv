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
            // Immediate assertion: fires if write address exceeds allocated range.
            assert (int'(wr_addr_i) < DEPTH)
                else $error("dual_port_ram: write address 0x%0h out of range (DEPTH=%0d)",
                            wr_addr_i, DEPTH);
            // INTENTIONAL TEST FAILURE — remove after verifying campaign runner catches it.
            // Fires the first time address 2 is written.
            // assert (wr_addr_i != 3'd2)
            //     else $fatal(1, "dual_port_ram: [immediate] forbidden write to addr 2 (wr_data=0x%0h)",
            //                 wr_data_i);
            mem[wr_addr_i] <= wr_data_i;
        end
    end

    // Concurrent assertion: fires if write address exceeds allocated range.
    assert property (@(posedge wr_clk_i) wr_en_i |-> (int'(wr_addr_i) < DEPTH))
        else $error("dual_port_ram: concurrent assertion — write address 0x%0h out of range",
                    wr_addr_i);

    // INTENTIONAL TEST FAILURE — remove after verifying campaign runner catches it.
    // Concurrent assertion: fires the first time address 3 is written with wr_en_i high.
    // assert property (@(posedge wr_clk_i) wr_en_i |-> (wr_addr_i != 3'd3))
    //     else $fatal(1, "dual_port_ram: [concurrent] forbidden write to addr 3 (wr_addr=0x%0h)",
    //                 wr_addr_i);

endmodule
