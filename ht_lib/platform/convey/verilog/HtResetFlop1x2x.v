/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
module HtResetFlop1x2x (
input clkhx,
input clk1x,
input clk2x,
input i_reset,
output r_reset1x,
output r_reset2x
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

reg r_rst1x;
always @(posedge clk1x) begin
    r_rst1x <= r_rsthx;
end
assign r_reset1x = r_rst1x;

reg r_rst2x;
always @(posedge clk2x) begin
    r_rst2x <= r_rsthx;
end
assign r_reset2x = r_rst2x;

`else //!CNY_PLATFORM_TYPE2

`ifdef CNY_ALTERA
(* noprune = 1 *)
(* syn_preserve = 1 *)
reg r_rst1x;
always @(posedge clk1x) begin
    if (i_reset)
        r_rst1x <= 1'b1;
    else
        r_rst1x <= 1'b0;
end
`else //!CNY_ALTERA
wire r_rst1x;
FDSE rst (.C(clk1x), .S(i_reset), .CE(r_rst1x), .D(!r_rst1x), .Q(r_rst1x));
`endif //!CNY_ALTERA

assign r_reset1x = r_rst1x;

reg r_rst2x;
always @(posedge clk2x) begin
    r_rst2x <= r_rst1x;
end
assign r_reset2x = r_rst2x;

`endif //!CNY_PLATFORM_TYPE2

endmodule
