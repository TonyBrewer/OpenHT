module muladd_wrap (
input		ck,

input	[63:0]	i_a, i_b, i_c,
input   [6:0]   i_htId,
input           i_vld,

output	[63:0]	o_res,
output  [6:0]   o_htId,
output          o_vld
);

// Wires & Registers
wire [63:0] c_t19_res;

reg [6:0]   r_t2_htId, r_t3_htId, r_t4_htId, r_t5_htId, r_t6_htId, r_t7_htId, r_t8_htId, r_t9_htId, r_t10_htId, r_t11_htId, r_t12_htId, r_t13_htId, r_t14_htId, r_t15_htId, r_t16_htId, r_t17_htId, r_t18_htId, r_t19_htId, r_t20_htId;
reg 	    r_t2_vld,  r_t3_vld,  r_t4_vld,  r_t5_vld,  r_t6_vld,  r_t7_vld,  r_t8_vld,  r_t9_vld,  r_t10_vld,  r_t11_vld,  r_t12_vld,  r_t13_vld,  r_t14_vld,  r_t15_vld,  r_t16_vld,  r_t17_vld,  r_t18_vld,  r_t19_vld,  r_t20_vld;
reg [63:0]  r_t2_a,    r_t3_a,    r_t4_a,    r_t5_a,    r_t6_a,    r_t7_a,    r_t8_a,    r_t9_a,    r_t10_a,    r_t11_a,    r_t12_a,    r_t13_a,    r_t14_a,    r_t15_a,    r_t16_a,    r_t17_a,    r_t18_a,    r_t19_a,    r_t20_a;

// The following example uses a fixed-length pipeline,
// but could be used with any length or a variable length pipeline.
always @(posedge ck) begin
	r_t2_htId <= i_htId;
	r_t2_vld  <= i_vld;
	r_t2_a    <= i_a;
   
 	r_t3_htId <= r_t2_htId;
	r_t3_vld  <= r_t2_vld;
	r_t3_a    <= r_t2_a;
   
 	r_t4_htId <= r_t3_htId;
	r_t4_vld  <= r_t3_vld;
	r_t4_a    <= r_t3_a;
   
	r_t5_htId <= r_t4_htId;
	r_t5_vld  <= r_t4_vld;
	r_t5_a    <= r_t4_a;
   
	r_t6_htId <= r_t5_htId;
	r_t6_vld  <= r_t5_vld;
	r_t6_a    <= r_t5_a;

	r_t7_htId <= r_t6_htId;
	r_t7_vld  <= r_t6_vld;
	r_t7_a    <= r_t6_a;
   
 	r_t8_htId <= r_t7_htId;
	r_t8_vld  <= r_t7_vld;
	r_t8_a    <= r_t7_a;
   
 	r_t9_htId <= r_t8_htId;
	r_t9_vld  <= r_t8_vld;
	r_t9_a    <= r_t8_a;
   
	r_t10_htId <= r_t9_htId;
	r_t10_vld  <= r_t9_vld;
	r_t10_a    <= r_t9_a;
   
	r_t11_htId <= r_t10_htId;
	r_t11_vld  <= r_t10_vld;
	r_t11_a    <= r_t10_a;

	r_t12_htId <= r_t11_htId;
	r_t12_vld  <= r_t11_vld;
	r_t12_a    <= r_t11_a;
   
 	r_t13_htId <= r_t12_htId;
	r_t13_vld  <= r_t12_vld;
	r_t13_a    <= r_t12_a;
   
 	r_t14_htId <= r_t13_htId;
	r_t14_vld  <= r_t13_vld;
	r_t14_a    <= r_t13_a;
   
	r_t15_htId <= r_t14_htId;
	r_t15_vld  <= r_t14_vld;
	r_t15_a    <= r_t14_a;
   
	r_t16_htId <= r_t15_htId;
	r_t16_vld  <= r_t15_vld;
	r_t16_a    <= r_t15_a;
   
	r_t17_htId <= r_t16_htId;
	r_t17_vld  <= r_t16_vld;
	r_t17_a    <= r_t16_a;
   
	r_t18_htId <= r_t17_htId;
	r_t18_vld  <= r_t17_vld;
	r_t18_a    <= r_t17_a;
   
	r_t19_htId <= r_t18_htId;
	r_t19_vld  <= r_t18_vld;
	r_t19_a    <= r_t18_a;
   
	r_t20_htId <= r_t19_htId;
	r_t20_vld  <= r_t19_vld;
	r_t20_a    <= r_t19_a + c_t19_res;
end

// Black box instantiation
mul_64b_int multiplier (.clk(ck), .a(i_b), .b(i_c), .p(c_t19_res));


// Outputs
assign o_res  = r_t20_a;
assign o_htId = r_t20_htId;
assign o_vld  = r_t20_vld;

endmodule