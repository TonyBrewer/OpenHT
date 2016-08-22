module mult_wrap (
input		ck,

input	[63:0]	i_a, i_b,
input   [8:0]   i_htId,
input           i_vld,

output	[63:0]	o_res,
output  [8:0]   o_htId,
output          o_vld
);

// Wires & Registers
wire [63:0] c_t19_res;
wire [8:0]  c_t1_htId;
wire        c_t1_vld;

reg [8:0]   r_t2_htId, r_t3_htId, r_t4_htId, r_t5_htId, r_t6_htId, r_t7_htId, r_t8_htId, r_t9_htId, r_t10_htId, r_t11_htId, r_t12_htId, r_t13_htId, r_t14_htId, r_t15_htId, r_t16_htId, r_t17_htId, r_t18_htId, r_t19_htId;
reg 	    r_t2_vld,  r_t3_vld,  r_t4_vld,  r_t5_vld,  r_t6_vld,  r_t7_vld,  r_t8_vld,  r_t9_vld,  r_t10_vld,  r_t11_vld,  r_t12_vld,  r_t13_vld,  r_t14_vld,  r_t15_vld,  r_t16_vld,  r_t17_vld,  r_t18_vld,  r_t19_vld;

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

	r_t7_htId <= r_t6_htId;
	r_t7_vld  <= r_t6_vld;
   
 	r_t8_htId <= r_t7_htId;
	r_t8_vld  <= r_t7_vld;
   
 	r_t9_htId <= r_t8_htId;
	r_t9_vld  <= r_t8_vld;
   
	r_t10_htId <= r_t9_htId;
	r_t10_vld  <= r_t9_vld;
   
	r_t11_htId <= r_t10_htId;
	r_t11_vld  <= r_t10_vld;

	r_t12_htId <= r_t11_htId;
	r_t12_vld  <= r_t11_vld;
   
 	r_t13_htId <= r_t12_htId;
	r_t13_vld  <= r_t12_vld;
   
 	r_t14_htId <= r_t13_htId;
	r_t14_vld  <= r_t13_vld;
   
	r_t15_htId <= r_t14_htId;
	r_t15_vld  <= r_t14_vld;
   
	r_t16_htId <= r_t15_htId;
	r_t16_vld  <= r_t15_vld;
   
	r_t17_htId <= r_t16_htId;
	r_t17_vld  <= r_t16_vld;
   
	r_t18_htId <= r_t17_htId;
	r_t18_vld  <= r_t17_vld;
   
	r_t19_htId <= r_t18_htId;
	r_t19_vld  <= r_t18_vld;
end

// Black box instantiation
mul_64b_int multiplier (.clk(ck), .a(i_a), .b(i_b), .p(c_t19_res));
   

// Inputs
assign c_t1_htId = i_htId;
assign c_t1_vld  = i_vld;

// Outputs
assign o_res  = c_t19_res;
assign o_htId = r_t19_htId;
assign o_vld  = r_t19_vld;

endmodule