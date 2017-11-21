`timescale 1 ns / 1 ps

module user_io_xcvr_wrapper #
(
   parameter NUM_AUR_LINKS = 8
)(
   input 			 i_init_clk,
   
   input 			 i_reset,
   input 			 i_gt_reset,
   output [NUM_AUR_LINKS*1-1:0]  o_sys_reset,
   
   output [NUM_AUR_LINKS*1-1:0]  o_user_clk,
   
   input [7:0] 			 qsfp_refclk,
   
   input [7:0] 			 qsfp_cp_rxp,
   input [7:0] 			 qsfp_cp_rxn,
   output [7:0] 		 cp_qsfp_txp,
   output [7:0] 		 cp_qsfp_txn,

   // AXI Connections 
   input [NUM_AUR_LINKS*8-1:0] 	 i_s_axi_tx_tkeep,
   input [NUM_AUR_LINKS*64-1:0]  i_s_axi_tx_tdata,
   input [NUM_AUR_LINKS*1-1:0] 	 i_s_axi_tx_tlast,
   input [NUM_AUR_LINKS*1-1:0] 	 i_s_axi_tx_tvalid,
   output [NUM_AUR_LINKS*1-1:0]  o_s_axi_tx_tready,

   output [NUM_AUR_LINKS*8-1:0]  o_m_axi_rx_tkeep,
   output [NUM_AUR_LINKS*64-1:0] o_m_axi_rx_tdata,
   output [NUM_AUR_LINKS*1-1:0]  o_m_axi_rx_tlast,
   output [NUM_AUR_LINKS*1-1:0]  o_m_axi_rx_tvalid,

   // optional alarm indications 
   output [7:0] 		 qsfp_fatal_alarm,
   output [7:0] 		 qsfp_corr_alarm,

   // status ports
   output [7:0] 		 o_stat_chan_up,
   output [7:0] 		 o_stat_lane_up,
   output [7:0] 		 o_stat_gt_pwrgd
);

// leda XV2_1002 off Internally generated asynchronous reset detected (block level). 
// leda XV4_1002 off Internally generated asynchronous reset detected (block level). 
// leda XV2P_1002 off Internally generated asynchronous reset detected (block level). 
// leda XV2P_1003 off Multiple asynchronous resets detected in this unit/module.
// leda XV2_1003 off Multiple asynchronous resets detected in this unit/module.
// leda XV4_1003 off Multiple asynchronous resets detected in this unit/module.

   /*AUTOWIRE*/
   // Beginning of automatic wires (for undeclared instantiated-module outputs)
   wire			p0_s_axi_arready_uc;	// From aurora_userio_p0 of aurora_64b66b_25p78G.v
   wire			p0_s_axi_awready_uc;	// From aurora_userio_p0 of aurora_64b66b_25p78G.v
   wire [1:0]		p0_s_axi_bresp_uc;	// From aurora_userio_p0 of aurora_64b66b_25p78G.v
   wire			p0_s_axi_bvalid_uc;	// From aurora_userio_p0 of aurora_64b66b_25p78G.v
   wire [31:0]		p0_s_axi_rdata_uc;	// From aurora_userio_p0 of aurora_64b66b_25p78G.v
   wire [1:0]		p0_s_axi_rresp_uc;	// From aurora_userio_p0 of aurora_64b66b_25p78G.v
   wire			p0_s_axi_rvalid_uc;	// From aurora_userio_p0 of aurora_64b66b_25p78G.v
   wire			p0_s_axi_wready_uc;	// From aurora_userio_p0 of aurora_64b66b_25p78G.v
   wire			p1_s_axi_arready_uc;	// From aurora_userio_p1 of aurora_64b66b_25p78G.v
   wire			p1_s_axi_awready_uc;	// From aurora_userio_p1 of aurora_64b66b_25p78G.v
   wire [1:0]		p1_s_axi_bresp_uc;	// From aurora_userio_p1 of aurora_64b66b_25p78G.v
   wire			p1_s_axi_bvalid_uc;	// From aurora_userio_p1 of aurora_64b66b_25p78G.v
   wire [31:0]		p1_s_axi_rdata_uc;	// From aurora_userio_p1 of aurora_64b66b_25p78G.v
   wire [1:0]		p1_s_axi_rresp_uc;	// From aurora_userio_p1 of aurora_64b66b_25p78G.v
   wire			p1_s_axi_rvalid_uc;	// From aurora_userio_p1 of aurora_64b66b_25p78G.v
   wire			p1_s_axi_wready_uc;	// From aurora_userio_p1 of aurora_64b66b_25p78G.v
   wire			p2_s_axi_arready_uc;	// From aurora_userio_p2 of aurora_64b66b_25p78G.v
   wire			p2_s_axi_awready_uc;	// From aurora_userio_p2 of aurora_64b66b_25p78G.v
   wire [1:0]		p2_s_axi_bresp_uc;	// From aurora_userio_p2 of aurora_64b66b_25p78G.v
   wire			p2_s_axi_bvalid_uc;	// From aurora_userio_p2 of aurora_64b66b_25p78G.v
   wire [31:0]		p2_s_axi_rdata_uc;	// From aurora_userio_p2 of aurora_64b66b_25p78G.v
   wire [1:0]		p2_s_axi_rresp_uc;	// From aurora_userio_p2 of aurora_64b66b_25p78G.v
   wire			p2_s_axi_rvalid_uc;	// From aurora_userio_p2 of aurora_64b66b_25p78G.v
   wire			p2_s_axi_wready_uc;	// From aurora_userio_p2 of aurora_64b66b_25p78G.v
   wire			p3_s_axi_arready_uc;	// From aurora_userio_p3 of aurora_64b66b_25p78G.v
   wire			p3_s_axi_awready_uc;	// From aurora_userio_p3 of aurora_64b66b_25p78G.v
   wire [1:0]		p3_s_axi_bresp_uc;	// From aurora_userio_p3 of aurora_64b66b_25p78G.v
   wire			p3_s_axi_bvalid_uc;	// From aurora_userio_p3 of aurora_64b66b_25p78G.v
   wire [31:0]		p3_s_axi_rdata_uc;	// From aurora_userio_p3 of aurora_64b66b_25p78G.v
   wire [1:0]		p3_s_axi_rresp_uc;	// From aurora_userio_p3 of aurora_64b66b_25p78G.v
   wire			p3_s_axi_rvalid_uc;	// From aurora_userio_p3 of aurora_64b66b_25p78G.v
   wire			p3_s_axi_wready_uc;	// From aurora_userio_p3 of aurora_64b66b_25p78G.v
   wire			p4_s_axi_arready_uc;	// From aurora_userio_p4 of aurora_64b66b_25p78G.v
   wire			p4_s_axi_awready_uc;	// From aurora_userio_p4 of aurora_64b66b_25p78G.v
   wire [1:0]		p4_s_axi_bresp_uc;	// From aurora_userio_p4 of aurora_64b66b_25p78G.v
   wire			p4_s_axi_bvalid_uc;	// From aurora_userio_p4 of aurora_64b66b_25p78G.v
   wire [31:0]		p4_s_axi_rdata_uc;	// From aurora_userio_p4 of aurora_64b66b_25p78G.v
   wire [1:0]		p4_s_axi_rresp_uc;	// From aurora_userio_p4 of aurora_64b66b_25p78G.v
   wire			p4_s_axi_rvalid_uc;	// From aurora_userio_p4 of aurora_64b66b_25p78G.v
   wire			p4_s_axi_wready_uc;	// From aurora_userio_p4 of aurora_64b66b_25p78G.v
   wire			p5_s_axi_arready_uc;	// From aurora_userio_p5 of aurora_64b66b_25p78G.v
   wire			p5_s_axi_awready_uc;	// From aurora_userio_p5 of aurora_64b66b_25p78G.v
   wire [1:0]		p5_s_axi_bresp_uc;	// From aurora_userio_p5 of aurora_64b66b_25p78G.v
   wire			p5_s_axi_bvalid_uc;	// From aurora_userio_p5 of aurora_64b66b_25p78G.v
   wire [31:0]		p5_s_axi_rdata_uc;	// From aurora_userio_p5 of aurora_64b66b_25p78G.v
   wire [1:0]		p5_s_axi_rresp_uc;	// From aurora_userio_p5 of aurora_64b66b_25p78G.v
   wire			p5_s_axi_rvalid_uc;	// From aurora_userio_p5 of aurora_64b66b_25p78G.v
   wire			p5_s_axi_wready_uc;	// From aurora_userio_p5 of aurora_64b66b_25p78G.v
   wire			p6_s_axi_arready_uc;	// From aurora_userio_p6 of aurora_64b66b_25p78G.v
   wire			p6_s_axi_awready_uc;	// From aurora_userio_p6 of aurora_64b66b_25p78G.v
   wire [1:0]		p6_s_axi_bresp_uc;	// From aurora_userio_p6 of aurora_64b66b_25p78G.v
   wire			p6_s_axi_bvalid_uc;	// From aurora_userio_p6 of aurora_64b66b_25p78G.v
   wire [31:0]		p6_s_axi_rdata_uc;	// From aurora_userio_p6 of aurora_64b66b_25p78G.v
   wire [1:0]		p6_s_axi_rresp_uc;	// From aurora_userio_p6 of aurora_64b66b_25p78G.v
   wire			p6_s_axi_rvalid_uc;	// From aurora_userio_p6 of aurora_64b66b_25p78G.v
   wire			p6_s_axi_wready_uc;	// From aurora_userio_p6 of aurora_64b66b_25p78G.v
   wire			p7_s_axi_arready_uc;	// From aurora_userio_p7 of aurora_64b66b_25p78G.v
   wire			p7_s_axi_awready_uc;	// From aurora_userio_p7 of aurora_64b66b_25p78G.v
   wire [1:0]		p7_s_axi_bresp_uc;	// From aurora_userio_p7 of aurora_64b66b_25p78G.v
   wire			p7_s_axi_bvalid_uc;	// From aurora_userio_p7 of aurora_64b66b_25p78G.v
   wire [31:0]		p7_s_axi_rdata_uc;	// From aurora_userio_p7 of aurora_64b66b_25p78G.v
   wire [1:0]		p7_s_axi_rresp_uc;	// From aurora_userio_p7 of aurora_64b66b_25p78G.v
   wire			p7_s_axi_rvalid_uc;	// From aurora_userio_p7 of aurora_64b66b_25p78G.v
   wire			p7_s_axi_wready_uc;	// From aurora_userio_p7 of aurora_64b66b_25p78G.v
   // End of automatics

   
   wire [NUM_AUR_LINKS*1-1:0] 	userio_tx_out_clk;
   reg [NUM_AUR_LINKS*1-1:0] 	userio_mmcm_not_locked_out;
   wire 			userio_clk_0, userio_clk_1, userio_clk_2, userio_clk_3,
				userio_clk_4, userio_clk_5, userio_clk_6, userio_clk_7;
   wire 			userio_bufg_gt_clr_out_0, userio_bufg_gt_clr_out_1, userio_bufg_gt_clr_out_2, userio_bufg_gt_clr_out_3,
				userio_bufg_gt_clr_out_4, userio_bufg_gt_clr_out_5, userio_bufg_gt_clr_out_6, userio_bufg_gt_clr_out_7;

   wire [NUM_AUR_LINKS*1-1:0] 	userio_reset_logic;
   reg [NUM_AUR_LINKS*1-1:0] 	userio_reset;
   wire [NUM_AUR_LINKS*1-1:0] 	userio_gt_reset;

   wire [NUM_AUR_LINKS*1-1:0] 	userio_gt_pll_lock;
   wire [NUM_AUR_LINKS*1-1:0] 	userio_link_reset_out;

   wire 	qpll0_lock, qpll1_lock;
   wire 	qpll0_outclk, qpll1_outclk;
   wire 	qpll0_outrefclk, qpll1_outrefclk;
   wire 	qpll0_refclklost, qpll1_refclklost;
   wire 	qpll0_reset, qpll1_reset;
   wire [7:0] 	qpll_lock, qpll_outclk, qpll_outrefclk, qpll_refclklost, qpll_reset;
   
   assign qpll_lock = {qpll1_lock, qpll1_lock, qpll1_lock, qpll1_lock,
		       qpll0_lock, qpll0_lock, qpll0_lock, qpll0_lock};
   assign qpll_outclk = {qpll1_outclk, qpll1_outclk, qpll1_outclk, qpll1_outclk,
			 qpll0_outclk, qpll0_outclk, qpll0_outclk, qpll0_outclk};
   assign qpll_outrefclk = {qpll1_outrefclk, qpll1_outrefclk, qpll1_outrefclk, qpll1_outrefclk,
			    qpll0_outrefclk, qpll0_outrefclk, qpll0_outrefclk, qpll0_outrefclk};
   assign qpll_refclklost = {qpll1_refclklost, qpll1_refclklost, qpll1_refclklost, qpll1_refclklost,
			     qpll0_refclklost, qpll0_refclklost, qpll0_refclklost, qpll0_refclklost};
   assign qpll1_reset = |qpll_reset[7:4];
   assign qpll0_reset = |qpll_reset[3:0];

   // Clocking
   aurora_64b66b_25p78G_gt_common_wrapper
     ultrascale_gt_common_0 (/*AUTOINST*/
			     // Outputs
			     .qpll0_lock	(qpll0_lock),	 // Templated
			     .qpll0_outclk	(qpll0_outclk),	 // Templated
			     .qpll0_outrefclk	(qpll0_outrefclk), // Templated
			     .qpll0_refclklost	(qpll0_refclklost), // Templated
			     // Inputs
			     .qpll0_refclk	(qsfp_refclk[0]), // Templated
			     .qpll0_reset	(qpll0_reset),	 // Templated
			     .qpll0_lock_detclk	(i_init_clk));	 // Templated
   aurora_64b66b_25p78G_gt_common_wrapper
     ultrascale_gt_common_1 (/*AUTOINST*/
			     // Outputs
			     .qpll0_lock	(qpll1_lock),	 // Templated
			     .qpll0_outclk	(qpll1_outclk),	 // Templated
			     .qpll0_outrefclk	(qpll1_outrefclk), // Templated
			     .qpll0_refclklost	(qpll1_refclklost), // Templated
			     // Inputs
			     .qpll0_refclk	(qsfp_refclk[4]), // Templated
			     .qpll0_reset	(qpll1_reset),	 // Templated
			     .qpll0_lock_detclk	(i_init_clk));	 // Templated
   
   
   // Aurora IP Instantiation
   genvar link;
   generate for (link = 0; link < 8; link=link+1) begin : g0_link
      
      if (link == 0) begin : p0
         if (link < NUM_AUR_LINKS) begin : aurora_if
	    
	    // FIXME: Does each core need its own BUFG?
	    BUFG_GT user_clk_0_buf (
					  .CE(1'b1),
					  .CEMASK(1'b0),
					  .CLR(userio_bufg_gt_clr_out_0),
					  .CLRMASK(1'b0),
					  .DIV(3'b0),
					  .I(userio_tx_out_clk[0]),
					  .O(userio_clk_0));

	    // leda B_1400 off Asynchronous reset/set/load signal is not a primary input to the current unit
	    always @(posedge userio_clk_0, posedge userio_bufg_gt_clr_out_0) begin
	       if (userio_bufg_gt_clr_out_0) begin
		  userio_mmcm_not_locked_out[0] <= 1'b0;
	       end else begin
		  userio_mmcm_not_locked_out[0] <= 1'b1;
	       end
	    end
	    // leda B_1400 on Asynchronous reset/set/load signal is not a primary input to the current unit

	    assign o_user_clk[0] = userio_clk_0;

	    // FIXME: userio_reset must be asserted for 128*userio_clk(?) cycles. How do we ensure this?
	    userio_reset_logic userio_reset_logic_0 (.i_user_clk(i_init_clk), //userio_clk_0
							 .i_init_clk(i_init_clk),
							 .i_reset(i_reset),
							 .i_sys_reset(i_reset), //o_sys_reset[0]
							 .i_gt_reset(i_gt_reset),
							 .o_sys_reset(userio_reset_logic[0]),
							 .o_gt_reset(userio_gt_reset[0]));


	    always @ (posedge i_init_clk) begin //userio_clk_0
	       userio_reset[0] <= userio_reset_logic[0] | !userio_gt_pll_lock[0] | userio_link_reset_out[0];
	    end
	    
            aurora_64b66b_25p78G aurora_userio_p0
            (/*AUTOINST*/
	     // Outputs
	     .s_axi_tx_tready		(o_s_axi_tx_tready[0]),	 // Templated
	     .m_axi_rx_tdata		(o_m_axi_rx_tdata[63:0]), // Templated
	     .m_axi_rx_tkeep		(o_m_axi_rx_tkeep[7:0]), // Templated
	     .m_axi_rx_tlast		(o_m_axi_rx_tlast[0]),	 // Templated
	     .m_axi_rx_tvalid		(o_m_axi_rx_tvalid[0]),	 // Templated
	     .txp			(cp_qsfp_txp[0]),	 // Templated
	     .txn			(cp_qsfp_txn[0]),	 // Templated
	     .hard_err			(qsfp_fatal_alarm[0]),	 // Templated
	     .soft_err			(qsfp_corr_alarm[0]),	 // Templated
	     .channel_up		(o_stat_chan_up[0]),	 // Templated
	     .lane_up			(o_stat_lane_up[0]),	 // Templated
	     .gt_to_common_qpllreset_out(qpll_reset[0]),	 // Templated
	     .s_axi_rdata		(p0_s_axi_rdata_uc[31:0]), // Templated
	     .s_axi_awready		(p0_s_axi_awready_uc),	 // Templated
	     .s_axi_wready		(p0_s_axi_wready_uc),	 // Templated
	     .s_axi_bvalid		(p0_s_axi_bvalid_uc),	 // Templated
	     .s_axi_bresp		(p0_s_axi_bresp_uc[1:0]), // Templated
	     .s_axi_rresp		(p0_s_axi_rresp_uc[1:0]), // Templated
	     .s_axi_arready		(p0_s_axi_arready_uc),	 // Templated
	     .s_axi_rvalid		(p0_s_axi_rvalid_uc),	 // Templated
	     .link_reset_out		(userio_link_reset_out[0]), // Templated
	     .gt_powergood		(o_stat_gt_pwrgd[0]),	 // Templated
	     .gt_pll_lock		(userio_gt_pll_lock[0]), // Templated
	     .sys_reset_out		(o_sys_reset[0]),	 // Templated
	     .bufg_gt_clr_out		(userio_bufg_gt_clr_out_0), // Templated
	     .tx_out_clk		(userio_tx_out_clk[0]),	 // Templated
	     // Inputs
	     .s_axi_tx_tdata		(i_s_axi_tx_tdata[63:0]), // Templated
	     .s_axi_tx_tkeep		(i_s_axi_tx_tkeep[7:0]), // Templated
	     .s_axi_tx_tlast		(i_s_axi_tx_tlast[0]),	 // Templated
	     .s_axi_tx_tvalid		(i_s_axi_tx_tvalid[0]),	 // Templated
	     .rxp			(qsfp_cp_rxp[0]),	 // Templated
	     .rxn			(qsfp_cp_rxn[0]),	 // Templated
	     .refclk1_in		(qsfp_refclk[0]),	 // Templated
	     .mmcm_not_locked		(!userio_mmcm_not_locked_out[0]), // Templated
	     .user_clk			(userio_clk_0),		 // Templated
	     .sync_clk			(userio_clk_0),		 // Templated
	     .reset_pb			(userio_reset[0]),	 // Templated
	     .gt_rxcdrovrden_in		(1'b0),			 // Templated
	     .power_down		(1'b0),			 // Templated
	     .loopback			(3'b0),			 // Templated
	     .pma_init			(userio_gt_reset[0]),	 // Templated
	     .gt_qpllclk_quad1_in	(qpll_outclk[0]),	 // Templated
	     .gt_qpllrefclk_quad1_in	(qpll_outrefclk[0]),	 // Templated
	     .gt_qplllock_quad1_in	(qpll_lock[0]),		 // Templated
	     .gt_qpllrefclklost_quad1	(qpll_refclklost[0]),	 // Templated
	     .s_axi_awaddr		(32'b0),		 // Templated
	     .s_axi_araddr		(32'b0),		 // Templated
	     .s_axi_wdata		(32'b0),		 // Templated
	     .s_axi_wstrb		(4'b0),			 // Templated
	     .s_axi_awvalid		(1'b0),			 // Templated
	     .s_axi_rready		(1'b0),			 // Templated
	     .s_axi_bready		(1'b0),			 // Templated
	     .s_axi_arvalid		(1'b0),			 // Templated
	     .s_axi_wvalid		(1'b0),			 // Templated
	     .init_clk			(i_init_clk));		 // Templated
         end else begin : tieoff
	    assign cp_qsfp_txp[0] = 1'b0;
	    assign cp_qsfp_txn[0] = 1'b1;

	    assign qsfp_fatal_alarm[0] = 1'b0;
	    assign qsfp_corr_alarm[0] = 1'b0;

	    assign qpll_reset[0] = 1'b0;

	    assign o_stat_chan_up[0] = 1'b0;
	    assign o_stat_lane_up[0] = 1'b0;
	    assign o_stat_gt_pwrgd[0] = 1'b0;
         end
      end

      
      if (link == 1) begin : p1
         if (link < NUM_AUR_LINKS) begin : aurora_if
	    
	    // FIXME: Does each core need its own BUFG?
	    BUFG_GT user_clk_1_buf (
					  .CE(1'b1),
					  .CEMASK(1'b0),
					  .CLR(userio_bufg_gt_clr_out_1),
					  .CLRMASK(1'b0),
					  .DIV(3'b0),
					  .I(userio_tx_out_clk[1]),
					  .O(userio_clk_1));

	    // leda B_1400 off Asynchronous reset/set/load signal is not a primary input to the current unit
	    always @(posedge userio_clk_1, posedge userio_bufg_gt_clr_out_1) begin
	       if (userio_bufg_gt_clr_out_1) begin
		  userio_mmcm_not_locked_out[1] <= 1'b0;
	       end else begin
		  userio_mmcm_not_locked_out[1] <= 1'b1;
	       end
	    end
	    // leda B_1400 on Asynchronous reset/set/load signal is not a primary input to the current unit

	    assign o_user_clk[1] = userio_clk_1;

	    // FIXME: userio_reset must be asserted for 128*userio_clk(?) cycles. How do we ensure this?
	    userio_reset_logic userio_reset_logic_1 (.i_user_clk(i_init_clk), //userio_clk_1
							 .i_init_clk(i_init_clk),
							 .i_reset(i_reset),
							 .i_sys_reset(i_reset), //o_sys_reset[1]
							 .i_gt_reset(i_gt_reset),
							 .o_sys_reset(userio_reset_logic[1]),
							 .o_gt_reset(userio_gt_reset[1]));


	    always @ (posedge i_init_clk) begin //userio_clk_1
	       userio_reset[1] <= userio_reset_logic[1] | !userio_gt_pll_lock[1] | userio_link_reset_out[1];
	    end
	    
            aurora_64b66b_25p78G aurora_userio_p1
            (/*AUTOINST*/
	     // Outputs
	     .s_axi_tx_tready		(o_s_axi_tx_tready[1]),	 // Templated
	     .m_axi_rx_tdata		(o_m_axi_rx_tdata[127:64]), // Templated
	     .m_axi_rx_tkeep		(o_m_axi_rx_tkeep[15:8]), // Templated
	     .m_axi_rx_tlast		(o_m_axi_rx_tlast[1]),	 // Templated
	     .m_axi_rx_tvalid		(o_m_axi_rx_tvalid[1]),	 // Templated
	     .txp			(cp_qsfp_txp[1]),	 // Templated
	     .txn			(cp_qsfp_txn[1]),	 // Templated
	     .hard_err			(qsfp_fatal_alarm[1]),	 // Templated
	     .soft_err			(qsfp_corr_alarm[1]),	 // Templated
	     .channel_up		(o_stat_chan_up[1]),	 // Templated
	     .lane_up			(o_stat_lane_up[1]),	 // Templated
	     .gt_to_common_qpllreset_out(qpll_reset[1]),	 // Templated
	     .s_axi_rdata		(p1_s_axi_rdata_uc[31:0]), // Templated
	     .s_axi_awready		(p1_s_axi_awready_uc),	 // Templated
	     .s_axi_wready		(p1_s_axi_wready_uc),	 // Templated
	     .s_axi_bvalid		(p1_s_axi_bvalid_uc),	 // Templated
	     .s_axi_bresp		(p1_s_axi_bresp_uc[1:0]), // Templated
	     .s_axi_rresp		(p1_s_axi_rresp_uc[1:0]), // Templated
	     .s_axi_arready		(p1_s_axi_arready_uc),	 // Templated
	     .s_axi_rvalid		(p1_s_axi_rvalid_uc),	 // Templated
	     .link_reset_out		(userio_link_reset_out[1]), // Templated
	     .gt_powergood		(o_stat_gt_pwrgd[1]),	 // Templated
	     .gt_pll_lock		(userio_gt_pll_lock[1]), // Templated
	     .sys_reset_out		(o_sys_reset[1]),	 // Templated
	     .bufg_gt_clr_out		(userio_bufg_gt_clr_out_1), // Templated
	     .tx_out_clk		(userio_tx_out_clk[1]),	 // Templated
	     // Inputs
	     .s_axi_tx_tdata		(i_s_axi_tx_tdata[127:64]), // Templated
	     .s_axi_tx_tkeep		(i_s_axi_tx_tkeep[15:8]), // Templated
	     .s_axi_tx_tlast		(i_s_axi_tx_tlast[1]),	 // Templated
	     .s_axi_tx_tvalid		(i_s_axi_tx_tvalid[1]),	 // Templated
	     .rxp			(qsfp_cp_rxp[1]),	 // Templated
	     .rxn			(qsfp_cp_rxn[1]),	 // Templated
	     .refclk1_in		(qsfp_refclk[1]),	 // Templated
	     .mmcm_not_locked		(!userio_mmcm_not_locked_out[1]), // Templated
	     .user_clk			(userio_clk_1),		 // Templated
	     .sync_clk			(userio_clk_1),		 // Templated
	     .reset_pb			(userio_reset[1]),	 // Templated
	     .gt_rxcdrovrden_in		(1'b0),			 // Templated
	     .power_down		(1'b0),			 // Templated
	     .loopback			(3'b0),			 // Templated
	     .pma_init			(userio_gt_reset[1]),	 // Templated
	     .gt_qpllclk_quad1_in	(qpll_outclk[1]),	 // Templated
	     .gt_qpllrefclk_quad1_in	(qpll_outrefclk[1]),	 // Templated
	     .gt_qplllock_quad1_in	(qpll_lock[1]),		 // Templated
	     .gt_qpllrefclklost_quad1	(qpll_refclklost[1]),	 // Templated
	     .s_axi_awaddr		(32'b0),		 // Templated
	     .s_axi_araddr		(32'b0),		 // Templated
	     .s_axi_wdata		(32'b0),		 // Templated
	     .s_axi_wstrb		(4'b0),			 // Templated
	     .s_axi_awvalid		(1'b0),			 // Templated
	     .s_axi_rready		(1'b0),			 // Templated
	     .s_axi_bready		(1'b0),			 // Templated
	     .s_axi_arvalid		(1'b0),			 // Templated
	     .s_axi_wvalid		(1'b0),			 // Templated
	     .init_clk			(i_init_clk));		 // Templated
         end else begin : tieoff
	    assign cp_qsfp_txp[1] = 1'b0;
	    assign cp_qsfp_txn[1] = 1'b1;

	    assign qsfp_fatal_alarm[1] = 1'b0;
	    assign qsfp_corr_alarm[1] = 1'b0;

	    assign qpll_reset[1] = 1'b0;

	    assign o_stat_chan_up[1] = 1'b0;
	    assign o_stat_lane_up[1] = 1'b0;
	    assign o_stat_gt_pwrgd[1] = 1'b0;
         end
      end

      
      if (link == 2) begin : p2
         if (link < NUM_AUR_LINKS) begin : aurora_if
	    
	    // FIXME: Does each core need its own BUFG?
	    BUFG_GT user_clk_2_buf (
					  .CE(1'b1),
					  .CEMASK(1'b0),
					  .CLR(userio_bufg_gt_clr_out_2),
					  .CLRMASK(1'b0),
					  .DIV(3'b0),
					  .I(userio_tx_out_clk[2]),
					  .O(userio_clk_2));

	    // leda B_1400 off Asynchronous reset/set/load signal is not a primary input to the current unit
	    always @(posedge userio_clk_2, posedge userio_bufg_gt_clr_out_2) begin
	       if (userio_bufg_gt_clr_out_2) begin
		  userio_mmcm_not_locked_out[2] <= 1'b0;
	       end else begin
		  userio_mmcm_not_locked_out[2] <= 1'b1;
	       end
	    end
	    // leda B_1400 on Asynchronous reset/set/load signal is not a primary input to the current unit

	    assign o_user_clk[2] = userio_clk_2;

	    // FIXME: userio_reset must be asserted for 128*userio_clk(?) cycles. How do we ensure this?
	    userio_reset_logic userio_reset_logic_2 (.i_user_clk(i_init_clk), //userio_clk_2
							 .i_init_clk(i_init_clk),
							 .i_reset(i_reset),
							 .i_sys_reset(i_reset), //o_sys_reset[2]
							 .i_gt_reset(i_gt_reset),
							 .o_sys_reset(userio_reset_logic[2]),
							 .o_gt_reset(userio_gt_reset[2]));


	    always @ (posedge i_init_clk) begin //userio_clk_2
	       userio_reset[2] <= userio_reset_logic[2] | !userio_gt_pll_lock[2] | userio_link_reset_out[2];
	    end
	    
            aurora_64b66b_25p78G aurora_userio_p2
            (/*AUTOINST*/
	     // Outputs
	     .s_axi_tx_tready		(o_s_axi_tx_tready[2]),	 // Templated
	     .m_axi_rx_tdata		(o_m_axi_rx_tdata[191:128]), // Templated
	     .m_axi_rx_tkeep		(o_m_axi_rx_tkeep[23:16]), // Templated
	     .m_axi_rx_tlast		(o_m_axi_rx_tlast[2]),	 // Templated
	     .m_axi_rx_tvalid		(o_m_axi_rx_tvalid[2]),	 // Templated
	     .txp			(cp_qsfp_txp[2]),	 // Templated
	     .txn			(cp_qsfp_txn[2]),	 // Templated
	     .hard_err			(qsfp_fatal_alarm[2]),	 // Templated
	     .soft_err			(qsfp_corr_alarm[2]),	 // Templated
	     .channel_up		(o_stat_chan_up[2]),	 // Templated
	     .lane_up			(o_stat_lane_up[2]),	 // Templated
	     .gt_to_common_qpllreset_out(qpll_reset[2]),	 // Templated
	     .s_axi_rdata		(p2_s_axi_rdata_uc[31:0]), // Templated
	     .s_axi_awready		(p2_s_axi_awready_uc),	 // Templated
	     .s_axi_wready		(p2_s_axi_wready_uc),	 // Templated
	     .s_axi_bvalid		(p2_s_axi_bvalid_uc),	 // Templated
	     .s_axi_bresp		(p2_s_axi_bresp_uc[1:0]), // Templated
	     .s_axi_rresp		(p2_s_axi_rresp_uc[1:0]), // Templated
	     .s_axi_arready		(p2_s_axi_arready_uc),	 // Templated
	     .s_axi_rvalid		(p2_s_axi_rvalid_uc),	 // Templated
	     .link_reset_out		(userio_link_reset_out[2]), // Templated
	     .gt_powergood		(o_stat_gt_pwrgd[2]),	 // Templated
	     .gt_pll_lock		(userio_gt_pll_lock[2]), // Templated
	     .sys_reset_out		(o_sys_reset[2]),	 // Templated
	     .bufg_gt_clr_out		(userio_bufg_gt_clr_out_2), // Templated
	     .tx_out_clk		(userio_tx_out_clk[2]),	 // Templated
	     // Inputs
	     .s_axi_tx_tdata		(i_s_axi_tx_tdata[191:128]), // Templated
	     .s_axi_tx_tkeep		(i_s_axi_tx_tkeep[23:16]), // Templated
	     .s_axi_tx_tlast		(i_s_axi_tx_tlast[2]),	 // Templated
	     .s_axi_tx_tvalid		(i_s_axi_tx_tvalid[2]),	 // Templated
	     .rxp			(qsfp_cp_rxp[2]),	 // Templated
	     .rxn			(qsfp_cp_rxn[2]),	 // Templated
	     .refclk1_in		(qsfp_refclk[2]),	 // Templated
	     .mmcm_not_locked		(!userio_mmcm_not_locked_out[2]), // Templated
	     .user_clk			(userio_clk_2),		 // Templated
	     .sync_clk			(userio_clk_2),		 // Templated
	     .reset_pb			(userio_reset[2]),	 // Templated
	     .gt_rxcdrovrden_in		(1'b0),			 // Templated
	     .power_down		(1'b0),			 // Templated
	     .loopback			(3'b0),			 // Templated
	     .pma_init			(userio_gt_reset[2]),	 // Templated
	     .gt_qpllclk_quad1_in	(qpll_outclk[2]),	 // Templated
	     .gt_qpllrefclk_quad1_in	(qpll_outrefclk[2]),	 // Templated
	     .gt_qplllock_quad1_in	(qpll_lock[2]),		 // Templated
	     .gt_qpllrefclklost_quad1	(qpll_refclklost[2]),	 // Templated
	     .s_axi_awaddr		(32'b0),		 // Templated
	     .s_axi_araddr		(32'b0),		 // Templated
	     .s_axi_wdata		(32'b0),		 // Templated
	     .s_axi_wstrb		(4'b0),			 // Templated
	     .s_axi_awvalid		(1'b0),			 // Templated
	     .s_axi_rready		(1'b0),			 // Templated
	     .s_axi_bready		(1'b0),			 // Templated
	     .s_axi_arvalid		(1'b0),			 // Templated
	     .s_axi_wvalid		(1'b0),			 // Templated
	     .init_clk			(i_init_clk));		 // Templated
         end else begin : tieoff
	    assign cp_qsfp_txp[2] = 1'b0;
	    assign cp_qsfp_txn[2] = 1'b1;

	    assign qsfp_fatal_alarm[2] = 1'b0;
	    assign qsfp_corr_alarm[2] = 1'b0;

	    assign qpll_reset[2] = 1'b0;

	    assign o_stat_chan_up[2] = 1'b0;
	    assign o_stat_lane_up[2] = 1'b0;
	    assign o_stat_gt_pwrgd[2] = 1'b0;
         end
      end

      
      if (link == 3) begin : p3
         if (link < NUM_AUR_LINKS) begin : aurora_if
	    
	    // FIXME: Does each core need its own BUFG?
	    BUFG_GT user_clk_3_buf (
					  .CE(1'b1),
					  .CEMASK(1'b0),
					  .CLR(userio_bufg_gt_clr_out_3),
					  .CLRMASK(1'b0),
					  .DIV(3'b0),
					  .I(userio_tx_out_clk[3]),
					  .O(userio_clk_3));

	    // leda B_1400 off Asynchronous reset/set/load signal is not a primary input to the current unit
	    always @(posedge userio_clk_3, posedge userio_bufg_gt_clr_out_3) begin
	       if (userio_bufg_gt_clr_out_3) begin
		  userio_mmcm_not_locked_out[3] <= 1'b0;
	       end else begin
		  userio_mmcm_not_locked_out[3] <= 1'b1;
	       end
	    end
	    // leda B_1400 on Asynchronous reset/set/load signal is not a primary input to the current unit

	    assign o_user_clk[3] = userio_clk_3;

	    // FIXME: userio_reset must be asserted for 128*userio_clk(?) cycles. How do we ensure this?
	    userio_reset_logic userio_reset_logic_3 (.i_user_clk(i_init_clk), //userio_clk_3
							 .i_init_clk(i_init_clk),
							 .i_reset(i_reset),
							 .i_sys_reset(i_reset), //o_sys_reset[3]
							 .i_gt_reset(i_gt_reset),
							 .o_sys_reset(userio_reset_logic[3]),
							 .o_gt_reset(userio_gt_reset[3]));


	    always @ (posedge i_init_clk) begin //userio_clk_3
	       userio_reset[3] <= userio_reset_logic[3] | !userio_gt_pll_lock[3] | userio_link_reset_out[3];
	    end
	    
            aurora_64b66b_25p78G aurora_userio_p3
            (/*AUTOINST*/
	     // Outputs
	     .s_axi_tx_tready		(o_s_axi_tx_tready[3]),	 // Templated
	     .m_axi_rx_tdata		(o_m_axi_rx_tdata[255:192]), // Templated
	     .m_axi_rx_tkeep		(o_m_axi_rx_tkeep[31:24]), // Templated
	     .m_axi_rx_tlast		(o_m_axi_rx_tlast[3]),	 // Templated
	     .m_axi_rx_tvalid		(o_m_axi_rx_tvalid[3]),	 // Templated
	     .txp			(cp_qsfp_txp[3]),	 // Templated
	     .txn			(cp_qsfp_txn[3]),	 // Templated
	     .hard_err			(qsfp_fatal_alarm[3]),	 // Templated
	     .soft_err			(qsfp_corr_alarm[3]),	 // Templated
	     .channel_up		(o_stat_chan_up[3]),	 // Templated
	     .lane_up			(o_stat_lane_up[3]),	 // Templated
	     .gt_to_common_qpllreset_out(qpll_reset[3]),	 // Templated
	     .s_axi_rdata		(p3_s_axi_rdata_uc[31:0]), // Templated
	     .s_axi_awready		(p3_s_axi_awready_uc),	 // Templated
	     .s_axi_wready		(p3_s_axi_wready_uc),	 // Templated
	     .s_axi_bvalid		(p3_s_axi_bvalid_uc),	 // Templated
	     .s_axi_bresp		(p3_s_axi_bresp_uc[1:0]), // Templated
	     .s_axi_rresp		(p3_s_axi_rresp_uc[1:0]), // Templated
	     .s_axi_arready		(p3_s_axi_arready_uc),	 // Templated
	     .s_axi_rvalid		(p3_s_axi_rvalid_uc),	 // Templated
	     .link_reset_out		(userio_link_reset_out[3]), // Templated
	     .gt_powergood		(o_stat_gt_pwrgd[3]),	 // Templated
	     .gt_pll_lock		(userio_gt_pll_lock[3]), // Templated
	     .sys_reset_out		(o_sys_reset[3]),	 // Templated
	     .bufg_gt_clr_out		(userio_bufg_gt_clr_out_3), // Templated
	     .tx_out_clk		(userio_tx_out_clk[3]),	 // Templated
	     // Inputs
	     .s_axi_tx_tdata		(i_s_axi_tx_tdata[255:192]), // Templated
	     .s_axi_tx_tkeep		(i_s_axi_tx_tkeep[31:24]), // Templated
	     .s_axi_tx_tlast		(i_s_axi_tx_tlast[3]),	 // Templated
	     .s_axi_tx_tvalid		(i_s_axi_tx_tvalid[3]),	 // Templated
	     .rxp			(qsfp_cp_rxp[3]),	 // Templated
	     .rxn			(qsfp_cp_rxn[3]),	 // Templated
	     .refclk1_in		(qsfp_refclk[3]),	 // Templated
	     .mmcm_not_locked		(!userio_mmcm_not_locked_out[3]), // Templated
	     .user_clk			(userio_clk_3),		 // Templated
	     .sync_clk			(userio_clk_3),		 // Templated
	     .reset_pb			(userio_reset[3]),	 // Templated
	     .gt_rxcdrovrden_in		(1'b0),			 // Templated
	     .power_down		(1'b0),			 // Templated
	     .loopback			(3'b0),			 // Templated
	     .pma_init			(userio_gt_reset[3]),	 // Templated
	     .gt_qpllclk_quad1_in	(qpll_outclk[3]),	 // Templated
	     .gt_qpllrefclk_quad1_in	(qpll_outrefclk[3]),	 // Templated
	     .gt_qplllock_quad1_in	(qpll_lock[3]),		 // Templated
	     .gt_qpllrefclklost_quad1	(qpll_refclklost[3]),	 // Templated
	     .s_axi_awaddr		(32'b0),		 // Templated
	     .s_axi_araddr		(32'b0),		 // Templated
	     .s_axi_wdata		(32'b0),		 // Templated
	     .s_axi_wstrb		(4'b0),			 // Templated
	     .s_axi_awvalid		(1'b0),			 // Templated
	     .s_axi_rready		(1'b0),			 // Templated
	     .s_axi_bready		(1'b0),			 // Templated
	     .s_axi_arvalid		(1'b0),			 // Templated
	     .s_axi_wvalid		(1'b0),			 // Templated
	     .init_clk			(i_init_clk));		 // Templated
         end else begin : tieoff
	    assign cp_qsfp_txp[3] = 1'b0;
	    assign cp_qsfp_txn[3] = 1'b1;

	    assign qsfp_fatal_alarm[3] = 1'b0;
	    assign qsfp_corr_alarm[3] = 1'b0;

	    assign qpll_reset[3] = 1'b0;

	    assign o_stat_chan_up[3] = 1'b0;
	    assign o_stat_lane_up[3] = 1'b0;
	    assign o_stat_gt_pwrgd[3] = 1'b0;
         end
      end

      
      if (link == 4) begin : p4
         if (link < NUM_AUR_LINKS) begin : aurora_if
	    
	    // FIXME: Does each core need its own BUFG?
	    BUFG_GT user_clk_4_buf (
					  .CE(1'b1),
					  .CEMASK(1'b0),
					  .CLR(userio_bufg_gt_clr_out_4),
					  .CLRMASK(1'b0),
					  .DIV(3'b0),
					  .I(userio_tx_out_clk[4]),
					  .O(userio_clk_4));

	    // leda B_1400 off Asynchronous reset/set/load signal is not a primary input to the current unit
	    always @(posedge userio_clk_4, posedge userio_bufg_gt_clr_out_4) begin
	       if (userio_bufg_gt_clr_out_4) begin
		  userio_mmcm_not_locked_out[4] <= 1'b0;
	       end else begin
		  userio_mmcm_not_locked_out[4] <= 1'b1;
	       end
	    end
	    // leda B_1400 on Asynchronous reset/set/load signal is not a primary input to the current unit

	    assign o_user_clk[4] = userio_clk_4;

	    // FIXME: userio_reset must be asserted for 128*userio_clk(?) cycles. How do we ensure this?
	    userio_reset_logic userio_reset_logic_4 (.i_user_clk(i_init_clk), //userio_clk_4
							 .i_init_clk(i_init_clk),
							 .i_reset(i_reset),
							 .i_sys_reset(i_reset), //o_sys_reset[4]
							 .i_gt_reset(i_gt_reset),
							 .o_sys_reset(userio_reset_logic[4]),
							 .o_gt_reset(userio_gt_reset[4]));


	    always @ (posedge i_init_clk) begin //userio_clk_4
	       userio_reset[4] <= userio_reset_logic[4] | !userio_gt_pll_lock[4] | userio_link_reset_out[4];
	    end
	    
            aurora_64b66b_25p78G aurora_userio_p4
            (/*AUTOINST*/
	     // Outputs
	     .s_axi_tx_tready		(o_s_axi_tx_tready[4]),	 // Templated
	     .m_axi_rx_tdata		(o_m_axi_rx_tdata[319:256]), // Templated
	     .m_axi_rx_tkeep		(o_m_axi_rx_tkeep[39:32]), // Templated
	     .m_axi_rx_tlast		(o_m_axi_rx_tlast[4]),	 // Templated
	     .m_axi_rx_tvalid		(o_m_axi_rx_tvalid[4]),	 // Templated
	     .txp			(cp_qsfp_txp[4]),	 // Templated
	     .txn			(cp_qsfp_txn[4]),	 // Templated
	     .hard_err			(qsfp_fatal_alarm[4]),	 // Templated
	     .soft_err			(qsfp_corr_alarm[4]),	 // Templated
	     .channel_up		(o_stat_chan_up[4]),	 // Templated
	     .lane_up			(o_stat_lane_up[4]),	 // Templated
	     .gt_to_common_qpllreset_out(qpll_reset[4]),	 // Templated
	     .s_axi_rdata		(p4_s_axi_rdata_uc[31:0]), // Templated
	     .s_axi_awready		(p4_s_axi_awready_uc),	 // Templated
	     .s_axi_wready		(p4_s_axi_wready_uc),	 // Templated
	     .s_axi_bvalid		(p4_s_axi_bvalid_uc),	 // Templated
	     .s_axi_bresp		(p4_s_axi_bresp_uc[1:0]), // Templated
	     .s_axi_rresp		(p4_s_axi_rresp_uc[1:0]), // Templated
	     .s_axi_arready		(p4_s_axi_arready_uc),	 // Templated
	     .s_axi_rvalid		(p4_s_axi_rvalid_uc),	 // Templated
	     .link_reset_out		(userio_link_reset_out[4]), // Templated
	     .gt_powergood		(o_stat_gt_pwrgd[4]),	 // Templated
	     .gt_pll_lock		(userio_gt_pll_lock[4]), // Templated
	     .sys_reset_out		(o_sys_reset[4]),	 // Templated
	     .bufg_gt_clr_out		(userio_bufg_gt_clr_out_4), // Templated
	     .tx_out_clk		(userio_tx_out_clk[4]),	 // Templated
	     // Inputs
	     .s_axi_tx_tdata		(i_s_axi_tx_tdata[319:256]), // Templated
	     .s_axi_tx_tkeep		(i_s_axi_tx_tkeep[39:32]), // Templated
	     .s_axi_tx_tlast		(i_s_axi_tx_tlast[4]),	 // Templated
	     .s_axi_tx_tvalid		(i_s_axi_tx_tvalid[4]),	 // Templated
	     .rxp			(qsfp_cp_rxp[4]),	 // Templated
	     .rxn			(qsfp_cp_rxn[4]),	 // Templated
	     .refclk1_in		(qsfp_refclk[4]),	 // Templated
	     .mmcm_not_locked		(!userio_mmcm_not_locked_out[4]), // Templated
	     .user_clk			(userio_clk_4),		 // Templated
	     .sync_clk			(userio_clk_4),		 // Templated
	     .reset_pb			(userio_reset[4]),	 // Templated
	     .gt_rxcdrovrden_in		(1'b0),			 // Templated
	     .power_down		(1'b0),			 // Templated
	     .loopback			(3'b0),			 // Templated
	     .pma_init			(userio_gt_reset[4]),	 // Templated
	     .gt_qpllclk_quad1_in	(qpll_outclk[4]),	 // Templated
	     .gt_qpllrefclk_quad1_in	(qpll_outrefclk[4]),	 // Templated
	     .gt_qplllock_quad1_in	(qpll_lock[4]),		 // Templated
	     .gt_qpllrefclklost_quad1	(qpll_refclklost[4]),	 // Templated
	     .s_axi_awaddr		(32'b0),		 // Templated
	     .s_axi_araddr		(32'b0),		 // Templated
	     .s_axi_wdata		(32'b0),		 // Templated
	     .s_axi_wstrb		(4'b0),			 // Templated
	     .s_axi_awvalid		(1'b0),			 // Templated
	     .s_axi_rready		(1'b0),			 // Templated
	     .s_axi_bready		(1'b0),			 // Templated
	     .s_axi_arvalid		(1'b0),			 // Templated
	     .s_axi_wvalid		(1'b0),			 // Templated
	     .init_clk			(i_init_clk));		 // Templated
         end else begin : tieoff
	    assign cp_qsfp_txp[4] = 1'b0;
	    assign cp_qsfp_txn[4] = 1'b1;

	    assign qsfp_fatal_alarm[4] = 1'b0;
	    assign qsfp_corr_alarm[4] = 1'b0;

	    assign qpll_reset[4] = 1'b0;

	    assign o_stat_chan_up[4] = 1'b0;
	    assign o_stat_lane_up[4] = 1'b0;
	    assign o_stat_gt_pwrgd[4] = 1'b0;
         end
      end

      
      if (link == 5) begin : p5
         if (link < NUM_AUR_LINKS) begin : aurora_if
	    
	    // FIXME: Does each core need its own BUFG?
	    BUFG_GT user_clk_5_buf (
					  .CE(1'b1),
					  .CEMASK(1'b0),
					  .CLR(userio_bufg_gt_clr_out_5),
					  .CLRMASK(1'b0),
					  .DIV(3'b0),
					  .I(userio_tx_out_clk[5]),
					  .O(userio_clk_5));

	    // leda B_1400 off Asynchronous reset/set/load signal is not a primary input to the current unit
	    always @(posedge userio_clk_5, posedge userio_bufg_gt_clr_out_5) begin
	       if (userio_bufg_gt_clr_out_5) begin
		  userio_mmcm_not_locked_out[5] <= 1'b0;
	       end else begin
		  userio_mmcm_not_locked_out[5] <= 1'b1;
	       end
	    end
	    // leda B_1400 on Asynchronous reset/set/load signal is not a primary input to the current unit

	    assign o_user_clk[5] = userio_clk_5;

	    // FIXME: userio_reset must be asserted for 128*userio_clk(?) cycles. How do we ensure this?
	    userio_reset_logic userio_reset_logic_5 (.i_user_clk(i_init_clk), //userio_clk_5
							 .i_init_clk(i_init_clk),
							 .i_reset(i_reset),
							 .i_sys_reset(i_reset), //o_sys_reset[5]
							 .i_gt_reset(i_gt_reset),
							 .o_sys_reset(userio_reset_logic[5]),
							 .o_gt_reset(userio_gt_reset[5]));


	    always @ (posedge i_init_clk) begin //userio_clk_5
	       userio_reset[5] <= userio_reset_logic[5] | !userio_gt_pll_lock[5] | userio_link_reset_out[5];
	    end
	    
            aurora_64b66b_25p78G aurora_userio_p5
            (/*AUTOINST*/
	     // Outputs
	     .s_axi_tx_tready		(o_s_axi_tx_tready[5]),	 // Templated
	     .m_axi_rx_tdata		(o_m_axi_rx_tdata[383:320]), // Templated
	     .m_axi_rx_tkeep		(o_m_axi_rx_tkeep[47:40]), // Templated
	     .m_axi_rx_tlast		(o_m_axi_rx_tlast[5]),	 // Templated
	     .m_axi_rx_tvalid		(o_m_axi_rx_tvalid[5]),	 // Templated
	     .txp			(cp_qsfp_txp[5]),	 // Templated
	     .txn			(cp_qsfp_txn[5]),	 // Templated
	     .hard_err			(qsfp_fatal_alarm[5]),	 // Templated
	     .soft_err			(qsfp_corr_alarm[5]),	 // Templated
	     .channel_up		(o_stat_chan_up[5]),	 // Templated
	     .lane_up			(o_stat_lane_up[5]),	 // Templated
	     .gt_to_common_qpllreset_out(qpll_reset[5]),	 // Templated
	     .s_axi_rdata		(p5_s_axi_rdata_uc[31:0]), // Templated
	     .s_axi_awready		(p5_s_axi_awready_uc),	 // Templated
	     .s_axi_wready		(p5_s_axi_wready_uc),	 // Templated
	     .s_axi_bvalid		(p5_s_axi_bvalid_uc),	 // Templated
	     .s_axi_bresp		(p5_s_axi_bresp_uc[1:0]), // Templated
	     .s_axi_rresp		(p5_s_axi_rresp_uc[1:0]), // Templated
	     .s_axi_arready		(p5_s_axi_arready_uc),	 // Templated
	     .s_axi_rvalid		(p5_s_axi_rvalid_uc),	 // Templated
	     .link_reset_out		(userio_link_reset_out[5]), // Templated
	     .gt_powergood		(o_stat_gt_pwrgd[5]),	 // Templated
	     .gt_pll_lock		(userio_gt_pll_lock[5]), // Templated
	     .sys_reset_out		(o_sys_reset[5]),	 // Templated
	     .bufg_gt_clr_out		(userio_bufg_gt_clr_out_5), // Templated
	     .tx_out_clk		(userio_tx_out_clk[5]),	 // Templated
	     // Inputs
	     .s_axi_tx_tdata		(i_s_axi_tx_tdata[383:320]), // Templated
	     .s_axi_tx_tkeep		(i_s_axi_tx_tkeep[47:40]), // Templated
	     .s_axi_tx_tlast		(i_s_axi_tx_tlast[5]),	 // Templated
	     .s_axi_tx_tvalid		(i_s_axi_tx_tvalid[5]),	 // Templated
	     .rxp			(qsfp_cp_rxp[5]),	 // Templated
	     .rxn			(qsfp_cp_rxn[5]),	 // Templated
	     .refclk1_in		(qsfp_refclk[5]),	 // Templated
	     .mmcm_not_locked		(!userio_mmcm_not_locked_out[5]), // Templated
	     .user_clk			(userio_clk_5),		 // Templated
	     .sync_clk			(userio_clk_5),		 // Templated
	     .reset_pb			(userio_reset[5]),	 // Templated
	     .gt_rxcdrovrden_in		(1'b0),			 // Templated
	     .power_down		(1'b0),			 // Templated
	     .loopback			(3'b0),			 // Templated
	     .pma_init			(userio_gt_reset[5]),	 // Templated
	     .gt_qpllclk_quad1_in	(qpll_outclk[5]),	 // Templated
	     .gt_qpllrefclk_quad1_in	(qpll_outrefclk[5]),	 // Templated
	     .gt_qplllock_quad1_in	(qpll_lock[5]),		 // Templated
	     .gt_qpllrefclklost_quad1	(qpll_refclklost[5]),	 // Templated
	     .s_axi_awaddr		(32'b0),		 // Templated
	     .s_axi_araddr		(32'b0),		 // Templated
	     .s_axi_wdata		(32'b0),		 // Templated
	     .s_axi_wstrb		(4'b0),			 // Templated
	     .s_axi_awvalid		(1'b0),			 // Templated
	     .s_axi_rready		(1'b0),			 // Templated
	     .s_axi_bready		(1'b0),			 // Templated
	     .s_axi_arvalid		(1'b0),			 // Templated
	     .s_axi_wvalid		(1'b0),			 // Templated
	     .init_clk			(i_init_clk));		 // Templated
         end else begin : tieoff
	    assign cp_qsfp_txp[5] = 1'b0;
	    assign cp_qsfp_txn[5] = 1'b1;

	    assign qsfp_fatal_alarm[5] = 1'b0;
	    assign qsfp_corr_alarm[5] = 1'b0;

	    assign qpll_reset[5] = 1'b0;

	    assign o_stat_chan_up[5] = 1'b0;
	    assign o_stat_lane_up[5] = 1'b0;
	    assign o_stat_gt_pwrgd[5] = 1'b0;
         end
      end

      
      if (link == 6) begin : p6
         if (link < NUM_AUR_LINKS) begin : aurora_if
	    
	    // FIXME: Does each core need its own BUFG?
	    BUFG_GT user_clk_6_buf (
					  .CE(1'b1),
					  .CEMASK(1'b0),
					  .CLR(userio_bufg_gt_clr_out_6),
					  .CLRMASK(1'b0),
					  .DIV(3'b0),
					  .I(userio_tx_out_clk[6]),
					  .O(userio_clk_6));

	    // leda B_1400 off Asynchronous reset/set/load signal is not a primary input to the current unit
	    always @(posedge userio_clk_6, posedge userio_bufg_gt_clr_out_6) begin
	       if (userio_bufg_gt_clr_out_6) begin
		  userio_mmcm_not_locked_out[6] <= 1'b0;
	       end else begin
		  userio_mmcm_not_locked_out[6] <= 1'b1;
	       end
	    end
	    // leda B_1400 on Asynchronous reset/set/load signal is not a primary input to the current unit

	    assign o_user_clk[6] = userio_clk_6;

	    // FIXME: userio_reset must be asserted for 128*userio_clk(?) cycles. How do we ensure this?
	    userio_reset_logic userio_reset_logic_6 (.i_user_clk(i_init_clk), //userio_clk_6
							 .i_init_clk(i_init_clk),
							 .i_reset(i_reset),
							 .i_sys_reset(i_reset), //o_sys_reset[6]
							 .i_gt_reset(i_gt_reset),
							 .o_sys_reset(userio_reset_logic[6]),
							 .o_gt_reset(userio_gt_reset[6]));


	    always @ (posedge i_init_clk) begin //userio_clk_6
	       userio_reset[6] <= userio_reset_logic[6] | !userio_gt_pll_lock[6] | userio_link_reset_out[6];
	    end
	    
            aurora_64b66b_25p78G aurora_userio_p6
            (/*AUTOINST*/
	     // Outputs
	     .s_axi_tx_tready		(o_s_axi_tx_tready[6]),	 // Templated
	     .m_axi_rx_tdata		(o_m_axi_rx_tdata[447:384]), // Templated
	     .m_axi_rx_tkeep		(o_m_axi_rx_tkeep[55:48]), // Templated
	     .m_axi_rx_tlast		(o_m_axi_rx_tlast[6]),	 // Templated
	     .m_axi_rx_tvalid		(o_m_axi_rx_tvalid[6]),	 // Templated
	     .txp			(cp_qsfp_txp[6]),	 // Templated
	     .txn			(cp_qsfp_txn[6]),	 // Templated
	     .hard_err			(qsfp_fatal_alarm[6]),	 // Templated
	     .soft_err			(qsfp_corr_alarm[6]),	 // Templated
	     .channel_up		(o_stat_chan_up[6]),	 // Templated
	     .lane_up			(o_stat_lane_up[6]),	 // Templated
	     .gt_to_common_qpllreset_out(qpll_reset[6]),	 // Templated
	     .s_axi_rdata		(p6_s_axi_rdata_uc[31:0]), // Templated
	     .s_axi_awready		(p6_s_axi_awready_uc),	 // Templated
	     .s_axi_wready		(p6_s_axi_wready_uc),	 // Templated
	     .s_axi_bvalid		(p6_s_axi_bvalid_uc),	 // Templated
	     .s_axi_bresp		(p6_s_axi_bresp_uc[1:0]), // Templated
	     .s_axi_rresp		(p6_s_axi_rresp_uc[1:0]), // Templated
	     .s_axi_arready		(p6_s_axi_arready_uc),	 // Templated
	     .s_axi_rvalid		(p6_s_axi_rvalid_uc),	 // Templated
	     .link_reset_out		(userio_link_reset_out[6]), // Templated
	     .gt_powergood		(o_stat_gt_pwrgd[6]),	 // Templated
	     .gt_pll_lock		(userio_gt_pll_lock[6]), // Templated
	     .sys_reset_out		(o_sys_reset[6]),	 // Templated
	     .bufg_gt_clr_out		(userio_bufg_gt_clr_out_6), // Templated
	     .tx_out_clk		(userio_tx_out_clk[6]),	 // Templated
	     // Inputs
	     .s_axi_tx_tdata		(i_s_axi_tx_tdata[447:384]), // Templated
	     .s_axi_tx_tkeep		(i_s_axi_tx_tkeep[55:48]), // Templated
	     .s_axi_tx_tlast		(i_s_axi_tx_tlast[6]),	 // Templated
	     .s_axi_tx_tvalid		(i_s_axi_tx_tvalid[6]),	 // Templated
	     .rxp			(qsfp_cp_rxp[6]),	 // Templated
	     .rxn			(qsfp_cp_rxn[6]),	 // Templated
	     .refclk1_in		(qsfp_refclk[6]),	 // Templated
	     .mmcm_not_locked		(!userio_mmcm_not_locked_out[6]), // Templated
	     .user_clk			(userio_clk_6),		 // Templated
	     .sync_clk			(userio_clk_6),		 // Templated
	     .reset_pb			(userio_reset[6]),	 // Templated
	     .gt_rxcdrovrden_in		(1'b0),			 // Templated
	     .power_down		(1'b0),			 // Templated
	     .loopback			(3'b0),			 // Templated
	     .pma_init			(userio_gt_reset[6]),	 // Templated
	     .gt_qpllclk_quad1_in	(qpll_outclk[6]),	 // Templated
	     .gt_qpllrefclk_quad1_in	(qpll_outrefclk[6]),	 // Templated
	     .gt_qplllock_quad1_in	(qpll_lock[6]),		 // Templated
	     .gt_qpllrefclklost_quad1	(qpll_refclklost[6]),	 // Templated
	     .s_axi_awaddr		(32'b0),		 // Templated
	     .s_axi_araddr		(32'b0),		 // Templated
	     .s_axi_wdata		(32'b0),		 // Templated
	     .s_axi_wstrb		(4'b0),			 // Templated
	     .s_axi_awvalid		(1'b0),			 // Templated
	     .s_axi_rready		(1'b0),			 // Templated
	     .s_axi_bready		(1'b0),			 // Templated
	     .s_axi_arvalid		(1'b0),			 // Templated
	     .s_axi_wvalid		(1'b0),			 // Templated
	     .init_clk			(i_init_clk));		 // Templated
         end else begin : tieoff
	    assign cp_qsfp_txp[6] = 1'b0;
	    assign cp_qsfp_txn[6] = 1'b1;

	    assign qsfp_fatal_alarm[6] = 1'b0;
	    assign qsfp_corr_alarm[6] = 1'b0;

	    assign qpll_reset[6] = 1'b0;

	    assign o_stat_chan_up[6] = 1'b0;
	    assign o_stat_lane_up[6] = 1'b0;
	    assign o_stat_gt_pwrgd[6] = 1'b0;
         end
      end

      
      if (link == 7) begin : p7
         if (link < NUM_AUR_LINKS) begin : aurora_if
	    
	    // FIXME: Does each core need its own BUFG?
	    BUFG_GT user_clk_7_buf (
					  .CE(1'b1),
					  .CEMASK(1'b0),
					  .CLR(userio_bufg_gt_clr_out_7),
					  .CLRMASK(1'b0),
					  .DIV(3'b0),
					  .I(userio_tx_out_clk[7]),
					  .O(userio_clk_7));

	    // leda B_1400 off Asynchronous reset/set/load signal is not a primary input to the current unit
	    always @(posedge userio_clk_7, posedge userio_bufg_gt_clr_out_7) begin
	       if (userio_bufg_gt_clr_out_7) begin
		  userio_mmcm_not_locked_out[7] <= 1'b0;
	       end else begin
		  userio_mmcm_not_locked_out[7] <= 1'b1;
	       end
	    end
	    // leda B_1400 on Asynchronous reset/set/load signal is not a primary input to the current unit

	    assign o_user_clk[7] = userio_clk_7;

	    // FIXME: userio_reset must be asserted for 128*userio_clk(?) cycles. How do we ensure this?
	    userio_reset_logic userio_reset_logic_7 (.i_user_clk(i_init_clk), //userio_clk_7
							 .i_init_clk(i_init_clk),
							 .i_reset(i_reset),
							 .i_sys_reset(i_reset), //o_sys_reset[7]
							 .i_gt_reset(i_gt_reset),
							 .o_sys_reset(userio_reset_logic[7]),
							 .o_gt_reset(userio_gt_reset[7]));


	    always @ (posedge i_init_clk) begin //userio_clk_7
	       userio_reset[7] <= userio_reset_logic[7] | !userio_gt_pll_lock[7] | userio_link_reset_out[7];
	    end
	    
            aurora_64b66b_25p78G aurora_userio_p7
            (/*AUTOINST*/
	     // Outputs
	     .s_axi_tx_tready		(o_s_axi_tx_tready[7]),	 // Templated
	     .m_axi_rx_tdata		(o_m_axi_rx_tdata[511:448]), // Templated
	     .m_axi_rx_tkeep		(o_m_axi_rx_tkeep[63:56]), // Templated
	     .m_axi_rx_tlast		(o_m_axi_rx_tlast[7]),	 // Templated
	     .m_axi_rx_tvalid		(o_m_axi_rx_tvalid[7]),	 // Templated
	     .txp			(cp_qsfp_txp[7]),	 // Templated
	     .txn			(cp_qsfp_txn[7]),	 // Templated
	     .hard_err			(qsfp_fatal_alarm[7]),	 // Templated
	     .soft_err			(qsfp_corr_alarm[7]),	 // Templated
	     .channel_up		(o_stat_chan_up[7]),	 // Templated
	     .lane_up			(o_stat_lane_up[7]),	 // Templated
	     .gt_to_common_qpllreset_out(qpll_reset[7]),	 // Templated
	     .s_axi_rdata		(p7_s_axi_rdata_uc[31:0]), // Templated
	     .s_axi_awready		(p7_s_axi_awready_uc),	 // Templated
	     .s_axi_wready		(p7_s_axi_wready_uc),	 // Templated
	     .s_axi_bvalid		(p7_s_axi_bvalid_uc),	 // Templated
	     .s_axi_bresp		(p7_s_axi_bresp_uc[1:0]), // Templated
	     .s_axi_rresp		(p7_s_axi_rresp_uc[1:0]), // Templated
	     .s_axi_arready		(p7_s_axi_arready_uc),	 // Templated
	     .s_axi_rvalid		(p7_s_axi_rvalid_uc),	 // Templated
	     .link_reset_out		(userio_link_reset_out[7]), // Templated
	     .gt_powergood		(o_stat_gt_pwrgd[7]),	 // Templated
	     .gt_pll_lock		(userio_gt_pll_lock[7]), // Templated
	     .sys_reset_out		(o_sys_reset[7]),	 // Templated
	     .bufg_gt_clr_out		(userio_bufg_gt_clr_out_7), // Templated
	     .tx_out_clk		(userio_tx_out_clk[7]),	 // Templated
	     // Inputs
	     .s_axi_tx_tdata		(i_s_axi_tx_tdata[511:448]), // Templated
	     .s_axi_tx_tkeep		(i_s_axi_tx_tkeep[63:56]), // Templated
	     .s_axi_tx_tlast		(i_s_axi_tx_tlast[7]),	 // Templated
	     .s_axi_tx_tvalid		(i_s_axi_tx_tvalid[7]),	 // Templated
	     .rxp			(qsfp_cp_rxp[7]),	 // Templated
	     .rxn			(qsfp_cp_rxn[7]),	 // Templated
	     .refclk1_in		(qsfp_refclk[7]),	 // Templated
	     .mmcm_not_locked		(!userio_mmcm_not_locked_out[7]), // Templated
	     .user_clk			(userio_clk_7),		 // Templated
	     .sync_clk			(userio_clk_7),		 // Templated
	     .reset_pb			(userio_reset[7]),	 // Templated
	     .gt_rxcdrovrden_in		(1'b0),			 // Templated
	     .power_down		(1'b0),			 // Templated
	     .loopback			(3'b0),			 // Templated
	     .pma_init			(userio_gt_reset[7]),	 // Templated
	     .gt_qpllclk_quad1_in	(qpll_outclk[7]),	 // Templated
	     .gt_qpllrefclk_quad1_in	(qpll_outrefclk[7]),	 // Templated
	     .gt_qplllock_quad1_in	(qpll_lock[7]),		 // Templated
	     .gt_qpllrefclklost_quad1	(qpll_refclklost[7]),	 // Templated
	     .s_axi_awaddr		(32'b0),		 // Templated
	     .s_axi_araddr		(32'b0),		 // Templated
	     .s_axi_wdata		(32'b0),		 // Templated
	     .s_axi_wstrb		(4'b0),			 // Templated
	     .s_axi_awvalid		(1'b0),			 // Templated
	     .s_axi_rready		(1'b0),			 // Templated
	     .s_axi_bready		(1'b0),			 // Templated
	     .s_axi_arvalid		(1'b0),			 // Templated
	     .s_axi_wvalid		(1'b0),			 // Templated
	     .init_clk			(i_init_clk));		 // Templated
         end else begin : tieoff
	    assign cp_qsfp_txp[7] = 1'b0;
	    assign cp_qsfp_txn[7] = 1'b1;

	    assign qsfp_fatal_alarm[7] = 1'b0;
	    assign qsfp_corr_alarm[7] = 1'b0;

	    assign qpll_reset[7] = 1'b0;

	    assign o_stat_chan_up[7] = 1'b0;
	    assign o_stat_lane_up[7] = 1'b0;
	    assign o_stat_gt_pwrgd[7] = 1'b0;
         end
      end

   end endgenerate

/* aurora_64b66b_25p78G AUTO_TEMPLATE (
      // Outputs
      .s_axi_tx_tready			(o_s_axi_tx_tready[@]),
      .m_axi_rx_tdata			(o_m_axi_rx_tdata[@"(- (* (+ 1 @) 64) 1)":@"(* @ 64)"]),
      .m_axi_rx_tkeep			(o_m_axi_rx_tkeep[@"(- (* (+ 1 @) 8) 1)":@"(* @ 8)"]),
      .m_axi_rx_tlast			(o_m_axi_rx_tlast[@]),
      .m_axi_rx_tvalid			(o_m_axi_rx_tvalid[@]),
      .txp				(cp_qsfp_txp[@]),
      .txn				(cp_qsfp_txn[@]),
      .hard_err				(qsfp_fatal_alarm[@]),
      .soft_err				(qsfp_corr_alarm[@]),
      .channel_up			(o_stat_chan_up[@]),
      .lane_up				(o_stat_lane_up[@]),
      .gt_to_common_qpllreset_out	(qpll_reset[@]),
      .s_axi_\(.*\)			(p@_s_axi_\1_uc[]),  // DRP (not used)
      .link_reset_out			(userio_link_reset_out[@]),
      .gt_powergood			(o_stat_gt_pwrgd[@]),
      .gt_pll_lock			(userio_gt_pll_lock[@]),
      .sys_reset_out			(o_sys_reset[@]),
      .bufg_gt_clr_out			(userio_bufg_gt_clr_out_@),
      .tx_out_clk			(userio_tx_out_clk[@]),
      // Inputs
      .s_axi_tx_tdata			(i_s_axi_tx_tdata[@"(- (* (+ 1 @) 64) 1)":@"(* @ 64)"]),
      .s_axi_tx_tkeep			(i_s_axi_tx_tkeep[@"(- (* (+ 1 @) 8) 1)":@"(* @ 8)"]),
      .s_axi_tx_tlast			(i_s_axi_tx_tlast[@]),
      .s_axi_tx_tvalid			(i_s_axi_tx_tvalid[@]),
      .rxp				(qsfp_cp_rxp[@]),
      .rxn				(qsfp_cp_rxn[@]),
      .refclk1_in			(qsfp_refclk[@]),
      .mmcm_not_locked			(!userio_mmcm_not_locked_out[@]),
      .user_clk				(userio_clk_@),
      .sync_clk				(userio_clk_@),
      .reset_pb				(userio_reset[@]),
      .gt_rxcdrovrden_in		(1'b0), // FIXME: Not sure what this is, maybe tied to loopback
      .power_down			(1'b0),
      .loopback				(3'b0),
      .pma_init				(userio_gt_reset[@]),
      .gt_qpllclk_quad1_in		(qpll_outclk[@]),
      .gt_qpllrefclk_quad1_in		(qpll_outrefclk[@]),
      .gt_qplllock_quad1_in		(qpll_lock[@]),
      .gt_qpllrefclklost_quad1		(qpll_refclklost[@]),
      .s_axi_awaddr			(32'b0), // DRP (not used)
      .s_axi_araddr			(32'b0), // DRP (not used)
      .s_axi_wdata			(32'b0), // DRP (not used)
      .s_axi_wstrb			(4'b0),  // DRP (not used)
      .s_axi_awvalid			(1'b0),  // DRP (not used)
      .s_axi_rready			(1'b0),  // DRP (not used)
      .s_axi_bready			(1'b0),  // DRP (not used)
      .s_axi_arvalid			(1'b0),  // DRP (not used)
      .s_axi_wvalid			(1'b0),  // DRP (not used)
      .init_clk				(i_init_clk),
   ); */
   
/* aurora_64b66b_25p78G_gt_common_wrapper AUTO_TEMPLATE (
			 // Outputs
			 .qpll0_lock		(qpll@_lock),
			 .qpll0_outclk		(qpll@_outclk),
			 .qpll0_outrefclk	(qpll@_outrefclk),
			 .qpll0_refclklost	(qpll@_refclklost),
			 // Inputs
			 .qpll0_refclk		(qsfp_refclk[@"(* 4 @)"]),
			 .qpll0_reset		(qpll@_reset),
			 .qpll0_lock_detclk	(i_init_clk),
   ); */

// leda XV2_1002 on Internally generated asynchronous reset detected (block level). 
// leda XV4_1002 on Internally generated asynchronous reset detected (block level).
// leda XV2P_1002 on Internally generated asynchronous reset detected (block level). 
// leda XV2P_1003 on Multiple asynchronous resets detected in this unit/module.
// leda XV2_1003 on Multiple asynchronous resets detected in this unit/module.
// leda XV4_1003 on Multiple asynchronous resets detected in this unit/module.
   
endmodule



module userio_reset_logic #()
(
   input 	i_user_clk,
   input 	i_init_clk,
   input 	i_reset,
   input 	i_sys_reset,
   input 	i_gt_reset,
   output 	o_sys_reset,
   output 	o_gt_reset
);

   wire 	sync_reset_reg;
   reg [3:0] 	sync_reset_capt;
   
   sync2_1 sync_reset ( .clk(i_user_clk), .d(i_reset), .q(sync_reset_reg) );
   always @ (posedge i_user_clk) begin
      sync_reset_capt <= {sync_reset_capt[2:0], sync_reset_reg};
   end

   assign o_sys_reset = &sync_reset_capt;


   wire 	sync_gt_reset_reg;
   reg [3:0] 	sync_gt_reset_capt;
   
   sync2_1 sync_gt_reset ( .clk(i_init_clk), .d(i_gt_reset), .q(sync_gt_reset_reg) );
   always @ (posedge i_init_clk) begin
      sync_gt_reset_capt <= {sync_gt_reset_capt[2:0], sync_gt_reset_reg};
   end
   assign o_gt_reset = &sync_gt_reset_capt;
	   
endmodule

// This is the search path for the autoinst commands in emacs.
// After modification you must save file, then reload with C-x C-v
//
// Local Variables:
// verilog-library-directories:("." "../../coregen/vu7p/ip_user_files/ip/aurora_64b66b_25p78G") 
// End:
