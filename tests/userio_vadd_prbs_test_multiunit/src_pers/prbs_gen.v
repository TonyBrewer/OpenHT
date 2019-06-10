`define GLBL

module prbs_gen (
input		ck,
input		rst,

input		i_req,

output	      	o_rdy,
output	      	o_vld,
output [63:0]	o_res_lower,
output [63:0]	o_res_upper
);
   
   reg 		r_init_active;
   reg [6:0]	r_init_cnt;
   reg 		r_init_done;
   always @(posedge ck) begin
      if (rst) begin
	 r_init_active <= 1'b0;
	 r_init_done   <= 1'b0;
      end

      else if (r_init_cnt == 7'h7f) begin
	 r_init_active <= 1'b0;
	 r_init_done   <= 1'b1;
      end

      else begin
	 r_init_active <= 1'b1;
	 r_init_done   <= 1'b0;
      end
   end
   
   always @(posedge ck) begin
      if (rst) begin
	 r_init_cnt <= 7'b0;
      end

      else if (r_init_active && r_init_cnt != 7'h7f) begin
	 r_init_cnt <= r_init_cnt + 1;
      end

      else begin
	 r_init_cnt <= r_init_cnt;
      end
   end

   reg 	[127:0]	r_seed;
   always @(posedge ck) begin
      if (rst) begin
	 r_seed <= 128'h0123456789ABCDEFFEDCBA9876543210;
      end

      else if (r_init_active) begin
	 r_seed <= {r_seed[126:0], r_seed[127]};
      end

      else begin
	 r_seed <= r_seed;
      end
   end

   assign o_rdy = r_init_done;
   
   wire	[127:0]	r_vld, r_res;
   genvar i;
   generate for (i=0; i<128; i=i+1) begin : g0

      wire 	  init_vld = r_init_active && (r_init_cnt == i);
      
      prbs_lfsr_tx prbs_lfsr_tx (
				 .ck(ck),
				 .i_init(init_vld),
				 .i_init_clk(r_init_active),
				 .i_seed(r_seed[127:96]),
				 .i_req(i_req),
				 .o_vld(r_vld[i]),
				 .o_res(r_res[i])
				 );

   end endgenerate

   // Cheat and just use valid from 0...they are always valid at the same time
   assign o_vld = r_vld[0];

   assign o_res_lower = r_res[63:0];
   assign o_res_upper = r_res[127:64];
   
endmodule


module prbs_lfsr_tx (
input		ck,
input		i_init,
input		i_init_clk,
input [31:0]	i_seed,

input		i_req,
output	      	o_vld,
output 		o_res
);

   reg 	[31:0]	r_lfsr;
   wire         c_xor_in = r_lfsr[31] ^ r_lfsr[21] ^ r_lfsr[1] ^ r_lfsr[0];
   always @(posedge ck) begin
      if (i_init) begin
	 r_lfsr <= i_seed;
      end

      else if (i_req | i_init_clk) begin
	 r_lfsr <= {r_lfsr[30:0], c_xor_in};
      end

      else begin
	 r_lfsr <= r_lfsr;
      end
   end


   reg 		r_t2_vld;
   reg  	r_t2_res;
   always @(posedge ck) begin
      r_t2_vld <= i_req;
      r_t2_res <= r_lfsr[31];
   end

   assign o_vld = r_t2_vld;
   assign o_res = r_t2_res;

endmodule