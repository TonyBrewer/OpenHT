/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
`timescale 1ns / 1ps

`include "pdk_fpga_defines.vh"

(* keep_hierarchy = "true" *)
module cae_pers #(
   parameter NUM_MC_PORTS = 16,
   parameter RTNCTL_WIDTH = 32,
   parameter AE_AE_WIDTH  = 32
)(
   //
   // Clocks and Resets
   //
   input		clk,		// personality clock
   input		clkhx,		// half-rate personality clock
   input		clk2x,		// 2x rate personality clock
   input		i_reset,	// global reset synchronized to clk

   //
   // Dispatch Interface
   //
   input		disp_inst_vld,
   input  [4:0]		disp_inst,
   input  [17:0]	disp_aeg_idx,
   input		disp_aeg_rd,
   input		disp_aeg_wr,
   input  [63:0]	disp_aeg_wr_data,

   output [17:0]	disp_aeg_cnt,
   output [15:0]	disp_exception,
   output [63:0]	disp_rtn_data,
   output		disp_rtn_data_vld,
   output		disp_idle,
   output		disp_stall,

   //
   // MC Interface(s)
   //
   output [NUM_MC_PORTS*1-1 :0]	mc_rq_vld,
   output [NUM_MC_PORTS*RTNCTL_WIDTH-1:0] mc_rq_rtnctl,
   output [NUM_MC_PORTS*64-1:0]	mc_rq_data,
   output [NUM_MC_PORTS*48-1:0]	mc_rq_vadr,
   output [NUM_MC_PORTS*2-1 :0]	mc_rq_size,
   output [NUM_MC_PORTS*3-1 :0]	mc_rq_cmd,
   output [NUM_MC_PORTS*4-1 :0]	mc_rq_scmd,
   input  [NUM_MC_PORTS*1-1 :0]	mc_rq_stall,

   input  [NUM_MC_PORTS*1-1 :0]	mc_rs_vld,
   input  [NUM_MC_PORTS*3-1 :0]	mc_rs_cmd,
   input  [NUM_MC_PORTS*4-1 :0]	mc_rs_scmd,
   input  [NUM_MC_PORTS*64-1:0]	mc_rs_data,
   input  [NUM_MC_PORTS*RTNCTL_WIDTH-1:0] mc_rs_rtnctl,
   output [NUM_MC_PORTS*1-1 :0]	mc_rs_stall,

   // Write flush
   output [NUM_MC_PORTS*1-1 :0]	mc_rq_flush,
   input  [NUM_MC_PORTS*1-1 :0]	mc_rs_flush_cmplt,

   //
   // Ae-Ae Interface
   //
`ifdef AE_AE_IF
   output		nxtae_rx_stall,
   input		nxtae_rx_clk,
   input  [AE_AE_WIDTH-1:0] nxtae_rx_data,
   input		nxtae_rx_vld,
   input		nxtae_tx_stall,
   output		nxtae_tx_clk,
   output [AE_AE_WIDTH-1:0] nxtae_tx_data,
   output		nxtae_tx_vld,
   output		prvae_rx_stall,
   input		prvae_rx_clk,
   input  [AE_AE_WIDTH-1:0] prvae_rx_data,
   input		prvae_rx_vld,
   input		prvae_tx_stall,
   output		prvae_tx_clk,
   output [AE_AE_WIDTH-1:0] prvae_tx_data,
   output		prvae_tx_vld,
`endif

   //
   // Control/Status Interface
   //
   input		csr_wr_vld,
   input		csr_rd_vld,
   input  [15:0]	csr_address,
   input  [63:0]	csr_wr_data,
   output		csr_rd_ack,
   output [63:0]	csr_rd_data,

   //
   // Miscellaneous
   //
   input  [3:0]		i_aeid
);

`include "pdk_fpga_param.vh"
`include "aemc_messages.vh"

    // unused
    assign mc_rq_flush = 'b0;
    assign csr_rd_ack = csr_rd_vld;
    assign csr_rd_data = 'b0;


    localparam IS_WX = CNY_PDK_PLATFORM == "wx-690" ||
		       CNY_PDK_PLATFORM == "wx-2000";

    localparam [3:0] NUM_AE = NUM_CAES;

    /*
     * Dispatch
     */
    wire [47:0]	dis_ctlQueBase;
    wire [7:0]	unitCnt;
    dispatch # (
	.FREQ(CLK_PERS_FREQ == "SYNC_CLK" ? (IS_WX ? 167 : 150) :
	      CLK_PERS_FREQ == "SYNC_HALF_CLK" ? (IS_WX ? 83 : 75) :
	      CLK_PERS_FREQ),
	.PART(PART_NUMBER)
    ) dis (
	.clk(clk),
	.reset(i_reset),

	.disp_inst_vld(disp_inst_vld),
	.disp_inst(disp_inst),
	.disp_aeg_idx(disp_aeg_idx),
	.disp_aeg_rd(disp_aeg_rd),
	.disp_aeg_wr(disp_aeg_wr),
	.disp_aeg_wr_data(disp_aeg_wr_data),

	.disp_aeg_cnt(disp_aeg_cnt),
	.disp_exception(disp_exception),
	.disp_rtn_data(disp_rtn_data),
	.disp_rtn_data_vld(disp_rtn_data_vld),
	.disp_idle(disp_idle),
	.disp_stall(disp_stall),

	.num_ae(NUM_AE),
	.num_units(unitCnt),
	.ctlQueBase(dis_ctlQueBase),
	.start(dis_start),
	.busy(hifToDisp_dispBusy),
	.reset_top(reset_top)
    );
`ifdef CNY_ALTERA
    global rsttop_bufg (.out(g_reset_top), .in(reset_top));
`else
    BUFG rsttop_bufg (.O(g_reset_top), .I(reset_top));
`endif

    /*
     * Hybrid-Threading Top
     */
    wire [31:0]  i_xbarToMif_wrRspTid[15:0];
    wire [15:0]  i_xbarToMif_rdRspRdy, i_xbarToMif_wrRspRdy,
		 i_xbarToMif_reqFull, o_mifToXbar_reqRdy,
		 o_mifToXbar_rdRspFull, o_mifToXbar_wrRspFull;
    wire [95:0]	 i_xbarToMif_rdRsp[15:0];
    wire [148:0] o_mifToXbar_req[15:0];

`ifndef AE_AE_IF
    wire	nxtae_rx_stall, nxtae_rx_vld, nxtae_tx_stall, nxtae_tx_vld,
		prvae_rx_stall, prvae_rx_vld, prvae_tx_stall, prvae_tx_vld;
    wire [AE_AE_WIDTH-1:0] nxtae_rx_data, nxtae_tx_data,
			   prvae_rx_data, prvae_tx_data;

    assign nxtae_rx_data = prvae_tx_data;
    assign nxtae_rx_vld = prvae_tx_vld;
    assign nxtae_tx_stall = prvae_rx_stall;
    assign prvae_rx_data = nxtae_tx_data;
    assign prvae_rx_vld = nxtae_tx_vld;
    assign prvae_tx_stall = nxtae_rx_stall;
`else
    // FIXME - used?
    assign nxtae_tx_clk = clk;
    assign prvae_tx_clk = clk;
`endif

    localparam TID_W = 32;

    CPersAeTop top (
	.i_aeId({4'b0, i_aeid}),
	.i_clock1x(clk),
	.i_clock2x(clk2x),
	.i_dispToHif_ctlQueBase(dis_ctlQueBase),
	.i_dispToHif_dispStart(dis_start),
	.i_reset(g_reset_top),
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$0(i_xbarToMif_rdRsp[0]),
	.i_xbarToMif_rdRspRdy$0(i_xbarToMif_rdRspRdy[0]),
	.i_xbarToMif_reqFull$0(i_xbarToMif_reqFull[0]),
	.i_xbarToMif_wrRspRdy$0(i_xbarToMif_wrRspRdy[0]),
	.i_xbarToMif_wrRspTid$0(i_xbarToMif_wrRspTid[0]),

	.o_mifToXbar_rdRspFull$0(o_mifToXbar_rdRspFull[0]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$0(o_mifToXbar_req[0]),
	.o_mifToXbar_reqRdy$0(o_mifToXbar_reqRdy[0]),
	.o_mifToXbar_wrRspFull$0(o_mifToXbar_wrRspFull[0]),
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$1(i_xbarToMif_rdRsp[1]),
	.i_xbarToMif_rdRspRdy$1(i_xbarToMif_rdRspRdy[1]),
	.i_xbarToMif_reqFull$1(i_xbarToMif_reqFull[1]),
	.i_xbarToMif_wrRspRdy$1(i_xbarToMif_wrRspRdy[1]),
	.i_xbarToMif_wrRspTid$1(i_xbarToMif_wrRspTid[1]),

	.o_mifToXbar_rdRspFull$1(o_mifToXbar_rdRspFull[1]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$1(o_mifToXbar_req[1]),
	.o_mifToXbar_reqRdy$1(o_mifToXbar_reqRdy[1]),
	.o_mifToXbar_wrRspFull$1(o_mifToXbar_wrRspFull[1]),
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$2(i_xbarToMif_rdRsp[2]),
	.i_xbarToMif_rdRspRdy$2(i_xbarToMif_rdRspRdy[2]),
	.i_xbarToMif_reqFull$2(i_xbarToMif_reqFull[2]),
	.i_xbarToMif_wrRspRdy$2(i_xbarToMif_wrRspRdy[2]),
	.i_xbarToMif_wrRspTid$2(i_xbarToMif_wrRspTid[2]),

	.o_mifToXbar_rdRspFull$2(o_mifToXbar_rdRspFull[2]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$2(o_mifToXbar_req[2]),
	.o_mifToXbar_reqRdy$2(o_mifToXbar_reqRdy[2]),
	.o_mifToXbar_wrRspFull$2(o_mifToXbar_wrRspFull[2]),
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$3(i_xbarToMif_rdRsp[3]),
	.i_xbarToMif_rdRspRdy$3(i_xbarToMif_rdRspRdy[3]),
	.i_xbarToMif_reqFull$3(i_xbarToMif_reqFull[3]),
	.i_xbarToMif_wrRspRdy$3(i_xbarToMif_wrRspRdy[3]),
	.i_xbarToMif_wrRspTid$3(i_xbarToMif_wrRspTid[3]),

	.o_mifToXbar_rdRspFull$3(o_mifToXbar_rdRspFull[3]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$3(o_mifToXbar_req[3]),
	.o_mifToXbar_reqRdy$3(o_mifToXbar_reqRdy[3]),
	.o_mifToXbar_wrRspFull$3(o_mifToXbar_wrRspFull[3]),
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$4(i_xbarToMif_rdRsp[4]),
	.i_xbarToMif_rdRspRdy$4(i_xbarToMif_rdRspRdy[4]),
	.i_xbarToMif_reqFull$4(i_xbarToMif_reqFull[4]),
	.i_xbarToMif_wrRspRdy$4(i_xbarToMif_wrRspRdy[4]),
	.i_xbarToMif_wrRspTid$4(i_xbarToMif_wrRspTid[4]),

	.o_mifToXbar_rdRspFull$4(o_mifToXbar_rdRspFull[4]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$4(o_mifToXbar_req[4]),
	.o_mifToXbar_reqRdy$4(o_mifToXbar_reqRdy[4]),
	.o_mifToXbar_wrRspFull$4(o_mifToXbar_wrRspFull[4]),
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$5(i_xbarToMif_rdRsp[5]),
	.i_xbarToMif_rdRspRdy$5(i_xbarToMif_rdRspRdy[5]),
	.i_xbarToMif_reqFull$5(i_xbarToMif_reqFull[5]),
	.i_xbarToMif_wrRspRdy$5(i_xbarToMif_wrRspRdy[5]),
	.i_xbarToMif_wrRspTid$5(i_xbarToMif_wrRspTid[5]),

	.o_mifToXbar_rdRspFull$5(o_mifToXbar_rdRspFull[5]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$5(o_mifToXbar_req[5]),
	.o_mifToXbar_reqRdy$5(o_mifToXbar_reqRdy[5]),
	.o_mifToXbar_wrRspFull$5(o_mifToXbar_wrRspFull[5]),
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$6(i_xbarToMif_rdRsp[6]),
	.i_xbarToMif_rdRspRdy$6(i_xbarToMif_rdRspRdy[6]),
	.i_xbarToMif_reqFull$6(i_xbarToMif_reqFull[6]),
	.i_xbarToMif_wrRspRdy$6(i_xbarToMif_wrRspRdy[6]),
	.i_xbarToMif_wrRspTid$6(i_xbarToMif_wrRspTid[6]),

	.o_mifToXbar_rdRspFull$6(o_mifToXbar_rdRspFull[6]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$6(o_mifToXbar_req[6]),
	.o_mifToXbar_reqRdy$6(o_mifToXbar_reqRdy[6]),
	.o_mifToXbar_wrRspFull$6(o_mifToXbar_wrRspFull[6]),
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$7(i_xbarToMif_rdRsp[7]),
	.i_xbarToMif_rdRspRdy$7(i_xbarToMif_rdRspRdy[7]),
	.i_xbarToMif_reqFull$7(i_xbarToMif_reqFull[7]),
	.i_xbarToMif_wrRspRdy$7(i_xbarToMif_wrRspRdy[7]),
	.i_xbarToMif_wrRspTid$7(i_xbarToMif_wrRspTid[7]),

	.o_mifToXbar_rdRspFull$7(o_mifToXbar_rdRspFull[7]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$7(o_mifToXbar_req[7]),
	.o_mifToXbar_reqRdy$7(o_mifToXbar_reqRdy[7]),
	.o_mifToXbar_wrRspFull$7(o_mifToXbar_wrRspFull[7]),
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$8(i_xbarToMif_rdRsp[8]),
	.i_xbarToMif_rdRspRdy$8(i_xbarToMif_rdRspRdy[8]),
	.i_xbarToMif_reqFull$8(i_xbarToMif_reqFull[8]),
	.i_xbarToMif_wrRspRdy$8(i_xbarToMif_wrRspRdy[8]),
	.i_xbarToMif_wrRspTid$8(i_xbarToMif_wrRspTid[8]),

	.o_mifToXbar_rdRspFull$8(o_mifToXbar_rdRspFull[8]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$8(o_mifToXbar_req[8]),
	.o_mifToXbar_reqRdy$8(o_mifToXbar_reqRdy[8]),
	.o_mifToXbar_wrRspFull$8(o_mifToXbar_wrRspFull[8]),
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$9(i_xbarToMif_rdRsp[9]),
	.i_xbarToMif_rdRspRdy$9(i_xbarToMif_rdRspRdy[9]),
	.i_xbarToMif_reqFull$9(i_xbarToMif_reqFull[9]),
	.i_xbarToMif_wrRspRdy$9(i_xbarToMif_wrRspRdy[9]),
	.i_xbarToMif_wrRspTid$9(i_xbarToMif_wrRspTid[9]),

	.o_mifToXbar_rdRspFull$9(o_mifToXbar_rdRspFull[9]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$9(o_mifToXbar_req[9]),
	.o_mifToXbar_reqRdy$9(o_mifToXbar_reqRdy[9]),
	.o_mifToXbar_wrRspFull$9(o_mifToXbar_wrRspFull[9]),
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$10(i_xbarToMif_rdRsp[10]),
	.i_xbarToMif_rdRspRdy$10(i_xbarToMif_rdRspRdy[10]),
	.i_xbarToMif_reqFull$10(i_xbarToMif_reqFull[10]),
	.i_xbarToMif_wrRspRdy$10(i_xbarToMif_wrRspRdy[10]),
	.i_xbarToMif_wrRspTid$10(i_xbarToMif_wrRspTid[10]),

	.o_mifToXbar_rdRspFull$10(o_mifToXbar_rdRspFull[10]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$10(o_mifToXbar_req[10]),
	.o_mifToXbar_reqRdy$10(o_mifToXbar_reqRdy[10]),
	.o_mifToXbar_wrRspFull$10(o_mifToXbar_wrRspFull[10]),
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$11(i_xbarToMif_rdRsp[11]),
	.i_xbarToMif_rdRspRdy$11(i_xbarToMif_rdRspRdy[11]),
	.i_xbarToMif_reqFull$11(i_xbarToMif_reqFull[11]),
	.i_xbarToMif_wrRspRdy$11(i_xbarToMif_wrRspRdy[11]),
	.i_xbarToMif_wrRspTid$11(i_xbarToMif_wrRspTid[11]),

	.o_mifToXbar_rdRspFull$11(o_mifToXbar_rdRspFull[11]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$11(o_mifToXbar_req[11]),
	.o_mifToXbar_reqRdy$11(o_mifToXbar_reqRdy[11]),
	.o_mifToXbar_wrRspFull$11(o_mifToXbar_wrRspFull[11]),
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$12(i_xbarToMif_rdRsp[12]),
	.i_xbarToMif_rdRspRdy$12(i_xbarToMif_rdRspRdy[12]),
	.i_xbarToMif_reqFull$12(i_xbarToMif_reqFull[12]),
	.i_xbarToMif_wrRspRdy$12(i_xbarToMif_wrRspRdy[12]),
	.i_xbarToMif_wrRspTid$12(i_xbarToMif_wrRspTid[12]),

	.o_mifToXbar_rdRspFull$12(o_mifToXbar_rdRspFull[12]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$12(o_mifToXbar_req[12]),
	.o_mifToXbar_reqRdy$12(o_mifToXbar_reqRdy[12]),
	.o_mifToXbar_wrRspFull$12(o_mifToXbar_wrRspFull[12]),
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$13(i_xbarToMif_rdRsp[13]),
	.i_xbarToMif_rdRspRdy$13(i_xbarToMif_rdRspRdy[13]),
	.i_xbarToMif_reqFull$13(i_xbarToMif_reqFull[13]),
	.i_xbarToMif_wrRspRdy$13(i_xbarToMif_wrRspRdy[13]),
	.i_xbarToMif_wrRspTid$13(i_xbarToMif_wrRspTid[13]),

	.o_mifToXbar_rdRspFull$13(o_mifToXbar_rdRspFull[13]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$13(o_mifToXbar_req[13]),
	.o_mifToXbar_reqRdy$13(o_mifToXbar_reqRdy[13]),
	.o_mifToXbar_wrRspFull$13(o_mifToXbar_wrRspFull[13]),
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$14(i_xbarToMif_rdRsp[14]),
	.i_xbarToMif_rdRspRdy$14(i_xbarToMif_rdRspRdy[14]),
	.i_xbarToMif_reqFull$14(i_xbarToMif_reqFull[14]),
	.i_xbarToMif_wrRspRdy$14(i_xbarToMif_wrRspRdy[14]),
	.i_xbarToMif_wrRspTid$14(i_xbarToMif_wrRspTid[14]),

	.o_mifToXbar_rdRspFull$14(o_mifToXbar_rdRspFull[14]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$14(o_mifToXbar_req[14]),
	.o_mifToXbar_reqRdy$14(o_mifToXbar_reqRdy[14]),
	.o_mifToXbar_wrRspFull$14(o_mifToXbar_wrRspFull[14]),
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$15(i_xbarToMif_rdRsp[15]),
	.i_xbarToMif_rdRspRdy$15(i_xbarToMif_rdRspRdy[15]),
	.i_xbarToMif_reqFull$15(i_xbarToMif_reqFull[15]),
	.i_xbarToMif_wrRspRdy$15(i_xbarToMif_wrRspRdy[15]),
	.i_xbarToMif_wrRspTid$15(i_xbarToMif_wrRspTid[15]),

	.o_mifToXbar_rdRspFull$15(o_mifToXbar_rdRspFull[15]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$15(o_mifToXbar_req[15]),
	.o_mifToXbar_reqRdy$15(o_mifToXbar_reqRdy[15]),
	.o_mifToXbar_wrRspFull$15(o_mifToXbar_wrRspFull[15]),
	.o_min_stall(nxtae_rx_stall),
	.i_min_data(nxtae_rx_data),
	.i_min_valid(nxtae_rx_vld),
	.i_mon_stall(nxtae_tx_stall),
	.o_mon_data(nxtae_tx_data),
	.o_mon_valid(nxtae_tx_vld),
	.o_mip_stall(prvae_rx_stall),
	.i_mip_data(prvae_rx_data),
	.i_mip_valid(prvae_rx_vld),
	.i_mop_stall(prvae_tx_stall),
	.o_mop_data(prvae_tx_data),
	.o_mop_valid(prvae_tx_vld),
	.o_hifToDisp_dispBusy(hifToDisp_dispBusy),
	.o_unitCnt(unitCnt)
    );

    genvar n;
    generate for (n=0; n<NUM_MC_PORTS; n=n+1) begin : g0
	assign i_xbarToMif_rdRsp[n] =
				{mc_rs_rtnctl[n*RTNCTL_WIDTH+3 +: RTNCTL_WIDTH-3],
				 mc_rs_cmd[n*3 +: 3] == MCAE_CMD_RD64_DATA ?
				     mc_rs_scmd[n*4 +: 3] : 3'b0,
				 mc_rs_data[n*64 +: 64]};

	assign i_xbarToMif_rdRspRdy[n] = mc_rs_vld[n] &&
				(mc_rs_cmd[n*3 +: 3] == MCAE_CMD_RD8_DATA ||
				 mc_rs_cmd[n*3 +: 3] == MCAE_CMD_RD64_DATA ||
				 mc_rs_cmd[n*3 +: 3] == MCAE_CMD_ATOMIC_DATA);

	assign i_xbarToMif_reqFull[n] = mc_rq_stall[n];

	assign i_xbarToMif_wrRspRdy[n] = mc_rs_vld[n] &&
				 (mc_rs_cmd[n*3 +: 3] == MCAE_CMD_WR_CMP ||
				  mc_rs_cmd[n*3 +: 3] == MCAE_CMD_WR64_CMP);

	assign i_xbarToMif_wrRspTid[n] = mc_rs_rtnctl[n*RTNCTL_WIDTH +: RTNCTL_WIDTH];

	assign mc_rs_stall[n]	= o_mifToXbar_rdRspFull[n] ||
				  o_mifToXbar_wrRspFull[n];

	assign mc_rq_vld[n]	= o_mifToXbar_reqRdy[n];

	assign mc_rq_rtnctl[n*RTNCTL_WIDTH +: RTNCTL_WIDTH] =
				o_mifToXbar_req[n][48+64 +: TID_W];

	assign mc_rq_data[n*64 +: 64] =
				o_mifToXbar_req[n][TID_W+48+64 +: 2] == 2'd3 ?
				64'h8000000000000000 :
				o_mifToXbar_req[n][0 +: 64];

	assign mc_rq_vadr[n*48 +: 48] =
				o_mifToXbar_req[n][64 +: 48];

	assign mc_rq_size[n*2 +: 2] = o_mifToXbar_req[n][1+2+TID_W+48+64 +: 2];

	assign mc_rq_cmd[n*3 +: 3] =
				o_mifToXbar_req[n][TID_W+48+64 +: 2] == 2'd0 ?
				    (~|o_mifToXbar_req[n][48+64 +: 3]     ? AEMC_CMD_RD8 :
									    AEMC_CMD_RD64) :
				o_mifToXbar_req[n][TID_W+48+64 +: 2] == 2'd1 ?
				    (~|o_mifToXbar_req[n][48+64 +: 3]     ? AEMC_CMD_WR8 :
									    AEMC_CMD_WR64) :
				AEMC_CMD_ATOMIC;

	assign mc_rq_scmd[n*4 +: 4] =
				mc_rq_cmd[n*3 +: 3] == AEMC_CMD_WR64 ?
				    (4'h7 & (o_mifToXbar_req[n][48+64 +: 3] + 1'b1)) :
				mc_rq_cmd[n*3 +: 3] == AEMC_CMD_ATOMIC ? AEMC_SCMD_ATOM_EXCH :
				4'b0;
    end

    // unused
    for (n=NUM_MC_PORTS; n<16; n=n+1) begin : g1
	assign i_xbarToMif_rdRsp[n] = 0;
	assign i_xbarToMif_rdRspRdy[n] = 0;
	assign i_xbarToMif_reqFull[n] = 0;
	assign i_xbarToMif_wrRspRdy[n] = 0;
	assign i_xbarToMif_wrRspTid[n] = 0;
    end
    endgenerate

endmodule // cae_pers
