`define GLBL

module prbs_gen (
input		ck,
input		rst,

input		i_req,

output	      	o_vld,
output [63:0]	o_res_lower,
output [63:0]	o_res_upper
);

   reg 	[127:0]	r_rand;
   always @(posedge ck) begin
      if (rst) begin
	 r_rand <= 128'h0123456789ABCDEFFEDCBA9876543210;
      end

      else if (i_req) begin
	 r_rand <= {r_rand[126:0], r_rand[127] ^ r_rand[125] ^ r_rand[100] ^ r_rand[98]};
      end

      else begin
	 r_rand <= r_rand;
      end
   end


   reg 		r_t2_vld;
   reg [127:0] 	r_t2_rnd;
   always @(posedge ck) begin
      r_t2_vld <= i_req;
      r_t2_rnd <= r_rand;
   end

   assign o_vld = r_t2_vld;
   assign o_res_lower = r_t2_rnd[63:0];
   assign o_res_upper = r_t2_rnd[127:64];

endmodule