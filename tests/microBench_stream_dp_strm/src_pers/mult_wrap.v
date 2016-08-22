module mult_wrap (
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

wire        mult_rdy;
wire [63:0] mult_res;
wire        mult_vld;

assign ef_pop = !ef_empty & mult_rdy;

// Black box instantiation
mul_64b_dp multiplier (.clk(ck), .a(ef_a), .b(ef_b), .operation_nd(ef_pop), .operation_rfd(mult_rdy), .result(mult_res), .rdy(mult_vld));

assign o_rdy = !ef_afull;
assign o_res = mult_res;
assign o_vld = mult_vld;
   
endmodule