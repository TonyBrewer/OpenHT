/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */

`timescale 1 ns / 1 ps

module bbox_wrap (
    //outputs
    vm, vt,
    //inputs
    intfClk1x, intfClk2x, reset,
    scalar, arc0,
    va, vb
    );

    input		intfClk1x;
    input		intfClk2x;
    input		reset;
    input [11:0]	arc0;
    input [63:0]	scalar;
    input [63:0]	va;
    input [63:0]        vb;
    output [63:0]	vt;		// result data
    output 	        vm;		// match (unused in both verilog and cpp)

    bbox_box_top bbox (
	.o_result(vt),			//
	.o_vm(vm),			// 
        .o_hit(),
        .o_done(),
        .o_error(),
        .clk2x(intfClk1x),			// bbox timed at 300 but we're running at 150
  	.clk1x(1'b0),			// rset of clocks unused
	.clkhx(1'b0),
	.bb_clk1x(1'b0),
	.bb_clk2x(1'b0),
        .i_reset(reset),
	.i_start(1'b1),			// start a instruction each clock
        .i_elemCnt(9'b00000001),	// one element for each start
	.i_scalar(scalar),
	.i_vrA(va),
	.i_vrB(vb),
        .i_arc0(arc0[10:0]),			// control
	.i_arc1(1'b0),			// unused
	.i_arc2(1'b0),			// unused
	.i_arc3(1'b0)			// unused
    );

endmodule
