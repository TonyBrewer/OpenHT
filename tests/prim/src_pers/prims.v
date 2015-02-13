module add8_prm (
input	[7:0]	a, b,
input		cyin,
output	[7:0]	c,
output		cyout
);

wire [8:0] t = a + b + cyin;
assign c = t[7:0];
assign cyout = t[8];

endmodule


module add16_prm (
input		intfClk1x,
input	[15:0]	a, b,
output	[15:0]	c
);

reg [15:0] r_a, r_b;
always @(posedge intfClk1x) begin
	r_a <= a;
	r_b <= b;
end
assign c = r_a + r_b;

endmodule
