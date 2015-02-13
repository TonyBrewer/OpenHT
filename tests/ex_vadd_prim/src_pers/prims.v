module add_5stage (
input		ck,

input	[63:0]	i_a, i_b,
input   [6:0]   i_htId,
input           i_vld,

output	[63:0]	o_res,
output  [6:0]   o_htId,
output          o_vld
);


// Wires & Registers
wire [63:0] c_t1_res;
wire [6:0]  c_t1_htId;
wire        c_t1_vld;
   
reg [63:0]  r_t2_res,  r_t3_res,  r_t4_res,  r_t5_res,  r_t6_res;
reg [6:0]   r_t2_htId, r_t3_htId, r_t4_htId, r_t5_htId, r_t6_htId;
reg 	    r_t2_vld,  r_t3_vld,  r_t4_vld,  r_t5_vld,  r_t6_vld;
   
// The following example uses a fixed-length pipeline,
// but could be used with any length or a variable length pipeline.
always @(posedge ck) begin
	r_t2_res  <= c_t1_res;
	r_t2_htId <= c_t1_htId;
	r_t2_vld  <= c_t1_vld;
   
	r_t3_res  <= r_t2_res;
 	r_t3_htId <= r_t2_htId;
	r_t3_vld  <= r_t2_vld;  
   
	r_t4_res  <= r_t3_res;
 	r_t4_htId <= r_t3_htId;
	r_t4_vld  <= r_t3_vld;  
   
	r_t5_res  <= r_t4_res;
	r_t5_htId <= r_t4_htId;
	r_t5_vld  <= r_t4_vld;
   
	r_t6_res  <= r_t5_res;
	r_t6_htId <= r_t5_htId;
	r_t6_vld  <= r_t5_vld;   
end

// Inputs
assign c_t1_res  = i_a + i_b;
assign c_t1_htId = i_htId;
assign c_t1_vld  = i_vld;

// Outputs
assign o_res  = r_t6_res;
assign o_htId = r_t6_htId;
assign o_vld  = r_t6_vld;

endmodule
