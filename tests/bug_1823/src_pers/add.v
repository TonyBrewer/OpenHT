module addC (
  input        ck,
  input  [31:0] i_a,
  input  [31:0] i_b,
  output [31:0] o_res
);

   wire [31:0] 	    c_t1_res;
   reg [31:0] 	    r_t2_res;

   assign o_res = r_t2_res;
   
   always @(posedge ck) begin
      r_t2_res <= c_t1_res;  
   end
   
   assign c_t1_res = i_a + i_b;

endmodule

module addP (
  input  [31:0] i_a,
  input  [31:0] i_b,
  output [31:0] o_res
);

   assign o_res = i_a + i_b;

endmodule

module addS (
  input  [63:0] i_in,
  output [31:0] o_res
);

   assign o_res[31:0] = i_in[63:32] + i_in[31:0];

endmodule
