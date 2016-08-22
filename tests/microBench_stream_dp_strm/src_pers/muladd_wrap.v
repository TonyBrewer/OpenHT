module muladd_wrap (
input		ck,
input		rst,

input	[63:0]	i_a, i_b, i_c, //a + b * c
input           i_vld,

output          o_rdy,
output	[63:0]	o_res,
output          o_vld
);

parameter ENTRY_DEPTH = 32; 
wire        ef_afull;
wire        ef_pop;
wire        ef_empty;
wire [63:0] ef_a, ef_b, ef_c;
   
fifo #(
       .WIDTH(64+64+64),
       .DEPTH(ENTRY_DEPTH),
       .PIPE(1),
       .AFULLCNT(ENTRY_DEPTH - 2)
       ) entry_fifo (
		     .clk(ck),
		     .reset(rst),
		     .push(i_vld),
		     .din({i_a, i_b, i_c}),
		     .afull(ef_afull),
		     .oclk(ck),
		     .pop(ef_pop),
		     .dout({ef_a, ef_b, ef_c}),
		     .empty(ef_empty)
		     );

wire        mult_rdy;
wire [63:0] mult_res;
wire        mult_vld;

assign ef_pop = !ef_empty & mult_rdy;
   
// Black box instantiation
mul_64b_dp multiplier (.clk(ck), .a(ef_b), .b(ef_c), .operation_nd(ef_pop), .operation_rfd(mult_rdy), .result(mult_res), .rdy(mult_vld));


reg [63:0]  r_t2_a, r_t3_a, r_t4_a, r_t5_a, r_t6_a, r_t7_a;

// The following example uses a fixed-length pipeline,
// but could be used with any length or a variable length pipeline.
always @(posedge ck) begin
	if (ef_pop) begin
		r_t2_a <= ef_a;
	end else begin
		r_t2_a <= r_t2_a;
	end
 	r_t3_a <= r_t2_a;
 	r_t4_a <= r_t3_a;
	r_t5_a <= r_t4_a;
	r_t6_a <= r_t5_a;
	r_t7_a <= r_t6_a;
end


parameter MID_DEPTH = 32; 
wire        mf_afull;
wire        mf_pop;
wire        mf_empty;
wire [63:0] mf_a, mf_bc;
   
fifo #(
       .WIDTH(64+64),
       .DEPTH(MID_DEPTH),
       .PIPE(1),
       .AFULLCNT(MID_DEPTH - 2)
       ) mid_fifo (
		   .clk(ck),
		   .reset(rst),
		   .push(mult_vld),
		   .din({r_t7_a, mult_res}),
		   .afull(mf_afull),
		   .oclk(ck),
		   .pop(mf_pop),
		   .dout({mf_a, mf_bc}),
		   .empty(mf_empty)
		   );
   
wire        add_rdy;
wire [63:0] add_res;
wire        add_vld;

assign mf_pop = !mf_empty & add_rdy;
   
// Black box instantiation
add_64b_dp adder (.clk(ck), .a(mf_a), .b(mf_bc), .operation_nd(mf_pop), .operation_rfd(add_rdy), .result(add_res), .rdy(add_vld));

// Outputs
assign o_rdy  = !ef_afull;
assign o_res  = add_res;
assign o_vld  = add_vld;

endmodule
