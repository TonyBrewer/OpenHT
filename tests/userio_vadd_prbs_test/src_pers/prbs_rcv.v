`define GLBL

module prbs_rcv (
input		ck,
input		rst,

input		i_req,
input [63:0]	i_din_lower,
input [63:0]	i_din_upper,

output	      	o_err,
output	      	o_vld
);

   
   wire [127:0] i_din = {i_din_upper, i_din_lower};
   wire [127:0] r_lck, r_err, r_vld;
   genvar i;
   generate for (i=0; i<128; i=i+1) begin : g0
      
      prbs_lfsr_rx prbs_lfsr_rx (
				 .ck(ck),
				 .rst(rst),
				 .i_req(i_req),
				 .i_din(i_din[i]),
				 .o_lck(r_lck[i]),
				 .o_err(r_err[i]),
				 .o_vld(r_vld[i])
				 );

   end endgenerate

   // Wait for Lock
   reg 		r_lock_seen;
   always @(posedge ck) begin
      if (rst) begin
	 r_lock_seen <= 1'b0;
      end

      else if (&r_lck) begin
	 r_lock_seen <= 1'b1;
      end

      else begin
	 r_lock_seen <= r_lock_seen;
      end
   end

   // Output error if we see it after locking...
   assign o_err = (r_lock_seen) ? |r_err : 1'b0;
   
   // Cheat and just use valid from 0...they are always valid at the same time
   assign o_vld = r_vld[0];
   
endmodule


module prbs_lfsr_rx (
input		ck,
input		rst,

input		i_req,
input		i_din,
output	      	o_lck,
output	      	o_err,
output	      	o_vld
);

   reg 	[31:0]	r_lfsr;
   wire         c_xor_in = r_lfsr[31] ^ r_lfsr[21] ^ r_lfsr[1] ^ r_lfsr[0];
   always @(posedge ck) begin
      if (i_req) begin
	 if (o_lck) begin
	    r_lfsr <= {r_lfsr[30:0], c_xor_in};
	 end else begin
	    r_lfsr <= {r_lfsr[30:0], i_din};
	 end
      end

      else begin
	 r_lfsr <= r_lfsr;
      end
   end

   
   wire 	c_input_err = i_req ? (c_xor_in != i_din) : 1'b0;
   reg 		r_input_err;
   reg [5:0] 	r_lock_cnt;
   always @(posedge ck) begin
      if (rst) begin
	 r_lock_cnt <= 6'b0;
      end

      else if (i_req) begin
	 if (c_input_err) begin
	    r_lock_cnt <= 6'b0;
	 end

	 else begin
	    if (r_lock_cnt != 6'd32) begin
	       r_lock_cnt <= r_lock_cnt + 6'd1;
	    end else begin
	       r_lock_cnt <= r_lock_cnt;
	    end
	 end
      end

      else begin
	 r_lock_cnt <= r_lock_cnt;
      end

      r_input_err <= c_input_err;
   end

   assign o_lck = r_lock_cnt[5];
   assign o_err = r_input_err;


   reg 		r_t2_vld;
   always @(posedge ck) begin
      r_t2_vld <= i_req;
   end

   assign o_vld = r_t2_vld;

endmodule