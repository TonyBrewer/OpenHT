/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
module HtResetFlop1x (
input clkhx,
input clk1x,
input i_reset,
output r_reset
);

`ifdef CNY_PLATFORM_TYPE2

`ifdef CNY_ALTERA
(* noprune = 1 *)
(* syn_preserve = 1 *)
reg r_rsthx;
always @(posedge clkhx) begin
    if (i_reset)
	r_rsthx <= 1'b1;
    else
	r_rsthx <= 1'b0;
end
`else //!CNY_ALTERA
wire r_rsthx;
FDSE rst (.C(clkhx), .S(i_reset), .CE(r_rsthx), .D(!r_rsthx), .Q(r_rsthx));
`endif //!CNY_ALTERA

reg r_rst;
always @(posedge clk1x) begin
    r_rst <= r_rsthx;
end
assign r_reset = r_rst;

`else //!CNY_PLATFORM_TYPE2

`ifdef CNY_ALTERA
(* noprune = 1 *)
(* syn_preserve = 1 *)
reg r_rst;
always @(posedge clk1x) begin
    if (i_reset)
        r_rst <= 1'b1;
    else
        r_rst <= 1'b0;
end
assign r_reset = r_rst;
`else //!CNY_ALTERA
FDSE rst (.C(clk1x), .S(i_reset), .CE(r_reset), .D(!r_reset), .Q(r_reset));
`endif //!CNY_ALTERA

`endif //!CNY_PLATFORM_TYPE2

endmodule
