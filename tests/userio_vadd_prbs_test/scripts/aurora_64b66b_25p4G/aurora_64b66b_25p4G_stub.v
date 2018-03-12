// Copyright 1986-2017 Xilinx, Inc. All Rights Reserved.
// --------------------------------------------------------------------------------
// Tool Version: Vivado v.2017.3.1 (lin64) Build 2035080 Fri Oct 20 14:20:00 MDT 2017
// Date        : Fri Mar  9 12:16:02 2018
// Host        : hwreg2.conveycomputer.com running 64-bit CentOS release 6.7 (Final)
// Command     : write_verilog -force -mode synth_stub
//               /netscratch/cbaronne/WX2_UIO_PRBS/DAY9/uvpt_100REF_25G_initclk/ip/aurora_64b66b_25p4G/aurora_64b66b_25p4G_stub.v
// Design      : aurora_64b66b_25p4G
// Purpose     : Stub declaration of top-level module interface
// Device      : xcvu7p-flvb2104-2L-e
// --------------------------------------------------------------------------------

// This empty module with port declaration file causes synthesis tools to infer a black box for IP.
// The synthesis directives are for Synopsys Synplify support to prevent IO buffer insertion.
// Please paste the declaration into a Verilog source file or add the file as an additional source.
(* X_CORE_INFO = "aurora_64b66b_v11_2_2, Coregen v14.3_ip3, Number of lanes = 1, Line rate is double25.4Gbps, Reference Clock is double100.0MHz, Interface is Framing, Flow Control is None and is operating in DUPLEX configuration" *)
module aurora_64b66b_25p4G(s_axi_tx_tdata, s_axi_tx_tlast, 
  s_axi_tx_tkeep, s_axi_tx_tvalid, s_axi_tx_tready, m_axi_rx_tdata, m_axi_rx_tlast, 
  m_axi_rx_tkeep, m_axi_rx_tvalid, rxp, rxn, txp, txn, refclk1_in, hard_err, soft_err, channel_up, 
  lane_up, mmcm_not_locked, user_clk, sync_clk, reset_pb, gt_rxcdrovrden_in, power_down, 
  loopback, pma_init, gt_pll_lock, gt_qpllclk_quad1_in, gt_qpllrefclk_quad1_in, 
  gt_qplllock_quad1_in, gt_qpllrefclklost_quad1, gt_to_common_qpllreset_out, 
  s_axi_awaddr, s_axi_awvalid, s_axi_awready, s_axi_wdata, s_axi_wstrb, s_axi_wvalid, 
  s_axi_wready, s_axi_bvalid, s_axi_bresp, s_axi_bready, s_axi_araddr, s_axi_arvalid, 
  s_axi_arready, s_axi_rdata, s_axi_rvalid, s_axi_rresp, s_axi_rready, init_clk, 
  link_reset_out, gt_powergood, sys_reset_out, bufg_gt_clr_out, tx_out_clk)
/* synthesis syn_black_box black_box_pad_pin="s_axi_tx_tdata[0:63],s_axi_tx_tlast,s_axi_tx_tkeep[0:7],s_axi_tx_tvalid,s_axi_tx_tready,m_axi_rx_tdata[0:63],m_axi_rx_tlast,m_axi_rx_tkeep[0:7],m_axi_rx_tvalid,rxp[0:0],rxn[0:0],txp[0:0],txn[0:0],refclk1_in,hard_err,soft_err,channel_up,lane_up[0:0],mmcm_not_locked,user_clk,sync_clk,reset_pb,gt_rxcdrovrden_in,power_down,loopback[2:0],pma_init,gt_pll_lock,gt_qpllclk_quad1_in,gt_qpllrefclk_quad1_in,gt_qplllock_quad1_in,gt_qpllrefclklost_quad1,gt_to_common_qpllreset_out,s_axi_awaddr[31:0],s_axi_awvalid,s_axi_awready,s_axi_wdata[31:0],s_axi_wstrb[3:0],s_axi_wvalid,s_axi_wready,s_axi_bvalid,s_axi_bresp[1:0],s_axi_bready,s_axi_araddr[31:0],s_axi_arvalid,s_axi_arready,s_axi_rdata[31:0],s_axi_rvalid,s_axi_rresp[1:0],s_axi_rready,init_clk,link_reset_out,gt_powergood[0:0],sys_reset_out,bufg_gt_clr_out,tx_out_clk" */;
  input [0:63]s_axi_tx_tdata;
  input s_axi_tx_tlast;
  input [0:7]s_axi_tx_tkeep;
  input s_axi_tx_tvalid;
  output s_axi_tx_tready;
  output [0:63]m_axi_rx_tdata;
  output m_axi_rx_tlast;
  output [0:7]m_axi_rx_tkeep;
  output m_axi_rx_tvalid;
  input [0:0]rxp;
  input [0:0]rxn;
  output [0:0]txp;
  output [0:0]txn;
  input refclk1_in;
  output hard_err;
  output soft_err;
  output channel_up;
  output [0:0]lane_up;
  input mmcm_not_locked;
  input user_clk;
  input sync_clk;
  input reset_pb;
  input gt_rxcdrovrden_in;
  input power_down;
  input [2:0]loopback;
  input pma_init;
  output gt_pll_lock;
  input gt_qpllclk_quad1_in;
  input gt_qpllrefclk_quad1_in;
  input gt_qplllock_quad1_in;
  input gt_qpllrefclklost_quad1;
  output gt_to_common_qpllreset_out;
  input [31:0]s_axi_awaddr;
  input s_axi_awvalid;
  output s_axi_awready;
  input [31:0]s_axi_wdata;
  input [3:0]s_axi_wstrb;
  input s_axi_wvalid;
  output s_axi_wready;
  output s_axi_bvalid;
  output [1:0]s_axi_bresp;
  input s_axi_bready;
  input [31:0]s_axi_araddr;
  input s_axi_arvalid;
  output s_axi_arready;
  output [31:0]s_axi_rdata;
  output s_axi_rvalid;
  output [1:0]s_axi_rresp;
  input s_axi_rready;
  input init_clk;
  output link_reset_out;
  output [0:0]gt_powergood;
  output sys_reset_out;
  output bufg_gt_clr_out;
  output tx_out_clk;
endmodule
