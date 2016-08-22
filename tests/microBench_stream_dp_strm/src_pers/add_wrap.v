module add_wrap (
input		ck,
input		rst,

input	[63:0]	i_a, i_b,
input           i_vld,

output          o_rdy,
output	[63:0]	o_res,
output          o_vld
);

parameter ENTRY_DEPTH = 32; 
wire        ef_afull;
wire        ef_pop;
wire        ef_empty;
wire [63:0] ef_a, ef_b;
   
fifo #(
       .WIDTH(64+64),
       .DEPTH(ENTRY_DEPTH),
       .PIPE(1),
       .AFULLCNT(ENTRY_DEPTH - 2)
       ) entry_fifo (
		     .clk(ck),
		     .reset(rst),
		     .push(i_vld),
		     .din({i_a, i_b}),
		     .afull(ef_afull),
		     .oclk(ck),
		     .pop(ef_pop),
		     .dout({ef_a, ef_b}),
		     .empty(ef_empty)
		     );

wire        add_rdy;
wire [63:0] add_res;
wire        add_vld;

assign ef_pop = !ef_empty & add_rdy;

// Black box instantiation
add_64b_dp adder (.clk(ck), .a(ef_a), .b(ef_b), .operation_nd(ef_pop), .operation_rfd(add_rdy), .result(add_res), .rdy(add_vld));

assign o_rdy = !ef_afull;
assign o_res = add_res;
assign o_vld = add_vld;

endmodule