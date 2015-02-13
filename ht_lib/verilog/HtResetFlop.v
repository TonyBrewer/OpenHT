/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
module HtResetFlop (
input clk,
input i_reset,
output r_reset
);

`ifdef CNY_ALTERA
(* noprune = 1 *)
(* syn_preserve = 1 *)
reg r_rst;
always @(posedge clk) begin
    if (i_reset)
	r_rst <= 1'b1;
    else
	r_rst <= 1'b0;
end
assign r_reset = r_rst;
`else
FDSE rst (.C(clk), .S(i_reset), .CE(r_reset), .D(!r_reset), .Q(r_reset));
`endif

endmodule
