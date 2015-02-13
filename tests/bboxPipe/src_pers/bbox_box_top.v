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

// leda B_3413 off Task call in a combinational block
// leda VER_2_1_2_4 off Task should not be used
// leda W484 off Possible loss of carry/borrow in addition/subtraction
module bbox_box_top (/*AUTOARG*/
   // Outputs
   o_result, o_vm, o_done, o_hit, o_error,
   // Inputs
   clk2x, clk1x, clkhx, bb_clk1x, bb_clk2x, i_reset, i_start,
   i_scalar, i_elemCnt, i_arc0, i_arc1, i_arc2, i_arc3, i_vrA, i_vrB
   ) ;

`include "bbox_user.vh"
   
   /* ----------        port declarations     ---------- */
   // clk2x is used for all other inputs and outputs 
   input 	clk2x;    // 300Mhz input clock, 
   input 	clk1x;    // 150Mhz input clock synchronous with clk2x
   input        clkhx;    // 75Mhz input clock, synchronous with clk2x
   input 	bb_clk1x; // dedicated BB clock that is asynchronous with clk1x
                          //   bb_clk1x is 1/2 the frequency of bb_clk2x
   input 	bb_clk2x; // dedicated BB clock that is asynchronous with clk1x
                          //   bb_clk2x is 2x the frequency of bb_clk1x

/*********** all following inputs/outputs are clocked by clk2x *******/
   input 	i_reset;  // input reset signal, asserted immediatly 
                          //   after the personality is loaded into the FPGA

/*********** input control signals (valid when start is asserted) *******/
   input 	i_start;  // a black box instruction is starting this cycle

   input [63:0] i_scalar; // 64 bit value from the instruction
   
   input [8:0] 	i_elemCnt;// number of elements to be processed for the 
                          //   coincident start signal
   
   input [BBUSER_ARC0MSB:0] i_arc0; // control of BBox
                                // i_arc0[0] - single instruction op
                                // i_arc0[1] - start a LD op to Unit#0 AMS
                                // i_arc0[2] - start a BB op from Unit#0 AMS
                                // i_arc0[3] - start a ST op from Unit#0 AMS
                                // i_arc0[4] - start a LD op to Unit#1 AMS
                                // i_arc0[5] - start a BB op from Unit#1 AMS
                                // i_arc0[6] - start a ST op from Unit#1 AMS
                                // i_arc0[7+] - user defined
   input [BBUSER_ARC1MSB:0] i_arc1; // 1 to 64 bits of user control for BBox
   input [BBUSER_ARC2MSB:0] i_arc2; // 1 to 64 bits of user control for BBox
   input [BBUSER_ARC3MSB:0] i_arc3; // 1 to 64 bits of user control for BBox

/******** input data elements (valid for elemCnt cycles starting coincident with i_start ****/
   input [63:0] i_vrA;
   input [63:0] i_vrB;
       
/******** output data elements (valid for elemCnt cycles  ****/
   output [63:0] o_result; // data results (valid for elemCnt cycles,
                           //   starting BBUSER_PIPE_LEN cycles after start
   
   output        o_vm;     // current element was a match, 
                           // this signal gets written to VMt if specified by the instruction
                           //   coincident with o_Result

/******** current multi-stage operation is done  ****/
   output 	 o_done;   // done with current operation in x? cycles, 
                           //   used in multi-stage mode

/******** these 2 signals are sent back to the controller to cause an exception *******/
   output 	 o_hit;    // black box found something it was looking for, pull an exception
   
   output        o_error;  // the BB has detected some sort of internal error, pull an exception
   
   /* ----------         include files        ---------- */
   /* ----------          wires & regs        ---------- */

   
   reg 		 r_reset;
   reg 		 r_t1_start;

   reg [63:0] 	 r_t1_scalar, r_t2_scalar;
   reg [8:0] 	 r_t1_elemCnt;
   reg [BBUSER_ARC0MSB:0] r_t1_arc0;
   reg [BBUSER_ARC0MSB:0] r_t2_arc0;
   reg [BBUSER_ARC1MSB:0] r_t1_arc1;// not needed if no bits in arc1 are defined
   reg [BBUSER_ARC2MSB:0] r_t1_arc2;// not needed if no bits in arc2 are defined
   reg [BBUSER_ARC3MSB:0] r_t1_arc3;// not needed if no bits in arc3 are defined

   reg [63:0] 	 r_t1_vrA, r_t1_vrB;
   reg [63:0] 	 r_t2_vrA, r_t2_vrB;

   wire [8:0] 	 c_t1_cnt;
   reg 	[8:0]	 r_t2_cnt;

   wire 	 c_t1_active;
   reg 		 r_t2_active;
   reg 		 r_t3_active;
   reg 		 r_t4_active;
   
   wire 	 c_t1_done;
   reg 		 r_t2_done;

   wire [63:0] 	 c_t2_inA;
   reg [63:0] 	 c_t2_inB, r_t3_inA, r_t3_inB;
   reg [5:0] 	 r_t4_posr, r_t4_posl;
   reg [11:0] 	 r_t5_wordr, r_t5_wordl;
   reg [63:0] 	 r_t6_datas;
   reg 		 r_t3_BBop;
   reg 		 r_t4_BBop;
   reg 		 r_t5_BBop;
   
   /* ----------   centrifuge wires and regs  ---------- */
   
   reg [63:0]  r_t4_inA,    r_t4_inB;
   reg [63:0]  r_t7_datas,  c_t6_datasr, c_t6_datasl;
   // spin right
   reg [3:0]   r_t4_balr,   c_t3_balr;
   reg [3:0]   r_t4_mixedr, c_t3_mixedr;
   reg [11:0]  r_t5_datar,  c_t4_datar;
   reg [11:0]  r_t6_wordr;
   reg [7:0]   r_t6_byter;
   reg         r_t6_BBop;
   reg [5:0]   r_t5_posr,   c_t4_posr, r_t6_posr_d;
   reg         c_t4_pos_ovr;
   reg 	       r_t5_bwer,   r_t6_bwer_d;
   // spin left
   reg [3:0]   r_t4_ball,   c_t3_ball;
   reg [3:0]   r_t4_mixedl, c_t3_mixedl;
   reg [11:0]  r_t5_datal,  c_t4_datal;
   reg [11:0]  r_t6_wordl;
   reg [7:0]   r_t6_bytel;
   reg [5:0]   r_t5_posl,   c_t4_posl, r_t6_posl_d;
   reg         c_t4_pos_ovl;
   reg 	       r_t5_bwel,   r_t6_bwel_d;
   reg [63:0]  r_t5_inA,    r_t5_inB;
   reg [63:0]  r_t8_datas,  c_t7_datasr, c_t7_datasl;
   // spin right
   reg [3:0]   r_t5_balr,   c_t4_balr;
   reg [3:0]   r_t5_mixedr, c_t4_mixedr;
   reg [11:0]  r_t6_datar,  c_t5_datar;
   reg [11:0]  r_t7_wordr;
   reg [7:0]   r_t7_byter;
   reg         r_t7_BBop;
   reg [5:0]   r_t6_posr,   c_t5_posr, r_t7_posr_d;
   reg         c_t5_pos_ovr;
   reg 	       r_t6_bwer,   r_t7_bwer_d;
   // spin left
   reg [3:0]   r_t5_ball,   c_t4_ball;
   reg [3:0]   r_t5_mixedl, c_t4_mixedl;
   reg [11:0]  r_t6_datal,  c_t5_datal;
   reg [11:0]  r_t7_wordl;
   reg [7:0]   r_t7_bytel;
   reg [5:0]   r_t6_posl,   c_t5_posl, r_t7_posl_d;
   reg         c_t5_pos_ovl;
   reg 	       r_t6_bwel,   r_t7_bwel_d;
   reg [63:0]  r_t6_inA,    r_t6_inB;
   reg [63:0]  r_t9_datas,  c_t8_datasr, c_t8_datasl;
   // spin right
   reg [3:0]   r_t6_balr,   c_t5_balr;
   reg [3:0]   r_t6_mixedr, c_t5_mixedr;
   reg [11:0]  r_t7_datar,  c_t6_datar;
   reg [11:0]  r_t8_wordr;
   reg [7:0]   r_t8_byter;
   reg         r_t8_BBop;
   reg [5:0]   r_t7_posr,   c_t6_posr, r_t8_posr_d;
   reg         c_t6_pos_ovr;
   reg 	       r_t7_bwer,   r_t8_bwer_d;
   // spin left
   reg [3:0]   r_t6_ball,   c_t5_ball;
   reg [3:0]   r_t6_mixedl, c_t5_mixedl;
   reg [11:0]  r_t7_datal,  c_t6_datal;
   reg [11:0]  r_t8_wordl;
   reg [7:0]   r_t8_bytel;
   reg [5:0]   r_t7_posl,   c_t6_posl, r_t8_posl_d;
   reg         c_t6_pos_ovl;
   reg 	       r_t7_bwel,   r_t8_bwel_d;
   reg [63:0]  r_t7_inA,    r_t7_inB;
   reg [63:0]  r_t10_datas,  c_t9_datasr, c_t9_datasl;
   // spin right
   reg [3:0]   r_t7_balr,   c_t6_balr;
   reg [3:0]   r_t7_mixedr, c_t6_mixedr;
   reg [11:0]  r_t8_datar,  c_t7_datar;
   reg [11:0]  r_t9_wordr;
   reg [7:0]   r_t9_byter;
   reg         r_t9_BBop;
   reg [5:0]   r_t8_posr,   c_t7_posr, r_t9_posr_d;
   reg         c_t7_pos_ovr;
   reg 	       r_t8_bwer,   r_t9_bwer_d;
   // spin left
   reg [3:0]   r_t7_ball,   c_t6_ball;
   reg [3:0]   r_t7_mixedl, c_t6_mixedl;
   reg [11:0]  r_t8_datal,  c_t7_datal;
   reg [11:0]  r_t9_wordl;
   reg [7:0]   r_t9_bytel;
   reg [5:0]   r_t8_posl,   c_t7_posl, r_t9_posl_d;
   reg         c_t7_pos_ovl;
   reg 	       r_t8_bwel,   r_t9_bwel_d;
   reg [63:0]  r_t8_inA,    r_t8_inB;
   reg [63:0]  r_t11_datas,  c_t10_datasr, c_t10_datasl;
   // spin right
   reg [3:0]   r_t8_balr,   c_t7_balr;
   reg [3:0]   r_t8_mixedr, c_t7_mixedr;
   reg [11:0]  r_t9_datar,  c_t8_datar;
   reg [11:0]  r_t10_wordr;
   reg [7:0]   r_t10_byter;
   reg         r_t10_BBop;
   reg [5:0]   r_t9_posr,   c_t8_posr, r_t10_posr_d;
   reg         c_t8_pos_ovr;
   reg 	       r_t9_bwer,   r_t10_bwer_d;
   // spin left
   reg [3:0]   r_t8_ball,   c_t7_ball;
   reg [3:0]   r_t8_mixedl, c_t7_mixedl;
   reg [11:0]  r_t9_datal,  c_t8_datal;
   reg [11:0]  r_t10_wordl;
   reg [7:0]   r_t10_bytel;
   reg [5:0]   r_t9_posl,   c_t8_posl, r_t10_posl_d;
   reg         c_t8_pos_ovl;
   reg 	       r_t9_bwel,   r_t10_bwel_d;
   reg [63:0]  r_t9_inA,    r_t9_inB;
   reg [63:0]  r_t12_datas,  c_t11_datasr, c_t11_datasl;
   // spin right
   reg [3:0]   r_t9_balr,   c_t8_balr;
   reg [3:0]   r_t9_mixedr, c_t8_mixedr;
   reg [11:0]  r_t10_datar,  c_t9_datar;
   reg [11:0]  r_t11_wordr;
   reg [7:0]   r_t11_byter;
   reg         r_t11_BBop;
   reg [5:0]   r_t10_posr,   c_t9_posr, r_t11_posr_d;
   reg         c_t9_pos_ovr;
   reg 	       r_t10_bwer,   r_t11_bwer_d;
   // spin left
   reg [3:0]   r_t9_ball,   c_t8_ball;
   reg [3:0]   r_t9_mixedl, c_t8_mixedl;
   reg [11:0]  r_t10_datal,  c_t9_datal;
   reg [11:0]  r_t11_wordl;
   reg [7:0]   r_t11_bytel;
   reg [5:0]   r_t10_posl,   c_t9_posl, r_t11_posl_d;
   reg         c_t9_pos_ovl;
   reg 	       r_t10_bwel,   r_t11_bwel_d;
   reg [63:0]  r_t10_inA,    r_t10_inB;
   reg [63:0]  r_t13_datas,  c_t12_datasr, c_t12_datasl;
   // spin right
   reg [3:0]   r_t10_balr,   c_t9_balr;
   reg [3:0]   r_t10_mixedr, c_t9_mixedr;
   reg [11:0]  r_t11_datar,  c_t10_datar;
   reg [11:0]  r_t12_wordr;
   reg [7:0]   r_t12_byter;
   reg         r_t12_BBop;
   reg [5:0]   r_t11_posr,   c_t10_posr, r_t12_posr_d;
   reg         c_t10_pos_ovr;
   reg 	       r_t11_bwer,   r_t12_bwer_d;
   // spin left
   reg [3:0]   r_t10_ball,   c_t9_ball;
   reg [3:0]   r_t10_mixedl, c_t9_mixedl;
   reg [11:0]  r_t11_datal,  c_t10_datal;
   reg [11:0]  r_t12_wordl;
   reg [7:0]   r_t12_bytel;
   reg [5:0]   r_t11_posl,   c_t10_posl, r_t12_posl_d;
   reg         c_t10_pos_ovl;
   reg 	       r_t11_bwel,   r_t12_bwel_d;
   reg [63:0]  r_t11_inA,    r_t11_inB;
   reg [63:0]  r_t14_datas,  c_t13_datasr, c_t13_datasl;
   // spin right
   reg [3:0]   r_t11_balr,   c_t10_balr;
   reg [3:0]   r_t11_mixedr, c_t10_mixedr;
   reg [11:0]  r_t12_datar,  c_t11_datar;
   reg [11:0]  r_t13_wordr;
   reg [7:0]   r_t13_byter;
   reg         r_t13_BBop;
   reg [5:0]   r_t12_posr,   c_t11_posr, r_t13_posr_d;
   reg         c_t11_pos_ovr;
   reg 	       r_t12_bwer,   r_t13_bwer_d;
   // spin left
   reg [3:0]   r_t11_ball,   c_t10_ball;
   reg [3:0]   r_t11_mixedl, c_t10_mixedl;
   reg [11:0]  r_t12_datal,  c_t11_datal;
   reg [11:0]  r_t13_wordl;
   reg [7:0]   r_t13_bytel;
   reg [5:0]   r_t12_posl,   c_t11_posl, r_t13_posl_d;
   reg         c_t11_pos_ovl;
   reg 	       r_t12_bwel,   r_t13_bwel_d;
   reg [63:0]  r_t12_inA,    r_t12_inB;
   reg [63:0]  r_t15_datas,  c_t14_datasr, c_t14_datasl;
   // spin right
   reg [3:0]   r_t12_balr,   c_t11_balr;
   reg [3:0]   r_t12_mixedr, c_t11_mixedr;
   reg [11:0]  r_t13_datar,  c_t12_datar;
   reg [11:0]  r_t14_wordr;
   reg [7:0]   r_t14_byter;
   reg         r_t14_BBop;
   reg [5:0]   r_t13_posr,   c_t12_posr, r_t14_posr_d;
   reg         c_t12_pos_ovr;
   reg 	       r_t13_bwer,   r_t14_bwer_d;
   // spin left
   reg [3:0]   r_t12_ball,   c_t11_ball;
   reg [3:0]   r_t12_mixedl, c_t11_mixedl;
   reg [11:0]  r_t13_datal,  c_t12_datal;
   reg [11:0]  r_t14_wordl;
   reg [7:0]   r_t14_bytel;
   reg [5:0]   r_t13_posl,   c_t12_posl, r_t14_posl_d;
   reg         c_t12_pos_ovl;
   reg 	       r_t13_bwel,   r_t14_bwel_d;
   reg [63:0]  r_t13_inA,    r_t13_inB;
   reg [63:0]  r_t16_datas,  c_t15_datasr, c_t15_datasl;
   // spin right
   reg [3:0]   r_t13_balr,   c_t12_balr;
   reg [3:0]   r_t13_mixedr, c_t12_mixedr;
   reg [11:0]  r_t14_datar,  c_t13_datar;
   reg [11:0]  r_t15_wordr;
   reg [7:0]   r_t15_byter;
   reg         r_t15_BBop;
   reg [5:0]   r_t14_posr,   c_t13_posr, r_t15_posr_d;
   reg         c_t13_pos_ovr;
   reg 	       r_t14_bwer,   r_t15_bwer_d;
   // spin left
   reg [3:0]   r_t13_ball,   c_t12_ball;
   reg [3:0]   r_t13_mixedl, c_t12_mixedl;
   reg [11:0]  r_t14_datal,  c_t13_datal;
   reg [11:0]  r_t15_wordl;
   reg [7:0]   r_t15_bytel;
   reg [5:0]   r_t14_posl,   c_t13_posl, r_t15_posl_d;
   reg         c_t13_pos_ovl;
   reg 	       r_t14_bwel,   r_t15_bwel_d;
   reg [63:0]  r_t14_inA,    r_t14_inB;
   reg [63:0]  r_t17_datas,  c_t16_datasr, c_t16_datasl;
   // spin right
   reg [3:0]   r_t14_balr,   c_t13_balr;
   reg [3:0]   r_t14_mixedr, c_t13_mixedr;
   reg [11:0]  r_t15_datar,  c_t14_datar;
   reg [11:0]  r_t16_wordr;
   reg [7:0]   r_t16_byter;
   reg         r_t16_BBop;
   reg [5:0]   r_t15_posr,   c_t14_posr, r_t16_posr_d;
   reg         c_t14_pos_ovr;
   reg 	       r_t15_bwer,   r_t16_bwer_d;
   // spin left
   reg [3:0]   r_t14_ball,   c_t13_ball;
   reg [3:0]   r_t14_mixedl, c_t13_mixedl;
   reg [11:0]  r_t15_datal,  c_t14_datal;
   reg [11:0]  r_t16_wordl;
   reg [7:0]   r_t16_bytel;
   reg [5:0]   r_t15_posl,   c_t14_posl, r_t16_posl_d;
   reg         c_t14_pos_ovl;
   reg 	       r_t15_bwel,   r_t16_bwel_d;
   reg [63:0]  r_t15_inA,    r_t15_inB;
   reg [63:0]  r_t18_datas,  c_t17_datasr, c_t17_datasl;
   // spin right
   reg [3:0]   r_t15_balr,   c_t14_balr;
   reg [3:0]   r_t15_mixedr, c_t14_mixedr;
   reg [11:0]  r_t16_datar,  c_t15_datar;
   reg [11:0]  r_t17_wordr;
   reg [7:0]   r_t17_byter;
   reg         r_t17_BBop;
   reg [5:0]   r_t16_posr,   c_t15_posr, r_t17_posr_d;
   reg         c_t15_pos_ovr;
   reg 	       r_t16_bwer,   r_t17_bwer_d;
   // spin left
   reg [3:0]   r_t15_ball,   c_t14_ball;
   reg [3:0]   r_t15_mixedl, c_t14_mixedl;
   reg [11:0]  r_t16_datal,  c_t15_datal;
   reg [11:0]  r_t17_wordl;
   reg [7:0]   r_t17_bytel;
   reg [5:0]   r_t16_posl,   c_t15_posl, r_t17_posl_d;
   reg         c_t15_pos_ovl;
   reg 	       r_t16_bwel,   r_t17_bwel_d;
   reg [63:0]  r_t16_inA,    r_t16_inB;
   reg [63:0]  r_t19_datas,  c_t18_datasr, c_t18_datasl;
   // spin right
   reg [3:0]   r_t16_balr,   c_t15_balr;
   reg [3:0]   r_t16_mixedr, c_t15_mixedr;
   reg [11:0]  r_t17_datar,  c_t16_datar;
   reg [11:0]  r_t18_wordr;
   reg [7:0]   r_t18_byter;
   reg         r_t18_BBop;
   reg [5:0]   r_t17_posr,   c_t16_posr, r_t18_posr_d;
   reg         c_t16_pos_ovr;
   reg 	       r_t17_bwer,   r_t18_bwer_d;
   // spin left
   reg [3:0]   r_t16_ball,   c_t15_ball;
   reg [3:0]   r_t16_mixedl, c_t15_mixedl;
   reg [11:0]  r_t17_datal,  c_t16_datal;
   reg [11:0]  r_t18_wordl;
   reg [7:0]   r_t18_bytel;
   reg [5:0]   r_t17_posl,   c_t16_posl, r_t18_posl_d;
   reg         c_t16_pos_ovl;
   reg 	       r_t17_bwel,   r_t18_bwel_d;
   reg [63:0]  r_t17_inA,    r_t17_inB;
   reg [63:0]  r_t20_datas,  c_t19_datasr, c_t19_datasl;
   // spin right
   reg [3:0]   r_t17_balr,   c_t16_balr;
   reg [3:0]   r_t17_mixedr, c_t16_mixedr;
   reg [11:0]  r_t18_datar,  c_t17_datar;
   reg [11:0]  r_t19_wordr;
   reg [7:0]   r_t19_byter;
   reg         r_t19_BBop;
   reg [5:0]   r_t18_posr,   c_t17_posr, r_t19_posr_d;
   reg         c_t17_pos_ovr;
   reg 	       r_t18_bwer,   r_t19_bwer_d;
   // spin left
   reg [3:0]   r_t17_ball,   c_t16_ball;
   reg [3:0]   r_t17_mixedl, c_t16_mixedl;
   reg [11:0]  r_t18_datal,  c_t17_datal;
   reg [11:0]  r_t19_wordl;
   reg [7:0]   r_t19_bytel;
   reg [5:0]   r_t18_posl,   c_t17_posl, r_t19_posl_d;
   reg         c_t17_pos_ovl;
   reg 	       r_t18_bwel,   r_t19_bwel_d;
   reg [63:0]  r_t18_inA,    r_t18_inB;
   reg [63:0]  r_t21_datas,  c_t20_datasr, c_t20_datasl;
   // spin right
   reg [3:0]   r_t18_balr,   c_t17_balr;
   reg [3:0]   r_t18_mixedr, c_t17_mixedr;
   reg [11:0]  r_t19_datar,  c_t18_datar;
   reg [11:0]  r_t20_wordr;
   reg [7:0]   r_t20_byter;
   reg         r_t20_BBop;
   reg [5:0]   r_t19_posr,   c_t18_posr, r_t20_posr_d;
   reg         c_t18_pos_ovr;
   reg 	       r_t19_bwer,   r_t20_bwer_d;
   // spin left
   reg [3:0]   r_t18_ball,   c_t17_ball;
   reg [3:0]   r_t18_mixedl, c_t17_mixedl;
   reg [11:0]  r_t19_datal,  c_t18_datal;
   reg [11:0]  r_t20_wordl;
   reg [7:0]   r_t20_bytel;
   reg [5:0]   r_t19_posl,   c_t18_posl, r_t20_posl_d;
   reg         c_t18_pos_ovl;
   reg 	       r_t19_bwel,   r_t20_bwel_d;
   reg [63:0]  r_t19_inA,    r_t19_inB;
   reg [63:0]  r_t22_datas,  c_t21_datasr, c_t21_datasl;
   // spin right
   reg [3:0]   r_t19_balr,   c_t18_balr;
   reg [3:0]   r_t19_mixedr, c_t18_mixedr;
   reg [11:0]  r_t20_datar,  c_t19_datar;
   reg [11:0]  r_t21_wordr;
   reg [7:0]   r_t21_byter;
   reg         r_t21_BBop;
   reg [5:0]   r_t20_posr,   c_t19_posr, r_t21_posr_d;
   reg         c_t19_pos_ovr;
   reg 	       r_t20_bwer,   r_t21_bwer_d;
   // spin left
   reg [3:0]   r_t19_ball,   c_t18_ball;
   reg [3:0]   r_t19_mixedl, c_t18_mixedl;
   reg [11:0]  r_t20_datal,  c_t19_datal;
   reg [11:0]  r_t21_wordl;
   reg [7:0]   r_t21_bytel;
   reg [5:0]   r_t20_posl,   c_t19_posl, r_t21_posl_d;
   reg         c_t19_pos_ovl;
   reg 	       r_t20_bwel,   r_t21_bwel_d;
   reg 	       r_t20_pos_ovr;
   reg [5:0]   r_t22_posl_d, r_t22_posr_d;
   reg [6:0]   r_t21_popc;
   reg [63:0]  r_t20_inA, r_t21_inA;
   reg [32:0]  r_t22_popc_l;
   reg [31:0]  r_t22_popc_u0, r_t22_popc_u1;
   
   reg 	       r_t22_BBop;
   reg [7:0]   r_t22_byter;
   reg [7:0]   r_t22_bytel;
   reg [63:0]  r_t23_datas,  c_t22_datasr, c_t22_datasl;
   reg [63:0]  r_t24_cfuge_out;
   
   // area reduction to improve synthesis area efficiency
   wire [63:0] 	 r_t6_MASK  = 64'b0;
   wire [63:0] 	 r_t7_MASK  = 64'hff000000000000ff;
   wire [63:0] 	 r_t8_MASK  = 64'hff000000000000ff;
   wire [63:0] 	 r_t9_MASK  = 64'hffff00000000ffff;
   wire [63:0] 	 r_t10_MASK = 64'hffff00000000ffff;
   wire [63:0] 	 r_t11_MASK = 64'hffffff0000ffffff;
   wire [63:0] 	 r_t12_MASK = 64'hffffff0000ffffff;
   wire [63:0] 	 r_t13_MASK = 64'hffffffffffffffff;
   wire [63:0] 	 r_t14_MASK = 64'hffffffffffffffff;
   wire [63:0] 	 r_t15_MASK = 64'hffffffffffffffff;
   wire [63:0] 	 r_t16_MASK = 64'hffffffffffffffff;
   wire [63:0] 	 r_t17_MASK = 64'hffffffffffffffff;
   wire [63:0] 	 r_t18_MASK = 64'hffffffffffffffff;
   wire [63:0] 	 r_t19_MASK = 64'hffffffffffffffff;
   wire [63:0] 	 r_t20_MASK = 64'hffffffffffffffff;
   wire [63:0] 	 r_t21_MASK = 64'hffffffffffffffff;
   wire [63:0] 	 r_t22_MASK = 64'hffffffffffffffff;

   /* ----------         define tasks         ---------- */
   // identifies and compresses input bits to preserve
   task mix;
      // example:
      //    data_in  = 4'b0010
      //    mask_in  = 4'b1011
      //    dir      = 1'b0
      //    data_out = 4'b0100
      //    balance  = 4'b1110
      input [3:0] data_in;
      input [3:0] mask_in;
      input       dir; // 1/0 = left / right
      output [3:0] data_out;
      output [3:0] balance;
      reg [2:0]    c_cnt;
      integer i;
      begin
	 c_cnt = 3'b0;
	 data_out = 4'b0;
	 balance = 4'h0;
	 if (dir) begin
	    for (i=3; i>-1; i=i-1) begin
	       if (mask_in[i]) begin
		  data_out[3'h3 - c_cnt] = data_in[i];
		  balance[3'h3 - c_cnt] = 1'b1;
		  c_cnt = c_cnt + 3'h1;
	       end
	    end
	 end else begin
	    for (i=0; i<4; i=i+1) begin
	       if (~mask_in[i]) begin
		  data_out[c_cnt] = data_in[i];
		  balance[c_cnt] = 1'b1;
		  c_cnt = c_cnt + 3'h1;
	       end
	    end
	 end // else: !if(dir)
      end
   endtask // mix
   
   // incorporates slice into 64-bit word accumulator
   // this task cannot achieve 300MHz
   //     need to divide output into 8-bit addressable slices, to reduce fanin
   //     also, should not 
   task spin;
      // example:
      //    slice_in   = 4'h5
      //    balance_in = 4'he
      //    pos_in     = 6'h07
      //    dir        = 1'b0
      //    data_out   = 64'h00000000000000ef
      //    pos_out    = 6'h08
      input [3:0]   slice_in;
      input [3:0]   balance_in;
      input [5:0]   pos_in;
      input 	    dir; // 1/0 = left / right
      output [11:0] data_out;
      output [5:0]  pos_out;
      output        pos_ov;
      reg [3:0]     pos_int;
      reg [2:0]     pop_cnt_bal;
      integer i;
      begin
//	 pos_out   = pos_in;
	 pos_int   = {1'b0,pos_in[2:0]};
	 data_out  = 12'b0;
	 pop_cnt_bal = balance_in[3] + balance_in[2] + balance_in[1] + balance_in[0];
	 if (dir) begin
	    for (i=3; i>-1; i=i-1) begin
	       if (balance_in[i]) begin
		  data_out[pos_int] = slice_in[i];
//		  pos_out           = pos_out - 6'h1;
		  pos_int           = |pos_int ? pos_int - 4'h1 : 4'd11;
	       end
	    end
	    pos_out = pos_in - pop_cnt_bal;
	    pos_ov = ~pos_in[5] & pos_out[5];
	 end else begin
	    for (i=0; i<4; i=i+1) begin
	       if (balance_in[i]) begin
		  data_out[pos_int] = slice_in[i];
//		  pos_out           = pos_out + 6'h1;
		  pos_int           = pos_int + 4'h1;
	       end
	    end
	    pos_out = pos_in + pop_cnt_bal;
	    pos_ov = pos_in[5] & ~pos_out[5];
	 end // else: !if(dir)
      end
   endtask // spin

   /* ----------         drive outputs        ---------- */

   wire 	 o_done   = r_t2_done; // default for non-multistage use
   wire [63:0] 	 o_result = r_t24_cfuge_out;
   wire 	 o_vm     = 1'b0;
   wire          o_hit    = 1'b0;
   wire 	 o_error  = 1'b0;
   
   /* ----------      combinatorial blocks    ---------- */

   // track the number of elements, starting at start
   assign c_t1_cnt = r_reset ? 9'h0 :
		     r_t1_start ? r_t1_elemCnt : 
		     |r_t2_cnt ? {r_t2_cnt - 9'h1} : r_t2_cnt;

   // is the pipe active this cycle?
   //   following is equivalent to: assign c_t1_active = |c_t1_cnt;
   assign c_t1_active = r_t1_start ? |r_t1_elemCnt : 
			|r_t2_cnt[8:1];
   
   // if not using multi-stage mode, make sure a multi-stage start doesn't hang the machine
   assign c_t1_done = r_t1_start && (r_t1_arc0[ARC0_START0] || r_t1_arc0[ARC0_START1]);
   
   // select first part of mux here, mux in AMS data next cycle
   always @(*)
     begin
	case (r_t2_arc0[ARC0_BMux+:2])
	  Arc0_BMux_B:    c_t2_inB = r_t2_vrB;
	  Arc0_BMux_S:    c_t2_inB = r_t2_scalar;
	  Arc0_BMux_Zero: c_t2_inB = 64'b0;
	  default:        c_t2_inB = 64'b0;
	endcase // case (r_t1_arc0[ARC0_BMux+:2])
     end
   
   assign c_t2_inA = r_t2_arc0[ARC0_AMux] == Arc0_AMux_Zero ? 64'b0 : r_t2_vrA;

   // centrifuge specific functionality is here
   
   // pipe 3
   always @(*)
     begin
	mix(r_t3_inA[0+:4], r_t3_inB[0+:4], 1'b0, c_t3_mixedr[3:0], c_t3_balr[3:0]);

	mix(r_t3_inA[63-:4], r_t3_inB[63-:4], 1'b1, c_t3_mixedl[3:0], c_t3_ball[3:0]);
     end
   // pipe 4
   always @(*)
     begin
	mix(r_t4_inA[4+:4], r_t4_inB[4+:4], 1'b0, c_t4_mixedr[3:0], c_t4_balr[3:0]);

	mix(r_t4_inA[59-:4], r_t4_inB[59-:4], 1'b1, c_t4_mixedl[3:0], c_t4_ball[3:0]);
     end
   // pipe 5
   always @(*)
     begin
	mix(r_t5_inA[8+:4], r_t5_inB[8+:4], 1'b0, c_t5_mixedr[3:0], c_t5_balr[3:0]);

	mix(r_t5_inA[55-:4], r_t5_inB[55-:4], 1'b1, c_t5_mixedl[3:0], c_t5_ball[3:0]);
     end
   // pipe 6
   always @(*)
     begin
	mix(r_t6_inA[12+:4], r_t6_inB[12+:4], 1'b0, c_t6_mixedr[3:0], c_t6_balr[3:0]);

	mix(r_t6_inA[51-:4], r_t6_inB[51-:4], 1'b1, c_t6_mixedl[3:0], c_t6_ball[3:0]);
     end
   // pipe 7
   always @(*)
     begin
	mix(r_t7_inA[16+:4], r_t7_inB[16+:4], 1'b0, c_t7_mixedr[3:0], c_t7_balr[3:0]);

	mix(r_t7_inA[47-:4], r_t7_inB[47-:4], 1'b1, c_t7_mixedl[3:0], c_t7_ball[3:0]);
     end
   // pipe 8
   always @(*)
     begin
	mix(r_t8_inA[20+:4], r_t8_inB[20+:4], 1'b0, c_t8_mixedr[3:0], c_t8_balr[3:0]);

	mix(r_t8_inA[43-:4], r_t8_inB[43-:4], 1'b1, c_t8_mixedl[3:0], c_t8_ball[3:0]);
     end
   // pipe 9
   always @(*)
     begin
	mix(r_t9_inA[24+:4], r_t9_inB[24+:4], 1'b0, c_t9_mixedr[3:0], c_t9_balr[3:0]);

	mix(r_t9_inA[39-:4], r_t9_inB[39-:4], 1'b1, c_t9_mixedl[3:0], c_t9_ball[3:0]);
     end
   // pipe 10
   always @(*)
     begin
	mix(r_t10_inA[28+:4], r_t10_inB[28+:4], 1'b0, c_t10_mixedr[3:0], c_t10_balr[3:0]);

	mix(r_t10_inA[35-:4], r_t10_inB[35-:4], 1'b1, c_t10_mixedl[3:0], c_t10_ball[3:0]);
     end
   // pipe 11
   always @(*)
     begin
	mix(r_t11_inA[32+:4], r_t11_inB[32+:4], 1'b0, c_t11_mixedr[3:0], c_t11_balr[3:0]);

	mix(r_t11_inA[31-:4], r_t11_inB[31-:4], 1'b1, c_t11_mixedl[3:0], c_t11_ball[3:0]);
     end
   // pipe 12
   always @(*)
     begin
	mix(r_t12_inA[36+:4], r_t12_inB[36+:4], 1'b0, c_t12_mixedr[3:0], c_t12_balr[3:0]);

	mix(r_t12_inA[27-:4], r_t12_inB[27-:4], 1'b1, c_t12_mixedl[3:0], c_t12_ball[3:0]);
     end
   // pipe 13
   always @(*)
     begin
	mix(r_t13_inA[40+:4], r_t13_inB[40+:4], 1'b0, c_t13_mixedr[3:0], c_t13_balr[3:0]);

	mix(r_t13_inA[23-:4], r_t13_inB[23-:4], 1'b1, c_t13_mixedl[3:0], c_t13_ball[3:0]);
     end
   // pipe 14
   always @(*)
     begin
	mix(r_t14_inA[44+:4], r_t14_inB[44+:4], 1'b0, c_t14_mixedr[3:0], c_t14_balr[3:0]);

	mix(r_t14_inA[19-:4], r_t14_inB[19-:4], 1'b1, c_t14_mixedl[3:0], c_t14_ball[3:0]);
     end
   // pipe 15
   always @(*)
     begin
	mix(r_t15_inA[48+:4], r_t15_inB[48+:4], 1'b0, c_t15_mixedr[3:0], c_t15_balr[3:0]);

	mix(r_t15_inA[15-:4], r_t15_inB[15-:4], 1'b1, c_t15_mixedl[3:0], c_t15_ball[3:0]);
     end
   // pipe 16
   always @(*)
     begin
	mix(r_t16_inA[52+:4], r_t16_inB[52+:4], 1'b0, c_t16_mixedr[3:0], c_t16_balr[3:0]);

	mix(r_t16_inA[11-:4], r_t16_inB[11-:4], 1'b1, c_t16_mixedl[3:0], c_t16_ball[3:0]);
     end
   // pipe 17
   always @(*)
     begin
	mix(r_t17_inA[56+:4], r_t17_inB[56+:4], 1'b0, c_t17_mixedr[3:0], c_t17_balr[3:0]);

	mix(r_t17_inA[7-:4], r_t17_inB[7-:4], 1'b1, c_t17_mixedl[3:0], c_t17_ball[3:0]);
     end
   // pipe 18
   always @(*)
     begin
	mix(r_t18_inA[60+:4], r_t18_inB[60+:4], 1'b0, c_t18_mixedr[3:0], c_t18_balr[3:0]);

	mix(r_t18_inA[3-:4], r_t18_inB[3-:4], 1'b1, c_t18_mixedl[3:0], c_t18_ball[3:0]);
     end

   // pipe 4
   always @(*)
     begin
	spin(r_t4_mixedr, r_t4_balr, r_t4_posr, 1'b0, 
	     c_t4_datar, c_t4_posr, c_t4_pos_ovr);

	spin(r_t4_mixedl, r_t4_ball, r_t4_posl, 1'b1, 
	     c_t4_datal, c_t4_posl, c_t4_pos_ovl);
     end
   // pipe 5
   always @(*)
     begin
	spin(r_t5_mixedr, r_t5_balr, r_t5_posr, 1'b0, 
	     c_t5_datar, c_t5_posr, c_t5_pos_ovr);

	spin(r_t5_mixedl, r_t5_ball, r_t5_posl, 1'b1, 
	     c_t5_datal, c_t5_posl, c_t5_pos_ovl);
     end
   // pipe 6
   always @(*)
     begin
	spin(r_t6_mixedr, r_t6_balr, r_t6_posr, 1'b0, 
	     c_t6_datar, c_t6_posr, c_t6_pos_ovr);

	spin(r_t6_mixedl, r_t6_ball, r_t6_posl, 1'b1, 
	     c_t6_datal, c_t6_posl, c_t6_pos_ovl);
     end
   // pipe 7
   always @(*)
     begin
	spin(r_t7_mixedr, r_t7_balr, r_t7_posr, 1'b0, 
	     c_t7_datar, c_t7_posr, c_t7_pos_ovr);

	spin(r_t7_mixedl, r_t7_ball, r_t7_posl, 1'b1, 
	     c_t7_datal, c_t7_posl, c_t7_pos_ovl);
     end
   // pipe 8
   always @(*)
     begin
	spin(r_t8_mixedr, r_t8_balr, r_t8_posr, 1'b0, 
	     c_t8_datar, c_t8_posr, c_t8_pos_ovr);

	spin(r_t8_mixedl, r_t8_ball, r_t8_posl, 1'b1, 
	     c_t8_datal, c_t8_posl, c_t8_pos_ovl);
     end
   // pipe 9
   always @(*)
     begin
	spin(r_t9_mixedr, r_t9_balr, r_t9_posr, 1'b0, 
	     c_t9_datar, c_t9_posr, c_t9_pos_ovr);

	spin(r_t9_mixedl, r_t9_ball, r_t9_posl, 1'b1, 
	     c_t9_datal, c_t9_posl, c_t9_pos_ovl);
     end
   // pipe 10
   always @(*)
     begin
	spin(r_t10_mixedr, r_t10_balr, r_t10_posr, 1'b0, 
	     c_t10_datar, c_t10_posr, c_t10_pos_ovr);

	spin(r_t10_mixedl, r_t10_ball, r_t10_posl, 1'b1, 
	     c_t10_datal, c_t10_posl, c_t10_pos_ovl);
     end
   // pipe 11
   always @(*)
     begin
	spin(r_t11_mixedr, r_t11_balr, r_t11_posr, 1'b0, 
	     c_t11_datar, c_t11_posr, c_t11_pos_ovr);

	spin(r_t11_mixedl, r_t11_ball, r_t11_posl, 1'b1, 
	     c_t11_datal, c_t11_posl, c_t11_pos_ovl);
     end
   // pipe 12
   always @(*)
     begin
	spin(r_t12_mixedr, r_t12_balr, r_t12_posr, 1'b0, 
	     c_t12_datar, c_t12_posr, c_t12_pos_ovr);

	spin(r_t12_mixedl, r_t12_ball, r_t12_posl, 1'b1, 
	     c_t12_datal, c_t12_posl, c_t12_pos_ovl);
     end
   // pipe 13
   always @(*)
     begin
	spin(r_t13_mixedr, r_t13_balr, r_t13_posr, 1'b0, 
	     c_t13_datar, c_t13_posr, c_t13_pos_ovr);

	spin(r_t13_mixedl, r_t13_ball, r_t13_posl, 1'b1, 
	     c_t13_datal, c_t13_posl, c_t13_pos_ovl);
     end
   // pipe 14
   always @(*)
     begin
	spin(r_t14_mixedr, r_t14_balr, r_t14_posr, 1'b0, 
	     c_t14_datar, c_t14_posr, c_t14_pos_ovr);

	spin(r_t14_mixedl, r_t14_ball, r_t14_posl, 1'b1, 
	     c_t14_datal, c_t14_posl, c_t14_pos_ovl);
     end
   // pipe 15
   always @(*)
     begin
	spin(r_t15_mixedr, r_t15_balr, r_t15_posr, 1'b0, 
	     c_t15_datar, c_t15_posr, c_t15_pos_ovr);

	spin(r_t15_mixedl, r_t15_ball, r_t15_posl, 1'b1, 
	     c_t15_datal, c_t15_posl, c_t15_pos_ovl);
     end
   // pipe 16
   always @(*)
     begin
	spin(r_t16_mixedr, r_t16_balr, r_t16_posr, 1'b0, 
	     c_t16_datar, c_t16_posr, c_t16_pos_ovr);

	spin(r_t16_mixedl, r_t16_ball, r_t16_posl, 1'b1, 
	     c_t16_datal, c_t16_posl, c_t16_pos_ovl);
     end
   // pipe 17
   always @(*)
     begin
	spin(r_t17_mixedr, r_t17_balr, r_t17_posr, 1'b0, 
	     c_t17_datar, c_t17_posr, c_t17_pos_ovr);

	spin(r_t17_mixedl, r_t17_ball, r_t17_posl, 1'b1, 
	     c_t17_datal, c_t17_posl, c_t17_pos_ovl);
     end
   // pipe 18
   always @(*)
     begin
	spin(r_t18_mixedr, r_t18_balr, r_t18_posr, 1'b0, 
	     c_t18_datar, c_t18_posr, c_t18_pos_ovr);

	spin(r_t18_mixedl, r_t18_ball, r_t18_posl, 1'b1, 
	     c_t18_datal, c_t18_posl, c_t18_pos_ovl);
     end
   // pipe 19
   always @(*)
     begin
	spin(r_t19_mixedr, r_t19_balr, r_t19_posr, 1'b0, 
	     c_t19_datar, c_t19_posr, c_t19_pos_ovr);

	spin(r_t19_mixedl, r_t19_ball, r_t19_posl, 1'b1, 
	     c_t19_datal, c_t19_posl, c_t19_pos_ovl);
     end

// leda E267 off Range index out of bound
   // pipe 6
   always @(*)
     begin
	c_t6_datasr = 64'b0;
	if (r_t6_bwer_d)
	  c_t6_datasr[{r_t6_posr_d[5:3],3'b0} +: 6'h8] = r_t6_byter[7:0];
	c_t6_datasl = 64'b0;
	if (r_t6_bwel_d)
	  c_t6_datasl[{r_t6_posl_d[5:3],3'b0} +: 6'h8] = r_t6_bytel[7:0];
     end
   // pipe 7
   always @(*)
     begin
	c_t7_datasr = 64'b0;
	if (r_t7_bwer_d)
	  c_t7_datasr[{r_t7_posr_d[5:3],3'b0} +: 6'h8] = r_t7_byter[7:0];
	c_t7_datasl = 64'b0;
	if (r_t7_bwel_d)
	  c_t7_datasl[{r_t7_posl_d[5:3],3'b0} +: 6'h8] = r_t7_bytel[7:0];
     end
   // pipe 8
   always @(*)
     begin
	c_t8_datasr = 64'b0;
	if (r_t8_bwer_d)
	  c_t8_datasr[{r_t8_posr_d[5:3],3'b0} +: 6'h8] = r_t8_byter[7:0];
	c_t8_datasl = 64'b0;
	if (r_t8_bwel_d)
	  c_t8_datasl[{r_t8_posl_d[5:3],3'b0} +: 6'h8] = r_t8_bytel[7:0];
     end
   // pipe 9
   always @(*)
     begin
	c_t9_datasr = 64'b0;
	if (r_t9_bwer_d)
	  c_t9_datasr[{r_t9_posr_d[5:3],3'b0} +: 6'h8] = r_t9_byter[7:0];
	c_t9_datasl = 64'b0;
	if (r_t9_bwel_d)
	  c_t9_datasl[{r_t9_posl_d[5:3],3'b0} +: 6'h8] = r_t9_bytel[7:0];
     end
   // pipe 10
   always @(*)
     begin
	c_t10_datasr = 64'b0;
	if (r_t10_bwer_d)
	  c_t10_datasr[{r_t10_posr_d[5:3],3'b0} +: 6'h8] = r_t10_byter[7:0];
	c_t10_datasl = 64'b0;
	if (r_t10_bwel_d)
	  c_t10_datasl[{r_t10_posl_d[5:3],3'b0} +: 6'h8] = r_t10_bytel[7:0];
     end
   // pipe 11
   always @(*)
     begin
	c_t11_datasr = 64'b0;
	if (r_t11_bwer_d)
	  c_t11_datasr[{r_t11_posr_d[5:3],3'b0} +: 6'h8] = r_t11_byter[7:0];
	c_t11_datasl = 64'b0;
	if (r_t11_bwel_d)
	  c_t11_datasl[{r_t11_posl_d[5:3],3'b0} +: 6'h8] = r_t11_bytel[7:0];
     end
   // pipe 12
   always @(*)
     begin
	c_t12_datasr = 64'b0;
	if (r_t12_bwer_d)
	  c_t12_datasr[{r_t12_posr_d[5:3],3'b0} +: 6'h8] = r_t12_byter[7:0];
	c_t12_datasl = 64'b0;
	if (r_t12_bwel_d)
	  c_t12_datasl[{r_t12_posl_d[5:3],3'b0} +: 6'h8] = r_t12_bytel[7:0];
     end
   // pipe 13
   always @(*)
     begin
	c_t13_datasr = 64'b0;
	if (r_t13_bwer_d)
	  c_t13_datasr[{r_t13_posr_d[5:3],3'b0} +: 6'h8] = r_t13_byter[7:0];
	c_t13_datasl = 64'b0;
	if (r_t13_bwel_d)
	  c_t13_datasl[{r_t13_posl_d[5:3],3'b0} +: 6'h8] = r_t13_bytel[7:0];
     end
   // pipe 14
   always @(*)
     begin
	c_t14_datasr = 64'b0;
	if (r_t14_bwer_d)
	  c_t14_datasr[{r_t14_posr_d[5:3],3'b0} +: 6'h8] = r_t14_byter[7:0];
	c_t14_datasl = 64'b0;
	if (r_t14_bwel_d)
	  c_t14_datasl[{r_t14_posl_d[5:3],3'b0} +: 6'h8] = r_t14_bytel[7:0];
     end
   // pipe 15
   always @(*)
     begin
	c_t15_datasr = 64'b0;
	if (r_t15_bwer_d)
	  c_t15_datasr[{r_t15_posr_d[5:3],3'b0} +: 6'h8] = r_t15_byter[7:0];
	c_t15_datasl = 64'b0;
	if (r_t15_bwel_d)
	  c_t15_datasl[{r_t15_posl_d[5:3],3'b0} +: 6'h8] = r_t15_bytel[7:0];
     end
   // pipe 16
   always @(*)
     begin
	c_t16_datasr = 64'b0;
	if (r_t16_bwer_d)
	  c_t16_datasr[{r_t16_posr_d[5:3],3'b0} +: 6'h8] = r_t16_byter[7:0];
	c_t16_datasl = 64'b0;
	if (r_t16_bwel_d)
	  c_t16_datasl[{r_t16_posl_d[5:3],3'b0} +: 6'h8] = r_t16_bytel[7:0];
     end
   // pipe 17
   always @(*)
     begin
	c_t17_datasr = 64'b0;
	if (r_t17_bwer_d)
	  c_t17_datasr[{r_t17_posr_d[5:3],3'b0} +: 6'h8] = r_t17_byter[7:0];
	c_t17_datasl = 64'b0;
	if (r_t17_bwel_d)
	  c_t17_datasl[{r_t17_posl_d[5:3],3'b0} +: 6'h8] = r_t17_bytel[7:0];
     end
   // pipe 18
   always @(*)
     begin
	c_t18_datasr = 64'b0;
	if (r_t18_bwer_d)
	  c_t18_datasr[{r_t18_posr_d[5:3],3'b0} +: 6'h8] = r_t18_byter[7:0];
	c_t18_datasl = 64'b0;
	if (r_t18_bwel_d)
	  c_t18_datasl[{r_t18_posl_d[5:3],3'b0} +: 6'h8] = r_t18_bytel[7:0];
     end
   // pipe 19
   always @(*)
     begin
	c_t19_datasr = 64'b0;
	if (r_t19_bwer_d)
	  c_t19_datasr[{r_t19_posr_d[5:3],3'b0} +: 6'h8] = r_t19_byter[7:0];
	c_t19_datasl = 64'b0;
	if (r_t19_bwel_d)
	  c_t19_datasl[{r_t19_posl_d[5:3],3'b0} +: 6'h8] = r_t19_bytel[7:0];
     end
   // pipe 20
   always @(*)
     begin
	c_t20_datasr = 64'b0;
	if (r_t20_bwer_d)
	  c_t20_datasr[{r_t20_posr_d[5:3],3'b0} +: 6'h8] = r_t20_byter[7:0];
	c_t20_datasl = 64'b0;
	if (r_t20_bwel_d)
	  c_t20_datasl[{r_t20_posl_d[5:3],3'b0} +: 6'h8] = r_t20_bytel[7:0];
     end
   // pipe 21
   always @(*)
     begin
	c_t21_datasr = 64'b0;
	  c_t21_datasr[{r_t21_posr_d[5:3],3'b0} +: 6'h8] = r_t21_byter[7:0];
	c_t21_datasl = 64'b0;
	  c_t21_datasl[{r_t21_posl_d[5:3],3'b0} +: 6'h8] = r_t21_bytel[7:0];
     end

   always @(*)
     begin
	c_t22_datasr = 64'b0;
	c_t22_datasr[8 * r_t22_posr_d[5:3] +: 8] = r_t22_byter[7:0];
	c_t22_datasl = 64'b0;
	c_t22_datasl[8 * r_t22_posl_d[5:3] +: 8] = r_t22_bytel[7:0];
     end
// leda E267 on Range index out of bound

   /* ----------            registers         ---------- */
   always @(posedge clk2x) begin // 300Mhz
      // register inputs
      r_reset      <= i_reset;
      r_t1_start   <= i_start;
      r_t1_vrA     <= i_vrA;
      r_t1_vrB     <= i_vrB;
      r_t2_vrA     <= r_t1_vrA;
      r_t2_vrB     <= r_t1_vrB;

      r_t1_scalar  <= i_start ? i_scalar  : r_t1_scalar;
      r_t1_elemCnt <= i_start ? i_elemCnt : r_t1_elemCnt;
      r_t1_arc0    <= i_start ? i_arc0    : r_t1_arc0;
      r_t1_arc1    <= i_start ? i_arc1    : r_t1_arc1;
      r_t1_arc2    <= i_start ? i_arc2    : r_t1_arc2;
      r_t1_arc3    <= i_start ? i_arc3    : r_t1_arc3;
      r_t2_scalar  <= r_t1_scalar;

      // internal registers
      r_t2_cnt     <= c_t1_cnt;
      r_t2_active  <= r_reset ? 1'b0 : c_t1_active;
      r_t2_arc0    <= r_reset ? BBARC0_idle : c_t1_active ? r_t1_arc0 : BBARC0_idle;
      r_t2_done    <= c_t1_done;
      r_t3_inA     <= r_t2_arc0[ARC0_BBop] ? c_t2_inB : c_t2_inA; // swap inputs if pop cnt
      
      r_t3_active  <= r_t2_active;
      r_t3_inB     <= r_t2_arc0[ARC0_BBop] ? c_t2_inA : c_t2_inB; // swap inputs if pop cnt

      r_t3_BBop    <= r_t2_arc0[ARC0_BBop];
      
      r_t4_active  <= r_t3_active;
      r_t4_BBop    <= r_t3_BBop;
      r_t5_BBop    <= r_t4_BBop;
      
      //centrifuge constants
      r_t4_posr    <= 6'h0;
      r_t4_posl    <= 6'h3f;
      r_t5_wordr   <= 12'h0;
      r_t5_wordl   <= 12'h0;
      r_t6_datas   <= 64'b0;
// centrifuge pipeline registers
      r_t4_inA    <= r_t3_inA;
      r_t4_inB    <= r_t3_inB;
      r_t4_mixedr <= c_t3_mixedr;
      r_t4_mixedl <= c_t3_mixedl;
      r_t4_balr   <= c_t3_balr;
      r_t4_ball   <= c_t3_ball;
      r_t5_posr   <= c_t4_posr;
      r_t5_posl   <= c_t4_posl;
      r_t5_bwer   <= c_t4_posr[3] ^ r_t4_posr[3];
      r_t5_bwel   <= c_t4_posl[3] ^ r_t4_posl[3];
      r_t5_datar  <= c_t4_datar;
      r_t5_datal  <= c_t4_datal;
      r_t6_BBop   <= r_t5_BBop;
      r_t6_wordr  <= r_t5_bwer ? {8'b0, r_t5_wordr[11:8] | r_t5_datar[11:8]}       : r_t5_wordr[11:0] | r_t5_datar[11:0];
      r_t6_wordl  <= r_t5_bwel ? {4'b0, r_t5_wordl[11:8] | r_t5_datal[11:8], 4'b0} : r_t5_wordl[11:0] | r_t5_datal[11:0];
      r_t6_byter  <= r_t5_wordr[7:0] | r_t5_datar[7:0];
      r_t6_bytel  <= r_t5_wordl[7:0] | r_t5_datal[7:0];
      r_t6_bwer_d <= r_t5_bwer;
      r_t6_bwel_d <= r_t5_bwel;
      r_t6_posr_d <= r_t5_bwer ? (|r_t5_posr ? r_t5_posr - 6'h8 : 6'h38) : r_t5_posr;
      r_t6_posl_d <= r_t5_bwel ? (&r_t5_posl ? 6'h00 : r_t5_posl + 6'h8) : r_t5_posl;
      r_t7_datas  <= r_t6_MASK & (c_t6_datasr | c_t6_datasl | r_t6_datas);
// centrifuge pipeline registers
      r_t5_inA    <= r_t4_inA;
      r_t5_inB    <= r_t4_inB;
      r_t5_mixedr <= c_t4_mixedr;
      r_t5_mixedl <= c_t4_mixedl;
      r_t5_balr   <= c_t4_balr;
      r_t5_ball   <= c_t4_ball;
      r_t6_posr   <= c_t5_posr;
      r_t6_posl   <= c_t5_posl;
      r_t6_bwer   <= c_t5_posr[3] ^ r_t5_posr[3];
      r_t6_bwel   <= c_t5_posl[3] ^ r_t5_posl[3];
      r_t6_datar  <= c_t5_datar;
      r_t6_datal  <= c_t5_datal;
      r_t7_BBop   <= r_t6_BBop;
      r_t7_wordr  <= r_t6_bwer ? {8'b0, r_t6_wordr[11:8] | r_t6_datar[11:8]}       : r_t6_wordr[11:0] | r_t6_datar[11:0];
      r_t7_wordl  <= r_t6_bwel ? {4'b0, r_t6_wordl[11:8] | r_t6_datal[11:8], 4'b0} : r_t6_wordl[11:0] | r_t6_datal[11:0];
      r_t7_byter  <= r_t6_wordr[7:0] | r_t6_datar[7:0];
      r_t7_bytel  <= r_t6_wordl[7:0] | r_t6_datal[7:0];
      r_t7_bwer_d <= r_t6_bwer;
      r_t7_bwel_d <= r_t6_bwel;
      r_t7_posr_d <= r_t6_bwer ? (|r_t6_posr ? r_t6_posr - 6'h8 : 6'h38) : r_t6_posr;
      r_t7_posl_d <= r_t6_bwel ? (&r_t6_posl ? 6'h00 : r_t6_posl + 6'h8) : r_t6_posl;
      r_t8_datas  <= r_t7_MASK & (c_t7_datasr | c_t7_datasl | r_t7_datas);
// centrifuge pipeline registers
      r_t6_inA    <= r_t5_inA;
      r_t6_inB    <= r_t5_inB;
      r_t6_mixedr <= c_t5_mixedr;
      r_t6_mixedl <= c_t5_mixedl;
      r_t6_balr   <= c_t5_balr;
      r_t6_ball   <= c_t5_ball;
      r_t7_posr   <= c_t6_posr;
      r_t7_posl   <= c_t6_posl;
      r_t7_bwer   <= c_t6_posr[3] ^ r_t6_posr[3];
      r_t7_bwel   <= c_t6_posl[3] ^ r_t6_posl[3];
      r_t7_datar  <= c_t6_datar;
      r_t7_datal  <= c_t6_datal;
      r_t8_BBop   <= r_t7_BBop;
      r_t8_wordr  <= r_t7_bwer ? {8'b0, r_t7_wordr[11:8] | r_t7_datar[11:8]}       : r_t7_wordr[11:0] | r_t7_datar[11:0];
      r_t8_wordl  <= r_t7_bwel ? {4'b0, r_t7_wordl[11:8] | r_t7_datal[11:8], 4'b0} : r_t7_wordl[11:0] | r_t7_datal[11:0];
      r_t8_byter  <= r_t7_wordr[7:0] | r_t7_datar[7:0];
      r_t8_bytel  <= r_t7_wordl[7:0] | r_t7_datal[7:0];
      r_t8_bwer_d <= r_t7_bwer;
      r_t8_bwel_d <= r_t7_bwel;
      r_t8_posr_d <= r_t7_bwer ? (|r_t7_posr ? r_t7_posr - 6'h8 : 6'h38) : r_t7_posr;
      r_t8_posl_d <= r_t7_bwel ? (&r_t7_posl ? 6'h00 : r_t7_posl + 6'h8) : r_t7_posl;
      r_t9_datas  <= r_t8_MASK & (c_t8_datasr | c_t8_datasl | r_t8_datas);
// centrifuge pipeline registers
      r_t7_inA    <= r_t6_inA;
      r_t7_inB    <= r_t6_inB;
      r_t7_mixedr <= c_t6_mixedr;
      r_t7_mixedl <= c_t6_mixedl;
      r_t7_balr   <= c_t6_balr;
      r_t7_ball   <= c_t6_ball;
      r_t8_posr   <= c_t7_posr;
      r_t8_posl   <= c_t7_posl;
      r_t8_bwer   <= c_t7_posr[3] ^ r_t7_posr[3];
      r_t8_bwel   <= c_t7_posl[3] ^ r_t7_posl[3];
      r_t8_datar  <= c_t7_datar;
      r_t8_datal  <= c_t7_datal;
      r_t9_BBop   <= r_t8_BBop;
      r_t9_wordr  <= r_t8_bwer ? {8'b0, r_t8_wordr[11:8] | r_t8_datar[11:8]}       : r_t8_wordr[11:0] | r_t8_datar[11:0];
      r_t9_wordl  <= r_t8_bwel ? {4'b0, r_t8_wordl[11:8] | r_t8_datal[11:8], 4'b0} : r_t8_wordl[11:0] | r_t8_datal[11:0];
      r_t9_byter  <= r_t8_wordr[7:0] | r_t8_datar[7:0];
      r_t9_bytel  <= r_t8_wordl[7:0] | r_t8_datal[7:0];
      r_t9_bwer_d <= r_t8_bwer;
      r_t9_bwel_d <= r_t8_bwel;
      r_t9_posr_d <= r_t8_bwer ? (|r_t8_posr ? r_t8_posr - 6'h8 : 6'h38) : r_t8_posr;
      r_t9_posl_d <= r_t8_bwel ? (&r_t8_posl ? 6'h00 : r_t8_posl + 6'h8) : r_t8_posl;
      r_t10_datas  <= r_t9_MASK & (c_t9_datasr | c_t9_datasl | r_t9_datas);
// centrifuge pipeline registers
      r_t8_inA    <= r_t7_inA;
      r_t8_inB    <= r_t7_inB;
      r_t8_mixedr <= c_t7_mixedr;
      r_t8_mixedl <= c_t7_mixedl;
      r_t8_balr   <= c_t7_balr;
      r_t8_ball   <= c_t7_ball;
      r_t9_posr   <= c_t8_posr;
      r_t9_posl   <= c_t8_posl;
      r_t9_bwer   <= c_t8_posr[3] ^ r_t8_posr[3];
      r_t9_bwel   <= c_t8_posl[3] ^ r_t8_posl[3];
      r_t9_datar  <= c_t8_datar;
      r_t9_datal  <= c_t8_datal;
      r_t10_BBop   <= r_t9_BBop;
      r_t10_wordr  <= r_t9_bwer ? {8'b0, r_t9_wordr[11:8] | r_t9_datar[11:8]}       : r_t9_wordr[11:0] | r_t9_datar[11:0];
      r_t10_wordl  <= r_t9_bwel ? {4'b0, r_t9_wordl[11:8] | r_t9_datal[11:8], 4'b0} : r_t9_wordl[11:0] | r_t9_datal[11:0];
      r_t10_byter  <= r_t9_wordr[7:0] | r_t9_datar[7:0];
      r_t10_bytel  <= r_t9_wordl[7:0] | r_t9_datal[7:0];
      r_t10_bwer_d <= r_t9_bwer;
      r_t10_bwel_d <= r_t9_bwel;
      r_t10_posr_d <= r_t9_bwer ? (|r_t9_posr ? r_t9_posr - 6'h8 : 6'h38) : r_t9_posr;
      r_t10_posl_d <= r_t9_bwel ? (&r_t9_posl ? 6'h00 : r_t9_posl + 6'h8) : r_t9_posl;
      r_t11_datas  <= r_t10_MASK & (c_t10_datasr | c_t10_datasl | r_t10_datas);
// centrifuge pipeline registers
      r_t9_inA    <= r_t8_inA;
      r_t9_inB    <= r_t8_inB;
      r_t9_mixedr <= c_t8_mixedr;
      r_t9_mixedl <= c_t8_mixedl;
      r_t9_balr   <= c_t8_balr;
      r_t9_ball   <= c_t8_ball;
      r_t10_posr   <= c_t9_posr;
      r_t10_posl   <= c_t9_posl;
      r_t10_bwer   <= c_t9_posr[3] ^ r_t9_posr[3];
      r_t10_bwel   <= c_t9_posl[3] ^ r_t9_posl[3];
      r_t10_datar  <= c_t9_datar;
      r_t10_datal  <= c_t9_datal;
      r_t11_BBop   <= r_t10_BBop;
      r_t11_wordr  <= r_t10_bwer ? {8'b0, r_t10_wordr[11:8] | r_t10_datar[11:8]}       : r_t10_wordr[11:0] | r_t10_datar[11:0];
      r_t11_wordl  <= r_t10_bwel ? {4'b0, r_t10_wordl[11:8] | r_t10_datal[11:8], 4'b0} : r_t10_wordl[11:0] | r_t10_datal[11:0];
      r_t11_byter  <= r_t10_wordr[7:0] | r_t10_datar[7:0];
      r_t11_bytel  <= r_t10_wordl[7:0] | r_t10_datal[7:0];
      r_t11_bwer_d <= r_t10_bwer;
      r_t11_bwel_d <= r_t10_bwel;
      r_t11_posr_d <= r_t10_bwer ? (|r_t10_posr ? r_t10_posr - 6'h8 : 6'h38) : r_t10_posr;
      r_t11_posl_d <= r_t10_bwel ? (&r_t10_posl ? 6'h00 : r_t10_posl + 6'h8) : r_t10_posl;
      r_t12_datas  <= r_t11_MASK & (c_t11_datasr | c_t11_datasl | r_t11_datas);
// centrifuge pipeline registers
      r_t10_inA    <= r_t9_inA;
      r_t10_inB    <= r_t9_inB;
      r_t10_mixedr <= c_t9_mixedr;
      r_t10_mixedl <= c_t9_mixedl;
      r_t10_balr   <= c_t9_balr;
      r_t10_ball   <= c_t9_ball;
      r_t11_posr   <= c_t10_posr;
      r_t11_posl   <= c_t10_posl;
      r_t11_bwer   <= c_t10_posr[3] ^ r_t10_posr[3];
      r_t11_bwel   <= c_t10_posl[3] ^ r_t10_posl[3];
      r_t11_datar  <= c_t10_datar;
      r_t11_datal  <= c_t10_datal;
      r_t12_BBop   <= r_t11_BBop;
      r_t12_wordr  <= r_t11_bwer ? {8'b0, r_t11_wordr[11:8] | r_t11_datar[11:8]}       : r_t11_wordr[11:0] | r_t11_datar[11:0];
      r_t12_wordl  <= r_t11_bwel ? {4'b0, r_t11_wordl[11:8] | r_t11_datal[11:8], 4'b0} : r_t11_wordl[11:0] | r_t11_datal[11:0];
      r_t12_byter  <= r_t11_wordr[7:0] | r_t11_datar[7:0];
      r_t12_bytel  <= r_t11_wordl[7:0] | r_t11_datal[7:0];
      r_t12_bwer_d <= r_t11_bwer;
      r_t12_bwel_d <= r_t11_bwel;
      r_t12_posr_d <= r_t11_bwer ? (|r_t11_posr ? r_t11_posr - 6'h8 : 6'h38) : r_t11_posr;
      r_t12_posl_d <= r_t11_bwel ? (&r_t11_posl ? 6'h00 : r_t11_posl + 6'h8) : r_t11_posl;
      r_t13_datas  <= r_t12_MASK & (c_t12_datasr | c_t12_datasl | r_t12_datas);
// centrifuge pipeline registers
      r_t11_inA    <= r_t10_inA;
      r_t11_inB    <= r_t10_inB;
      r_t11_mixedr <= c_t10_mixedr;
      r_t11_mixedl <= c_t10_mixedl;
      r_t11_balr   <= c_t10_balr;
      r_t11_ball   <= c_t10_ball;
      r_t12_posr   <= c_t11_posr;
      r_t12_posl   <= c_t11_posl;
      r_t12_bwer   <= c_t11_posr[3] ^ r_t11_posr[3];
      r_t12_bwel   <= c_t11_posl[3] ^ r_t11_posl[3];
      r_t12_datar  <= c_t11_datar;
      r_t12_datal  <= c_t11_datal;
      r_t13_BBop   <= r_t12_BBop;
      r_t13_wordr  <= r_t12_bwer ? {8'b0, r_t12_wordr[11:8] | r_t12_datar[11:8]}       : r_t12_wordr[11:0] | r_t12_datar[11:0];
      r_t13_wordl  <= r_t12_bwel ? {4'b0, r_t12_wordl[11:8] | r_t12_datal[11:8], 4'b0} : r_t12_wordl[11:0] | r_t12_datal[11:0];
      r_t13_byter  <= r_t12_wordr[7:0] | r_t12_datar[7:0];
      r_t13_bytel  <= r_t12_wordl[7:0] | r_t12_datal[7:0];
      r_t13_bwer_d <= r_t12_bwer;
      r_t13_bwel_d <= r_t12_bwel;
      r_t13_posr_d <= r_t12_bwer ? (|r_t12_posr ? r_t12_posr - 6'h8 : 6'h38) : r_t12_posr;
      r_t13_posl_d <= r_t12_bwel ? (&r_t12_posl ? 6'h00 : r_t12_posl + 6'h8) : r_t12_posl;
      r_t14_datas  <= r_t13_MASK & (c_t13_datasr | c_t13_datasl | r_t13_datas);
// centrifuge pipeline registers
      r_t12_inA    <= r_t11_inA;
      r_t12_inB    <= r_t11_inB;
      r_t12_mixedr <= c_t11_mixedr;
      r_t12_mixedl <= c_t11_mixedl;
      r_t12_balr   <= c_t11_balr;
      r_t12_ball   <= c_t11_ball;
      r_t13_posr   <= c_t12_posr;
      r_t13_posl   <= c_t12_posl;
      r_t13_bwer   <= c_t12_posr[3] ^ r_t12_posr[3];
      r_t13_bwel   <= c_t12_posl[3] ^ r_t12_posl[3];
      r_t13_datar  <= c_t12_datar;
      r_t13_datal  <= c_t12_datal;
      r_t14_BBop   <= r_t13_BBop;
      r_t14_wordr  <= r_t13_bwer ? {8'b0, r_t13_wordr[11:8] | r_t13_datar[11:8]}       : r_t13_wordr[11:0] | r_t13_datar[11:0];
      r_t14_wordl  <= r_t13_bwel ? {4'b0, r_t13_wordl[11:8] | r_t13_datal[11:8], 4'b0} : r_t13_wordl[11:0] | r_t13_datal[11:0];
      r_t14_byter  <= r_t13_wordr[7:0] | r_t13_datar[7:0];
      r_t14_bytel  <= r_t13_wordl[7:0] | r_t13_datal[7:0];
      r_t14_bwer_d <= r_t13_bwer;
      r_t14_bwel_d <= r_t13_bwel;
      r_t14_posr_d <= r_t13_bwer ? (|r_t13_posr ? r_t13_posr - 6'h8 : 6'h38) : r_t13_posr;
      r_t14_posl_d <= r_t13_bwel ? (&r_t13_posl ? 6'h00 : r_t13_posl + 6'h8) : r_t13_posl;
      r_t15_datas  <= r_t14_MASK & (c_t14_datasr | c_t14_datasl | r_t14_datas);
// centrifuge pipeline registers
      r_t13_inA    <= r_t12_inA;
      r_t13_inB    <= r_t12_inB;
      r_t13_mixedr <= c_t12_mixedr;
      r_t13_mixedl <= c_t12_mixedl;
      r_t13_balr   <= c_t12_balr;
      r_t13_ball   <= c_t12_ball;
      r_t14_posr   <= c_t13_posr;
      r_t14_posl   <= c_t13_posl;
      r_t14_bwer   <= c_t13_posr[3] ^ r_t13_posr[3];
      r_t14_bwel   <= c_t13_posl[3] ^ r_t13_posl[3];
      r_t14_datar  <= c_t13_datar;
      r_t14_datal  <= c_t13_datal;
      r_t15_BBop   <= r_t14_BBop;
      r_t15_wordr  <= r_t14_bwer ? {8'b0, r_t14_wordr[11:8] | r_t14_datar[11:8]}       : r_t14_wordr[11:0] | r_t14_datar[11:0];
      r_t15_wordl  <= r_t14_bwel ? {4'b0, r_t14_wordl[11:8] | r_t14_datal[11:8], 4'b0} : r_t14_wordl[11:0] | r_t14_datal[11:0];
      r_t15_byter  <= r_t14_wordr[7:0] | r_t14_datar[7:0];
      r_t15_bytel  <= r_t14_wordl[7:0] | r_t14_datal[7:0];
      r_t15_bwer_d <= r_t14_bwer;
      r_t15_bwel_d <= r_t14_bwel;
      r_t15_posr_d <= r_t14_bwer ? (|r_t14_posr ? r_t14_posr - 6'h8 : 6'h38) : r_t14_posr;
      r_t15_posl_d <= r_t14_bwel ? (&r_t14_posl ? 6'h00 : r_t14_posl + 6'h8) : r_t14_posl;
      r_t16_datas  <= r_t15_MASK & (c_t15_datasr | c_t15_datasl | r_t15_datas);
// centrifuge pipeline registers
      r_t14_inA    <= r_t13_inA;
      r_t14_inB    <= r_t13_inB;
      r_t14_mixedr <= c_t13_mixedr;
      r_t14_mixedl <= c_t13_mixedl;
      r_t14_balr   <= c_t13_balr;
      r_t14_ball   <= c_t13_ball;
      r_t15_posr   <= c_t14_posr;
      r_t15_posl   <= c_t14_posl;
      r_t15_bwer   <= c_t14_posr[3] ^ r_t14_posr[3];
      r_t15_bwel   <= c_t14_posl[3] ^ r_t14_posl[3];
      r_t15_datar  <= c_t14_datar;
      r_t15_datal  <= c_t14_datal;
      r_t16_BBop   <= r_t15_BBop;
      r_t16_wordr  <= r_t15_bwer ? {8'b0, r_t15_wordr[11:8] | r_t15_datar[11:8]}       : r_t15_wordr[11:0] | r_t15_datar[11:0];
      r_t16_wordl  <= r_t15_bwel ? {4'b0, r_t15_wordl[11:8] | r_t15_datal[11:8], 4'b0} : r_t15_wordl[11:0] | r_t15_datal[11:0];
      r_t16_byter  <= r_t15_wordr[7:0] | r_t15_datar[7:0];
      r_t16_bytel  <= r_t15_wordl[7:0] | r_t15_datal[7:0];
      r_t16_bwer_d <= r_t15_bwer;
      r_t16_bwel_d <= r_t15_bwel;
      r_t16_posr_d <= r_t15_bwer ? (|r_t15_posr ? r_t15_posr - 6'h8 : 6'h38) : r_t15_posr;
      r_t16_posl_d <= r_t15_bwel ? (&r_t15_posl ? 6'h00 : r_t15_posl + 6'h8) : r_t15_posl;
      r_t17_datas  <= r_t16_MASK & (c_t16_datasr | c_t16_datasl | r_t16_datas);
// centrifuge pipeline registers
      r_t15_inA    <= r_t14_inA;
      r_t15_inB    <= r_t14_inB;
      r_t15_mixedr <= c_t14_mixedr;
      r_t15_mixedl <= c_t14_mixedl;
      r_t15_balr   <= c_t14_balr;
      r_t15_ball   <= c_t14_ball;
      r_t16_posr   <= c_t15_posr;
      r_t16_posl   <= c_t15_posl;
      r_t16_bwer   <= c_t15_posr[3] ^ r_t15_posr[3];
      r_t16_bwel   <= c_t15_posl[3] ^ r_t15_posl[3];
      r_t16_datar  <= c_t15_datar;
      r_t16_datal  <= c_t15_datal;
      r_t17_BBop   <= r_t16_BBop;
      r_t17_wordr  <= r_t16_bwer ? {8'b0, r_t16_wordr[11:8] | r_t16_datar[11:8]}       : r_t16_wordr[11:0] | r_t16_datar[11:0];
      r_t17_wordl  <= r_t16_bwel ? {4'b0, r_t16_wordl[11:8] | r_t16_datal[11:8], 4'b0} : r_t16_wordl[11:0] | r_t16_datal[11:0];
      r_t17_byter  <= r_t16_wordr[7:0] | r_t16_datar[7:0];
      r_t17_bytel  <= r_t16_wordl[7:0] | r_t16_datal[7:0];
      r_t17_bwer_d <= r_t16_bwer;
      r_t17_bwel_d <= r_t16_bwel;
      r_t17_posr_d <= r_t16_bwer ? (|r_t16_posr ? r_t16_posr - 6'h8 : 6'h38) : r_t16_posr;
      r_t17_posl_d <= r_t16_bwel ? (&r_t16_posl ? 6'h00 : r_t16_posl + 6'h8) : r_t16_posl;
      r_t18_datas  <= r_t17_MASK & (c_t17_datasr | c_t17_datasl | r_t17_datas);
// centrifuge pipeline registers
      r_t16_inA    <= r_t15_inA;
      r_t16_inB    <= r_t15_inB;
      r_t16_mixedr <= c_t15_mixedr;
      r_t16_mixedl <= c_t15_mixedl;
      r_t16_balr   <= c_t15_balr;
      r_t16_ball   <= c_t15_ball;
      r_t17_posr   <= c_t16_posr;
      r_t17_posl   <= c_t16_posl;
      r_t17_bwer   <= c_t16_posr[3] ^ r_t16_posr[3];
      r_t17_bwel   <= c_t16_posl[3] ^ r_t16_posl[3];
      r_t17_datar  <= c_t16_datar;
      r_t17_datal  <= c_t16_datal;
      r_t18_BBop   <= r_t17_BBop;
      r_t18_wordr  <= r_t17_bwer ? {8'b0, r_t17_wordr[11:8] | r_t17_datar[11:8]}       : r_t17_wordr[11:0] | r_t17_datar[11:0];
      r_t18_wordl  <= r_t17_bwel ? {4'b0, r_t17_wordl[11:8] | r_t17_datal[11:8], 4'b0} : r_t17_wordl[11:0] | r_t17_datal[11:0];
      r_t18_byter  <= r_t17_wordr[7:0] | r_t17_datar[7:0];
      r_t18_bytel  <= r_t17_wordl[7:0] | r_t17_datal[7:0];
      r_t18_bwer_d <= r_t17_bwer;
      r_t18_bwel_d <= r_t17_bwel;
      r_t18_posr_d <= r_t17_bwer ? (|r_t17_posr ? r_t17_posr - 6'h8 : 6'h38) : r_t17_posr;
      r_t18_posl_d <= r_t17_bwel ? (&r_t17_posl ? 6'h00 : r_t17_posl + 6'h8) : r_t17_posl;
      r_t19_datas  <= r_t18_MASK & (c_t18_datasr | c_t18_datasl | r_t18_datas);
// centrifuge pipeline registers
      r_t17_inA    <= r_t16_inA;
      r_t17_inB    <= r_t16_inB;
      r_t17_mixedr <= c_t16_mixedr;
      r_t17_mixedl <= c_t16_mixedl;
      r_t17_balr   <= c_t16_balr;
      r_t17_ball   <= c_t16_ball;
      r_t18_posr   <= c_t17_posr;
      r_t18_posl   <= c_t17_posl;
      r_t18_bwer   <= c_t17_posr[3] ^ r_t17_posr[3];
      r_t18_bwel   <= c_t17_posl[3] ^ r_t17_posl[3];
      r_t18_datar  <= c_t17_datar;
      r_t18_datal  <= c_t17_datal;
      r_t19_BBop   <= r_t18_BBop;
      r_t19_wordr  <= r_t18_bwer ? {8'b0, r_t18_wordr[11:8] | r_t18_datar[11:8]}       : r_t18_wordr[11:0] | r_t18_datar[11:0];
      r_t19_wordl  <= r_t18_bwel ? {4'b0, r_t18_wordl[11:8] | r_t18_datal[11:8], 4'b0} : r_t18_wordl[11:0] | r_t18_datal[11:0];
      r_t19_byter  <= r_t18_wordr[7:0] | r_t18_datar[7:0];
      r_t19_bytel  <= r_t18_wordl[7:0] | r_t18_datal[7:0];
      r_t19_bwer_d <= r_t18_bwer;
      r_t19_bwel_d <= r_t18_bwel;
      r_t19_posr_d <= r_t18_bwer ? (|r_t18_posr ? r_t18_posr - 6'h8 : 6'h38) : r_t18_posr;
      r_t19_posl_d <= r_t18_bwel ? (&r_t18_posl ? 6'h00 : r_t18_posl + 6'h8) : r_t18_posl;
      r_t20_datas  <= r_t19_MASK & (c_t19_datasr | c_t19_datasl | r_t19_datas);
// centrifuge pipeline registers
      r_t18_inA    <= r_t17_inA;
      r_t18_inB    <= r_t17_inB;
      r_t18_mixedr <= c_t17_mixedr;
      r_t18_mixedl <= c_t17_mixedl;
      r_t18_balr   <= c_t17_balr;
      r_t18_ball   <= c_t17_ball;
      r_t19_posr   <= c_t18_posr;
      r_t19_posl   <= c_t18_posl;
      r_t19_bwer   <= c_t18_posr[3] ^ r_t18_posr[3];
      r_t19_bwel   <= c_t18_posl[3] ^ r_t18_posl[3];
      r_t19_datar  <= c_t18_datar;
      r_t19_datal  <= c_t18_datal;
      r_t20_BBop   <= r_t19_BBop;
      r_t20_wordr  <= r_t19_bwer ? {8'b0, r_t19_wordr[11:8] | r_t19_datar[11:8]}       : r_t19_wordr[11:0] | r_t19_datar[11:0];
      r_t20_wordl  <= r_t19_bwel ? {4'b0, r_t19_wordl[11:8] | r_t19_datal[11:8], 4'b0} : r_t19_wordl[11:0] | r_t19_datal[11:0];
      r_t20_byter  <= r_t19_wordr[7:0] | r_t19_datar[7:0];
      r_t20_bytel  <= r_t19_wordl[7:0] | r_t19_datal[7:0];
      r_t20_bwer_d <= r_t19_bwer;
      r_t20_bwel_d <= r_t19_bwel;
      r_t20_posr_d <= r_t19_bwer ? (|r_t19_posr ? r_t19_posr - 6'h8 : 6'h38) : r_t19_posr;
      r_t20_posl_d <= r_t19_bwel ? (&r_t19_posl ? 6'h00 : r_t19_posl + 6'h8) : r_t19_posl;
      r_t21_datas  <= r_t20_MASK & (c_t20_datasr | c_t20_datasl | r_t20_datas);
// centrifuge pipeline registers
      r_t19_inA    <= r_t18_inA;
      r_t19_inB    <= r_t18_inB;
      r_t19_mixedr <= c_t18_mixedr;
      r_t19_mixedl <= c_t18_mixedl;
      r_t19_balr   <= c_t18_balr;
      r_t19_ball   <= c_t18_ball;
      r_t20_posr   <= c_t19_posr;
      r_t20_posl   <= c_t19_posl;
      r_t20_bwer   <= c_t19_posr[3] ^ r_t19_posr[3];
      r_t20_bwel   <= c_t19_posl[3] ^ r_t19_posl[3];
      r_t20_datar  <= c_t19_datar;
      r_t20_datal  <= c_t19_datal;
      r_t21_BBop   <= r_t20_BBop;
      r_t21_wordr  <= {8'b0, r_t20_wordr[11:8] | r_t20_datar[11:8]};
      r_t21_wordl  <= {4'b0, r_t20_wordl[11:8] | r_t20_datal[11:8], 4'b0};
      r_t21_byter  <= r_t20_wordr[7:0] | r_t20_datar[7:0];
      r_t21_bytel  <= r_t20_wordl[7:0] | r_t20_datal[7:0];
      r_t21_bwer_d <= r_t20_bwer;
      r_t21_bwel_d <= r_t20_bwel;
      r_t21_posr_d <= r_t20_bwer ? (|r_t20_posr ? r_t20_posr - 6'h8 : 6'h38) : r_t20_posr;
      r_t21_posl_d <= r_t20_bwel ? (&r_t20_posl ? 6'h00 : r_t20_posl + 6'h8) : r_t20_posl;
      r_t22_datas  <= r_t21_MASK & (c_t21_datasr | c_t21_datasl | r_t21_datas);
      r_t20_pos_ovr <= c_t19_pos_ovr;
      r_t20_inA    <= r_t19_inA;
      r_t21_inA    <= r_t20_inA;
      r_t21_popc   <= 7'h40 - {r_t20_pos_ovr,r_t20_posr};
//      r_t22_popc   <= {57'b0, r_t21_popc} + r_t21_inA;
      r_t22_popc_l <= {26'b0, r_t21_popc} + {1'b0,r_t21_inA[31:0]};
      r_t22_popc_u0 <= r_t21_inA[63:32];
      r_t22_popc_u1 <= r_t21_inA[63:32] + 32'b1;
      r_t22_posr_d <= |r_t21_posr_d ? r_t21_posr_d + 6'h8 : 6'h38;
      r_t22_posl_d <= &r_t21_posl_d ? 6'h00 : r_t21_posl_d - 6'h8;
      r_t22_byter  <= r_t21_wordr[7:0];
      r_t22_bytel  <= r_t21_wordl[7:0];
      r_t22_BBop   <= r_t21_BBop;
      r_t23_datas  <= r_t22_BBop ? {r_t22_popc_l[32] ? r_t22_popc_u1 : r_t22_popc_u0, r_t22_popc_l[31:0]} : r_t22_MASK & (c_t22_datasr | c_t22_datasl | r_t22_datas);
      r_t24_cfuge_out <= r_t23_datas;
      
   end

   /* ---------- debug & synopsys off blocks  ---------- */
   
endmodule // bbox_box_top
