module add_5stage (
input		ck,
input	[63:0]	t1_a, t1_b,
output	[63:0]	t6_res
);

wire [63:0] c_t1_res;
reg [63:0] r_t2_res, r_t3_res, r_t4_res, r_t5_res, r_t6_res;
always @(posedge ck) begin
	r_t2_res <= c_t1_res;
	r_t3_res <= r_t2_res;
	r_t4_res <= r_t3_res;
	r_t5_res <= r_t4_res;
	r_t6_res <= r_t5_res;
end
assign c_t1_res = t1_a + t1_b;
assign t6_res = r_t6_res;

endmodule
