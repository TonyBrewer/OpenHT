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
   parameter NUM_MC_PORTS    = 16,
   parameter RTNCTL_WIDTH    = 32,
`ifdef USER_IO_ENABLED
   parameter NUM_UIO_PORTS   = 32,
   parameter UIO_PORTS_WIDTH = 32,
`endif //USER_IO_ENABLED
   parameter AE_AE_WIDTH     = 32
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
   // User IO Interface(s)
   //
`ifdef USER_IO_ENABLED
   output [NUM_UIO_PORTS*1-1 :0] uio_rq_vld,
   output [NUM_UIO_PORTS*UIO_PORTS_WIDTH-1:0] uio_rq_data,
   input  [NUM_UIO_PORTS*1-1 :0] uio_rq_afull,

   input  [NUM_UIO_PORTS*1-1 :0] uio_rs_vld,
   input  [NUM_UIO_PORTS*UIO_PORTS_WIDTH-1:0] uio_rs_data,
   output [NUM_UIO_PORTS*1-1 :0] uio_rs_afull,
`endif

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

`ifdef CNY_PLATFORM_TYPE2
   localparam            NUM_CAES = 1;
   localparam             MC_XBAR = 1;
`endif //CNY_PLATFORM_TYPE2

    // unused
    assign mc_rq_flush = 'b0;
    assign csr_rd_ack = csr_rd_vld;
    assign csr_rd_data = 'b0;


    localparam IS_WX = CNY_PDK_PLATFORM == "wx-690" ||
		       CNY_PDK_PLATFORM == "wx-2000";
    localparam IS_WX2 = CNY_PDK_PLATFORM == "wx2-vu7p";

    localparam [3:0] NUM_AE = NUM_CAES;

    /*
     * Dispatch
     */
    wire [15:0]	dis_ctlQueWidth;
    wire [47:0]	dis_ctlQueBase;
    wire [7:0]	unitCnt;
    dispatch # (
	.FREQ(CLK_PERS_FREQ == "SYNC_CLK" ? (IS_WX ? 167 : (IS_WX2 ? 267 : 150)) :
	      CLK_PERS_FREQ == "SYNC_HALF_CLK" ? (IS_WX ? 83 : (IS_WX2 ? 133 : 75)) :
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
	.ctlQueWidth(dis_ctlQueWidth),
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
    wire [31:0]  i_xbarToMif_wrRspTid[31:0];
    wire [31:0]  i_xbarToMif_rdRspRdy, i_xbarToMif_wrRspRdy,
		 i_xbarToMif_reqFull, o_mifToXbar_reqRdy,
		 o_mifToXbar_rdRspFull, o_mifToXbar_wrRspFull;
    wire [95:0]	 i_xbarToMif_rdRsp[31:0];
    wire [148:0] o_mifToXbar_req[31:0];

`ifdef USER_IO_ENABLED
    wire [31:0]  i_portToAu_uio_Rdy, o_portToAu_uio_AFull;
    wire [UIO_PORTS_WIDTH-1:0] i_portToAu_uio_Data[31:0];
    wire [31:0]  o_auToPort_uio_Rdy, i_auToPort_uio_AFull;
    wire [UIO_PORTS_WIDTH-1:0] o_auToPort_uio_Data[31:0];
`endif

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
	.i_dispToHif_ctlQueWidth(dis_ctlQueWidth),
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

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$0(i_portToAu_uio_Rdy[0]),
	.i_portToAu_uio_Data$0(i_portToAu_uio_Data[0]),
	.o_portToAu_uio_AFull$0(o_portToAu_uio_AFull[0]),

	.o_auToPort_uio_Rdy$0(o_auToPort_uio_Rdy[0]),
	.o_auToPort_uio_Data$0(o_auToPort_uio_Data[0]),
	.i_auToPort_uio_AFull$0(i_auToPort_uio_AFull[0]),
`endif
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

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$1(i_portToAu_uio_Rdy[1]),
	.i_portToAu_uio_Data$1(i_portToAu_uio_Data[1]),
	.o_portToAu_uio_AFull$1(o_portToAu_uio_AFull[1]),

	.o_auToPort_uio_Rdy$1(o_auToPort_uio_Rdy[1]),
	.o_auToPort_uio_Data$1(o_auToPort_uio_Data[1]),
	.i_auToPort_uio_AFull$1(i_auToPort_uio_AFull[1]),
`endif
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

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$2(i_portToAu_uio_Rdy[2]),
	.i_portToAu_uio_Data$2(i_portToAu_uio_Data[2]),
	.o_portToAu_uio_AFull$2(o_portToAu_uio_AFull[2]),

	.o_auToPort_uio_Rdy$2(o_auToPort_uio_Rdy[2]),
	.o_auToPort_uio_Data$2(o_auToPort_uio_Data[2]),
	.i_auToPort_uio_AFull$2(i_auToPort_uio_AFull[2]),
`endif
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

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$3(i_portToAu_uio_Rdy[3]),
	.i_portToAu_uio_Data$3(i_portToAu_uio_Data[3]),
	.o_portToAu_uio_AFull$3(o_portToAu_uio_AFull[3]),

	.o_auToPort_uio_Rdy$3(o_auToPort_uio_Rdy[3]),
	.o_auToPort_uio_Data$3(o_auToPort_uio_Data[3]),
	.i_auToPort_uio_AFull$3(i_auToPort_uio_AFull[3]),
`endif
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

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$4(i_portToAu_uio_Rdy[4]),
	.i_portToAu_uio_Data$4(i_portToAu_uio_Data[4]),
	.o_portToAu_uio_AFull$4(o_portToAu_uio_AFull[4]),

	.o_auToPort_uio_Rdy$4(o_auToPort_uio_Rdy[4]),
	.o_auToPort_uio_Data$4(o_auToPort_uio_Data[4]),
	.i_auToPort_uio_AFull$4(i_auToPort_uio_AFull[4]),
`endif
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

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$5(i_portToAu_uio_Rdy[5]),
	.i_portToAu_uio_Data$5(i_portToAu_uio_Data[5]),
	.o_portToAu_uio_AFull$5(o_portToAu_uio_AFull[5]),

	.o_auToPort_uio_Rdy$5(o_auToPort_uio_Rdy[5]),
	.o_auToPort_uio_Data$5(o_auToPort_uio_Data[5]),
	.i_auToPort_uio_AFull$5(i_auToPort_uio_AFull[5]),
`endif
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

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$6(i_portToAu_uio_Rdy[6]),
	.i_portToAu_uio_Data$6(i_portToAu_uio_Data[6]),
	.o_portToAu_uio_AFull$6(o_portToAu_uio_AFull[6]),

	.o_auToPort_uio_Rdy$6(o_auToPort_uio_Rdy[6]),
	.o_auToPort_uio_Data$6(o_auToPort_uio_Data[6]),
	.i_auToPort_uio_AFull$6(i_auToPort_uio_AFull[6]),
`endif
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

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$7(i_portToAu_uio_Rdy[7]),
	.i_portToAu_uio_Data$7(i_portToAu_uio_Data[7]),
	.o_portToAu_uio_AFull$7(o_portToAu_uio_AFull[7]),

	.o_auToPort_uio_Rdy$7(o_auToPort_uio_Rdy[7]),
	.o_auToPort_uio_Data$7(o_auToPort_uio_Data[7]),
	.i_auToPort_uio_AFull$7(i_auToPort_uio_AFull[7]),
`endif
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

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$8(i_portToAu_uio_Rdy[8]),
	.i_portToAu_uio_Data$8(i_portToAu_uio_Data[8]),
	.o_portToAu_uio_AFull$8(o_portToAu_uio_AFull[8]),

	.o_auToPort_uio_Rdy$8(o_auToPort_uio_Rdy[8]),
	.o_auToPort_uio_Data$8(o_auToPort_uio_Data[8]),
	.i_auToPort_uio_AFull$8(i_auToPort_uio_AFull[8]),
`endif
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

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$9(i_portToAu_uio_Rdy[9]),
	.i_portToAu_uio_Data$9(i_portToAu_uio_Data[9]),
	.o_portToAu_uio_AFull$9(o_portToAu_uio_AFull[9]),

	.o_auToPort_uio_Rdy$9(o_auToPort_uio_Rdy[9]),
	.o_auToPort_uio_Data$9(o_auToPort_uio_Data[9]),
	.i_auToPort_uio_AFull$9(i_auToPort_uio_AFull[9]),
`endif
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

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$10(i_portToAu_uio_Rdy[10]),
	.i_portToAu_uio_Data$10(i_portToAu_uio_Data[10]),
	.o_portToAu_uio_AFull$10(o_portToAu_uio_AFull[10]),

	.o_auToPort_uio_Rdy$10(o_auToPort_uio_Rdy[10]),
	.o_auToPort_uio_Data$10(o_auToPort_uio_Data[10]),
	.i_auToPort_uio_AFull$10(i_auToPort_uio_AFull[10]),
`endif
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

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$11(i_portToAu_uio_Rdy[11]),
	.i_portToAu_uio_Data$11(i_portToAu_uio_Data[11]),
	.o_portToAu_uio_AFull$11(o_portToAu_uio_AFull[11]),

	.o_auToPort_uio_Rdy$11(o_auToPort_uio_Rdy[11]),
	.o_auToPort_uio_Data$11(o_auToPort_uio_Data[11]),
	.i_auToPort_uio_AFull$11(i_auToPort_uio_AFull[11]),
`endif
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

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$12(i_portToAu_uio_Rdy[12]),
	.i_portToAu_uio_Data$12(i_portToAu_uio_Data[12]),
	.o_portToAu_uio_AFull$12(o_portToAu_uio_AFull[12]),

	.o_auToPort_uio_Rdy$12(o_auToPort_uio_Rdy[12]),
	.o_auToPort_uio_Data$12(o_auToPort_uio_Data[12]),
	.i_auToPort_uio_AFull$12(i_auToPort_uio_AFull[12]),
`endif
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

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$13(i_portToAu_uio_Rdy[13]),
	.i_portToAu_uio_Data$13(i_portToAu_uio_Data[13]),
	.o_portToAu_uio_AFull$13(o_portToAu_uio_AFull[13]),

	.o_auToPort_uio_Rdy$13(o_auToPort_uio_Rdy[13]),
	.o_auToPort_uio_Data$13(o_auToPort_uio_Data[13]),
	.i_auToPort_uio_AFull$13(i_auToPort_uio_AFull[13]),
`endif
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

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$14(i_portToAu_uio_Rdy[14]),
	.i_portToAu_uio_Data$14(i_portToAu_uio_Data[14]),
	.o_portToAu_uio_AFull$14(o_portToAu_uio_AFull[14]),

	.o_auToPort_uio_Rdy$14(o_auToPort_uio_Rdy[14]),
	.o_auToPort_uio_Data$14(o_auToPort_uio_Data[14]),
	.i_auToPort_uio_AFull$14(i_auToPort_uio_AFull[14]),
`endif
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

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$15(i_portToAu_uio_Rdy[15]),
	.i_portToAu_uio_Data$15(i_portToAu_uio_Data[15]),
	.o_portToAu_uio_AFull$15(o_portToAu_uio_AFull[15]),

	.o_auToPort_uio_Rdy$15(o_auToPort_uio_Rdy[15]),
	.o_auToPort_uio_Data$15(o_auToPort_uio_Data[15]),
	.i_auToPort_uio_AFull$15(i_auToPort_uio_AFull[15]),
`endif
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$16(i_xbarToMif_rdRsp[16]),
	.i_xbarToMif_rdRspRdy$16(i_xbarToMif_rdRspRdy[16]),
	.i_xbarToMif_reqFull$16(i_xbarToMif_reqFull[16]),
	.i_xbarToMif_wrRspRdy$16(i_xbarToMif_wrRspRdy[16]),
	.i_xbarToMif_wrRspTid$16(i_xbarToMif_wrRspTid[16]),

	.o_mifToXbar_rdRspFull$16(o_mifToXbar_rdRspFull[16]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$16(o_mifToXbar_req[16]),
	.o_mifToXbar_reqRdy$16(o_mifToXbar_reqRdy[16]),
	.o_mifToXbar_wrRspFull$16(o_mifToXbar_wrRspFull[16]),

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$16(i_portToAu_uio_Rdy[16]),
	.i_portToAu_uio_Data$16(i_portToAu_uio_Data[16]),
	.o_portToAu_uio_AFull$16(o_portToAu_uio_AFull[16]),

	.o_auToPort_uio_Rdy$16(o_auToPort_uio_Rdy[16]),
	.o_auToPort_uio_Data$16(o_auToPort_uio_Data[16]),
	.i_auToPort_uio_AFull$16(i_auToPort_uio_AFull[16]),
`endif
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$17(i_xbarToMif_rdRsp[17]),
	.i_xbarToMif_rdRspRdy$17(i_xbarToMif_rdRspRdy[17]),
	.i_xbarToMif_reqFull$17(i_xbarToMif_reqFull[17]),
	.i_xbarToMif_wrRspRdy$17(i_xbarToMif_wrRspRdy[17]),
	.i_xbarToMif_wrRspTid$17(i_xbarToMif_wrRspTid[17]),

	.o_mifToXbar_rdRspFull$17(o_mifToXbar_rdRspFull[17]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$17(o_mifToXbar_req[17]),
	.o_mifToXbar_reqRdy$17(o_mifToXbar_reqRdy[17]),
	.o_mifToXbar_wrRspFull$17(o_mifToXbar_wrRspFull[17]),

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$17(i_portToAu_uio_Rdy[17]),
	.i_portToAu_uio_Data$17(i_portToAu_uio_Data[17]),
	.o_portToAu_uio_AFull$17(o_portToAu_uio_AFull[17]),

	.o_auToPort_uio_Rdy$17(o_auToPort_uio_Rdy[17]),
	.o_auToPort_uio_Data$17(o_auToPort_uio_Data[17]),
	.i_auToPort_uio_AFull$17(i_auToPort_uio_AFull[17]),
`endif
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$18(i_xbarToMif_rdRsp[18]),
	.i_xbarToMif_rdRspRdy$18(i_xbarToMif_rdRspRdy[18]),
	.i_xbarToMif_reqFull$18(i_xbarToMif_reqFull[18]),
	.i_xbarToMif_wrRspRdy$18(i_xbarToMif_wrRspRdy[18]),
	.i_xbarToMif_wrRspTid$18(i_xbarToMif_wrRspTid[18]),

	.o_mifToXbar_rdRspFull$18(o_mifToXbar_rdRspFull[18]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$18(o_mifToXbar_req[18]),
	.o_mifToXbar_reqRdy$18(o_mifToXbar_reqRdy[18]),
	.o_mifToXbar_wrRspFull$18(o_mifToXbar_wrRspFull[18]),

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$18(i_portToAu_uio_Rdy[18]),
	.i_portToAu_uio_Data$18(i_portToAu_uio_Data[18]),
	.o_portToAu_uio_AFull$18(o_portToAu_uio_AFull[18]),

	.o_auToPort_uio_Rdy$18(o_auToPort_uio_Rdy[18]),
	.o_auToPort_uio_Data$18(o_auToPort_uio_Data[18]),
	.i_auToPort_uio_AFull$18(i_auToPort_uio_AFull[18]),
`endif
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$19(i_xbarToMif_rdRsp[19]),
	.i_xbarToMif_rdRspRdy$19(i_xbarToMif_rdRspRdy[19]),
	.i_xbarToMif_reqFull$19(i_xbarToMif_reqFull[19]),
	.i_xbarToMif_wrRspRdy$19(i_xbarToMif_wrRspRdy[19]),
	.i_xbarToMif_wrRspTid$19(i_xbarToMif_wrRspTid[19]),

	.o_mifToXbar_rdRspFull$19(o_mifToXbar_rdRspFull[19]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$19(o_mifToXbar_req[19]),
	.o_mifToXbar_reqRdy$19(o_mifToXbar_reqRdy[19]),
	.o_mifToXbar_wrRspFull$19(o_mifToXbar_wrRspFull[19]),

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$19(i_portToAu_uio_Rdy[19]),
	.i_portToAu_uio_Data$19(i_portToAu_uio_Data[19]),
	.o_portToAu_uio_AFull$19(o_portToAu_uio_AFull[19]),

	.o_auToPort_uio_Rdy$19(o_auToPort_uio_Rdy[19]),
	.o_auToPort_uio_Data$19(o_auToPort_uio_Data[19]),
	.i_auToPort_uio_AFull$19(i_auToPort_uio_AFull[19]),
`endif
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$20(i_xbarToMif_rdRsp[20]),
	.i_xbarToMif_rdRspRdy$20(i_xbarToMif_rdRspRdy[20]),
	.i_xbarToMif_reqFull$20(i_xbarToMif_reqFull[20]),
	.i_xbarToMif_wrRspRdy$20(i_xbarToMif_wrRspRdy[20]),
	.i_xbarToMif_wrRspTid$20(i_xbarToMif_wrRspTid[20]),

	.o_mifToXbar_rdRspFull$20(o_mifToXbar_rdRspFull[20]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$20(o_mifToXbar_req[20]),
	.o_mifToXbar_reqRdy$20(o_mifToXbar_reqRdy[20]),
	.o_mifToXbar_wrRspFull$20(o_mifToXbar_wrRspFull[20]),

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$20(i_portToAu_uio_Rdy[20]),
	.i_portToAu_uio_Data$20(i_portToAu_uio_Data[20]),
	.o_portToAu_uio_AFull$20(o_portToAu_uio_AFull[20]),

	.o_auToPort_uio_Rdy$20(o_auToPort_uio_Rdy[20]),
	.o_auToPort_uio_Data$20(o_auToPort_uio_Data[20]),
	.i_auToPort_uio_AFull$20(i_auToPort_uio_AFull[20]),
`endif
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$21(i_xbarToMif_rdRsp[21]),
	.i_xbarToMif_rdRspRdy$21(i_xbarToMif_rdRspRdy[21]),
	.i_xbarToMif_reqFull$21(i_xbarToMif_reqFull[21]),
	.i_xbarToMif_wrRspRdy$21(i_xbarToMif_wrRspRdy[21]),
	.i_xbarToMif_wrRspTid$21(i_xbarToMif_wrRspTid[21]),

	.o_mifToXbar_rdRspFull$21(o_mifToXbar_rdRspFull[21]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$21(o_mifToXbar_req[21]),
	.o_mifToXbar_reqRdy$21(o_mifToXbar_reqRdy[21]),
	.o_mifToXbar_wrRspFull$21(o_mifToXbar_wrRspFull[21]),

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$21(i_portToAu_uio_Rdy[21]),
	.i_portToAu_uio_Data$21(i_portToAu_uio_Data[21]),
	.o_portToAu_uio_AFull$21(o_portToAu_uio_AFull[21]),

	.o_auToPort_uio_Rdy$21(o_auToPort_uio_Rdy[21]),
	.o_auToPort_uio_Data$21(o_auToPort_uio_Data[21]),
	.i_auToPort_uio_AFull$21(i_auToPort_uio_AFull[21]),
`endif
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$22(i_xbarToMif_rdRsp[22]),
	.i_xbarToMif_rdRspRdy$22(i_xbarToMif_rdRspRdy[22]),
	.i_xbarToMif_reqFull$22(i_xbarToMif_reqFull[22]),
	.i_xbarToMif_wrRspRdy$22(i_xbarToMif_wrRspRdy[22]),
	.i_xbarToMif_wrRspTid$22(i_xbarToMif_wrRspTid[22]),

	.o_mifToXbar_rdRspFull$22(o_mifToXbar_rdRspFull[22]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$22(o_mifToXbar_req[22]),
	.o_mifToXbar_reqRdy$22(o_mifToXbar_reqRdy[22]),
	.o_mifToXbar_wrRspFull$22(o_mifToXbar_wrRspFull[22]),

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$22(i_portToAu_uio_Rdy[22]),
	.i_portToAu_uio_Data$22(i_portToAu_uio_Data[22]),
	.o_portToAu_uio_AFull$22(o_portToAu_uio_AFull[22]),

	.o_auToPort_uio_Rdy$22(o_auToPort_uio_Rdy[22]),
	.o_auToPort_uio_Data$22(o_auToPort_uio_Data[22]),
	.i_auToPort_uio_AFull$22(i_auToPort_uio_AFull[22]),
`endif
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$23(i_xbarToMif_rdRsp[23]),
	.i_xbarToMif_rdRspRdy$23(i_xbarToMif_rdRspRdy[23]),
	.i_xbarToMif_reqFull$23(i_xbarToMif_reqFull[23]),
	.i_xbarToMif_wrRspRdy$23(i_xbarToMif_wrRspRdy[23]),
	.i_xbarToMif_wrRspTid$23(i_xbarToMif_wrRspTid[23]),

	.o_mifToXbar_rdRspFull$23(o_mifToXbar_rdRspFull[23]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$23(o_mifToXbar_req[23]),
	.o_mifToXbar_reqRdy$23(o_mifToXbar_reqRdy[23]),
	.o_mifToXbar_wrRspFull$23(o_mifToXbar_wrRspFull[23]),

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$23(i_portToAu_uio_Rdy[23]),
	.i_portToAu_uio_Data$23(i_portToAu_uio_Data[23]),
	.o_portToAu_uio_AFull$23(o_portToAu_uio_AFull[23]),

	.o_auToPort_uio_Rdy$23(o_auToPort_uio_Rdy[23]),
	.o_auToPort_uio_Data$23(o_auToPort_uio_Data[23]),
	.i_auToPort_uio_AFull$23(i_auToPort_uio_AFull[23]),
`endif
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$24(i_xbarToMif_rdRsp[24]),
	.i_xbarToMif_rdRspRdy$24(i_xbarToMif_rdRspRdy[24]),
	.i_xbarToMif_reqFull$24(i_xbarToMif_reqFull[24]),
	.i_xbarToMif_wrRspRdy$24(i_xbarToMif_wrRspRdy[24]),
	.i_xbarToMif_wrRspTid$24(i_xbarToMif_wrRspTid[24]),

	.o_mifToXbar_rdRspFull$24(o_mifToXbar_rdRspFull[24]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$24(o_mifToXbar_req[24]),
	.o_mifToXbar_reqRdy$24(o_mifToXbar_reqRdy[24]),
	.o_mifToXbar_wrRspFull$24(o_mifToXbar_wrRspFull[24]),

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$24(i_portToAu_uio_Rdy[24]),
	.i_portToAu_uio_Data$24(i_portToAu_uio_Data[24]),
	.o_portToAu_uio_AFull$24(o_portToAu_uio_AFull[24]),

	.o_auToPort_uio_Rdy$24(o_auToPort_uio_Rdy[24]),
	.o_auToPort_uio_Data$24(o_auToPort_uio_Data[24]),
	.i_auToPort_uio_AFull$24(i_auToPort_uio_AFull[24]),
`endif
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$25(i_xbarToMif_rdRsp[25]),
	.i_xbarToMif_rdRspRdy$25(i_xbarToMif_rdRspRdy[25]),
	.i_xbarToMif_reqFull$25(i_xbarToMif_reqFull[25]),
	.i_xbarToMif_wrRspRdy$25(i_xbarToMif_wrRspRdy[25]),
	.i_xbarToMif_wrRspTid$25(i_xbarToMif_wrRspTid[25]),

	.o_mifToXbar_rdRspFull$25(o_mifToXbar_rdRspFull[25]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$25(o_mifToXbar_req[25]),
	.o_mifToXbar_reqRdy$25(o_mifToXbar_reqRdy[25]),
	.o_mifToXbar_wrRspFull$25(o_mifToXbar_wrRspFull[25]),

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$25(i_portToAu_uio_Rdy[25]),
	.i_portToAu_uio_Data$25(i_portToAu_uio_Data[25]),
	.o_portToAu_uio_AFull$25(o_portToAu_uio_AFull[25]),

	.o_auToPort_uio_Rdy$25(o_auToPort_uio_Rdy[25]),
	.o_auToPort_uio_Data$25(o_auToPort_uio_Data[25]),
	.i_auToPort_uio_AFull$25(i_auToPort_uio_AFull[25]),
`endif
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$26(i_xbarToMif_rdRsp[26]),
	.i_xbarToMif_rdRspRdy$26(i_xbarToMif_rdRspRdy[26]),
	.i_xbarToMif_reqFull$26(i_xbarToMif_reqFull[26]),
	.i_xbarToMif_wrRspRdy$26(i_xbarToMif_wrRspRdy[26]),
	.i_xbarToMif_wrRspTid$26(i_xbarToMif_wrRspTid[26]),

	.o_mifToXbar_rdRspFull$26(o_mifToXbar_rdRspFull[26]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$26(o_mifToXbar_req[26]),
	.o_mifToXbar_reqRdy$26(o_mifToXbar_reqRdy[26]),
	.o_mifToXbar_wrRspFull$26(o_mifToXbar_wrRspFull[26]),

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$26(i_portToAu_uio_Rdy[26]),
	.i_portToAu_uio_Data$26(i_portToAu_uio_Data[26]),
	.o_portToAu_uio_AFull$26(o_portToAu_uio_AFull[26]),

	.o_auToPort_uio_Rdy$26(o_auToPort_uio_Rdy[26]),
	.o_auToPort_uio_Data$26(o_auToPort_uio_Data[26]),
	.i_auToPort_uio_AFull$26(i_auToPort_uio_AFull[26]),
`endif
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$27(i_xbarToMif_rdRsp[27]),
	.i_xbarToMif_rdRspRdy$27(i_xbarToMif_rdRspRdy[27]),
	.i_xbarToMif_reqFull$27(i_xbarToMif_reqFull[27]),
	.i_xbarToMif_wrRspRdy$27(i_xbarToMif_wrRspRdy[27]),
	.i_xbarToMif_wrRspTid$27(i_xbarToMif_wrRspTid[27]),

	.o_mifToXbar_rdRspFull$27(o_mifToXbar_rdRspFull[27]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$27(o_mifToXbar_req[27]),
	.o_mifToXbar_reqRdy$27(o_mifToXbar_reqRdy[27]),
	.o_mifToXbar_wrRspFull$27(o_mifToXbar_wrRspFull[27]),

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$27(i_portToAu_uio_Rdy[27]),
	.i_portToAu_uio_Data$27(i_portToAu_uio_Data[27]),
	.o_portToAu_uio_AFull$27(o_portToAu_uio_AFull[27]),

	.o_auToPort_uio_Rdy$27(o_auToPort_uio_Rdy[27]),
	.o_auToPort_uio_Data$27(o_auToPort_uio_Data[27]),
	.i_auToPort_uio_AFull$27(i_auToPort_uio_AFull[27]),
`endif
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$28(i_xbarToMif_rdRsp[28]),
	.i_xbarToMif_rdRspRdy$28(i_xbarToMif_rdRspRdy[28]),
	.i_xbarToMif_reqFull$28(i_xbarToMif_reqFull[28]),
	.i_xbarToMif_wrRspRdy$28(i_xbarToMif_wrRspRdy[28]),
	.i_xbarToMif_wrRspTid$28(i_xbarToMif_wrRspTid[28]),

	.o_mifToXbar_rdRspFull$28(o_mifToXbar_rdRspFull[28]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$28(o_mifToXbar_req[28]),
	.o_mifToXbar_reqRdy$28(o_mifToXbar_reqRdy[28]),
	.o_mifToXbar_wrRspFull$28(o_mifToXbar_wrRspFull[28]),

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$28(i_portToAu_uio_Rdy[28]),
	.i_portToAu_uio_Data$28(i_portToAu_uio_Data[28]),
	.o_portToAu_uio_AFull$28(o_portToAu_uio_AFull[28]),

	.o_auToPort_uio_Rdy$28(o_auToPort_uio_Rdy[28]),
	.o_auToPort_uio_Data$28(o_auToPort_uio_Data[28]),
	.i_auToPort_uio_AFull$28(i_auToPort_uio_AFull[28]),
`endif
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$29(i_xbarToMif_rdRsp[29]),
	.i_xbarToMif_rdRspRdy$29(i_xbarToMif_rdRspRdy[29]),
	.i_xbarToMif_reqFull$29(i_xbarToMif_reqFull[29]),
	.i_xbarToMif_wrRspRdy$29(i_xbarToMif_wrRspRdy[29]),
	.i_xbarToMif_wrRspTid$29(i_xbarToMif_wrRspTid[29]),

	.o_mifToXbar_rdRspFull$29(o_mifToXbar_rdRspFull[29]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$29(o_mifToXbar_req[29]),
	.o_mifToXbar_reqRdy$29(o_mifToXbar_reqRdy[29]),
	.o_mifToXbar_wrRspFull$29(o_mifToXbar_wrRspFull[29]),

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$29(i_portToAu_uio_Rdy[29]),
	.i_portToAu_uio_Data$29(i_portToAu_uio_Data[29]),
	.o_portToAu_uio_AFull$29(o_portToAu_uio_AFull[29]),

	.o_auToPort_uio_Rdy$29(o_auToPort_uio_Rdy[29]),
	.o_auToPort_uio_Data$29(o_auToPort_uio_Data[29]),
	.i_auToPort_uio_AFull$29(i_auToPort_uio_AFull[29]),
`endif
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$30(i_xbarToMif_rdRsp[30]),
	.i_xbarToMif_rdRspRdy$30(i_xbarToMif_rdRspRdy[30]),
	.i_xbarToMif_reqFull$30(i_xbarToMif_reqFull[30]),
	.i_xbarToMif_wrRspRdy$30(i_xbarToMif_wrRspRdy[30]),
	.i_xbarToMif_wrRspTid$30(i_xbarToMif_wrRspTid[30]),

	.o_mifToXbar_rdRspFull$30(o_mifToXbar_rdRspFull[30]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$30(o_mifToXbar_req[30]),
	.o_mifToXbar_reqRdy$30(o_mifToXbar_reqRdy[30]),
	.o_mifToXbar_wrRspFull$30(o_mifToXbar_wrRspFull[30]),

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$30(i_portToAu_uio_Rdy[30]),
	.i_portToAu_uio_Data$30(i_portToAu_uio_Data[30]),
	.o_portToAu_uio_AFull$30(o_portToAu_uio_AFull[30]),

	.o_auToPort_uio_Rdy$30(o_auToPort_uio_Rdy[30]),
	.o_auToPort_uio_Data$30(o_auToPort_uio_Data[30]),
	.i_auToPort_uio_AFull$30(i_auToPort_uio_AFull[30]),
`endif
	// {TID_W'tid, 64'data}
	.i_xbarToMif_rdRsp$31(i_xbarToMif_rdRsp[31]),
	.i_xbarToMif_rdRspRdy$31(i_xbarToMif_rdRspRdy[31]),
	.i_xbarToMif_reqFull$31(i_xbarToMif_reqFull[31]),
	.i_xbarToMif_wrRspRdy$31(i_xbarToMif_wrRspRdy[31]),
	.i_xbarToMif_wrRspTid$31(i_xbarToMif_wrRspTid[31]),

	.o_mifToXbar_rdRspFull$31(o_mifToXbar_rdRspFull[31]),
	// {2'size, 1'host, 2'cmd, TID_W'tid, 48'addr, 64'data}
	.o_mifToXbar_req$31(o_mifToXbar_req[31]),
	.o_mifToXbar_reqRdy$31(o_mifToXbar_reqRdy[31]),
	.o_mifToXbar_wrRspFull$31(o_mifToXbar_wrRspFull[31]),

`ifdef USER_IO_ENABLED
	.i_portToAu_uio_Rdy$31(i_portToAu_uio_Rdy[31]),
	.i_portToAu_uio_Data$31(i_portToAu_uio_Data[31]),
	.o_portToAu_uio_AFull$31(o_portToAu_uio_AFull[31]),

	.o_auToPort_uio_Rdy$31(o_auToPort_uio_Rdy[31]),
	.o_auToPort_uio_Data$31(o_auToPort_uio_Data[31]),
	.i_auToPort_uio_AFull$31(i_auToPort_uio_AFull[31]),
`endif
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
`ifdef USER_IO_ENABLED
    for (n=0; n<NUM_UIO_PORTS; n=n+1) begin : g1
	assign i_portToAu_uio_Rdy[n]   = uio_rs_vld[n];
	assign i_portToAu_uio_Data[n]  = uio_rs_data[n*UIO_PORTS_WIDTH +: UIO_PORTS_WIDTH];
	assign uio_rs_afull[n]         = o_portToAu_uio_AFull[n];

	assign uio_rq_vld[n]           = o_auToPort_uio_Rdy[n];
	assign uio_rq_data[n*UIO_PORTS_WIDTH +: UIO_PORTS_WIDTH] = o_auToPort_uio_Data[n];
	assign i_auToPort_uio_AFull[n] = uio_rq_afull[n];
    end
`endif

    // unused
    for (n=NUM_MC_PORTS; n<32; n=n+1) begin : g2
	assign i_xbarToMif_rdRsp[n] = 0;
	assign i_xbarToMif_rdRspRdy[n] = 0;
	assign i_xbarToMif_reqFull[n] = 0;
	assign i_xbarToMif_wrRspRdy[n] = 0;
	assign i_xbarToMif_wrRspTid[n] = 0;
    end
`ifdef USER_IO_ENABLED
    for (n=NUM_UIO_PORTS; n<32; n=n+1) begin : g3
	assign i_portToAu_uio_Rdy[n] = 0;
	assign i_portToAu_uio_Data[n] = 0;
	assign i_auToPort_uio_AFull[n] = 0;
    end
`endif
    endgenerate

endmodule // cae_pers
