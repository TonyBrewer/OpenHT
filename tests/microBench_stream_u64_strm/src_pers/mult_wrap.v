module mult_wrap (
input		ck,

input	[63:0]	i_a, i_b,
input           i_vld,

output	[63:0]	o_res,
output          o_vld
);

// Wires & Registers
wire [63:0] r_t19_res;

reg 	    r_t2_vld, r_t3_vld, r_t4_vld, r_t5_vld, r_t6_vld, r_t7_vld, r_t8_vld, r_t9_vld, r_t10_vld, r_t11_vld, r_t12_vld, r_t13_vld, r_t14_vld, r_t15_vld, r_t16_vld, r_t17_vld, r_t18_vld, r_t19_vld;

// The following example uses a fixed-length pipeline,
// but could be used with any length or a variable length pipeline.
always @(posedge ck) begin
	r_t2_vld  <= i_vld;
	r_t3_vld  <= r_t2_vld;
	r_t4_vld  <= r_t3_vld;
	r_t5_vld  <= r_t4_vld;
	r_t6_vld  <= r_t5_vld;
	r_t7_vld  <= r_t6_vld;
	r_t8_vld  <= r_t7_vld;
	r_t9_vld  <= r_t8_vld;
	r_t10_vld <= r_t9_vld;
	r_t11_vld <= r_t10_vld;
	r_t12_vld <= r_t11_vld;
	r_t13_vld <= r_t12_vld;
	r_t14_vld <= r_t13_vld;
	r_t15_vld <= r_t14_vld;
	r_t16_vld <= r_t15_vld;
	r_t17_vld <= r_t16_vld;
	r_t18_vld <= r_t17_vld;
	r_t19_vld <= r_t18_vld;
end

// Black box instantiation
mul_64b_int multiplier (.clk(ck), .a(i_a), .b(i_b), .p(r_t19_res));


// Outputs
assign o_res = r_t19_res;
assign o_vld = r_t19_vld;

endmodule