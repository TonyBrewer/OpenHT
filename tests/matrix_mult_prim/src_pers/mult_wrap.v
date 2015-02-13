module mult_wrap (
input		ck,

input	[31:0]	i_a, i_b,
input   [3:0]   i_htId,
input           i_vld,

output	[63:0]	o_res,
output  [3:0]   o_htId,
output          o_vld
);

// Wires & Registers
wire [63:0] c_t6_res;
wire [3:0]  c_t1_htId;
wire        c_t1_vld;

reg [3:0]   r_t2_htId, r_t3_htId, r_t4_htId, r_t5_htId, r_t6_htId;
reg 	    r_t2_vld,  r_t3_vld,  r_t4_vld,  r_t5_vld,  r_t6_vld;

// The following example uses a fixed-length pipeline,
// but could be used with any length or a variable length pipeline.
always @(posedge ck) begin
	r_t2_htId <= c_t1_htId;
	r_t2_vld  <= c_t1_vld;
   
 	r_t3_htId <= r_t2_htId;
	r_t3_vld  <= r_t2_vld;  
   
 	r_t4_htId <= r_t3_htId;
	r_t4_vld  <= r_t3_vld;  
   
	r_t5_htId <= r_t4_htId;
	r_t5_vld  <= r_t4_vld;
   
	r_t6_htId <= r_t5_htId;
	r_t6_vld  <= r_t5_vld;   
end

// Black box instantiation
multiplier multiplier (.clk(ck), .a(i_a), .b(i_b), .p(c_t6_res));
   

// Inputs
assign c_t1_htId = i_htId;
assign c_t1_vld  = i_vld;

// Outputs
assign o_res  = c_t6_res;
assign o_htId = r_t6_htId;
assign o_vld  = r_t6_vld;

endmodule