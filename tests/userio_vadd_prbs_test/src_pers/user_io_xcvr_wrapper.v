`timescale 1 ns / 1 ps

module user_io_xcvr_wrapper #
(
   parameter QSFP0_WIDTH = 4,
   parameter QSFP1_WIDTH = 4
)(
   input             i_init_clk,
   
   input             i_reset,
   input             i_gt_reset,
   output [3:0]      o_sys_reset0,
   output [3:0]      o_sys_reset1,
   
   output [3:0]      o_user_clk0,
   output [3:0]      o_user_clk1,
   
   input             qsfp0_refclk,
   input             qsfp1_refclk,
   
   input [4:1]       qsfp0_cp_rxp,
   input [4:1]       qsfp0_cp_rxn,
   output [4:1]      cp_qsfp0_txp,
   output [4:1]      cp_qsfp0_txn,
   
   input [4:1]       qsfp1_cp_rxp,
   input [4:1]       qsfp1_cp_rxn,
   output [4:1]      cp_qsfp1_txp,
   output [4:1]      cp_qsfp1_txn,

   // AXI Connections (QSFP0)
   input [4*8-1:0]   i_s0_axi_tx_tkeep,
   input [4*64-1:0]  i_s0_axi_tx_tdata,
   input [4*1-1:0]   i_s0_axi_tx_tlast,
   input [4*1-1:0]   i_s0_axi_tx_tvalid,
   output [4*1-1:0]  o_s0_axi_tx_tready,

   output [4*8-1:0]  o_m0_axi_rx_tkeep,
   output [4*64-1:0] o_m0_axi_rx_tdata,
   output [4*1-1:0]  o_m0_axi_rx_tlast,
   output [4*1-1:0]  o_m0_axi_rx_tvalid,

   // AXI Connections (QSFP1)
   input [4*8-1:0]   i_s1_axi_tx_tkeep,
   input [4*64-1:0]  i_s1_axi_tx_tdata,
   input [4*1-1:0]   i_s1_axi_tx_tlast,
   input [4*1-1:0]   i_s1_axi_tx_tvalid,
   output [4*1-1:0]  o_s1_axi_tx_tready,

   output [4*8-1:0]  o_m1_axi_rx_tkeep,
   output [4*64-1:0] o_m1_axi_rx_tdata,
   output [4*1-1:0]  o_m1_axi_rx_tlast,
   output [4*1-1:0]  o_m1_axi_rx_tvalid,

   // optional alarm indications 
   output [4:1]      qsfp0_fatal_alarm,
   output [4:1]      qsfp0_corr_alarm,
   output [4:1]      qsfp1_fatal_alarm,
   output [4:1]      qsfp1_corr_alarm,

   // status ports
   output [3:0]      o_stat_chan_up0,
   output [3:0]      o_stat_lane_up0,
   output [3:0]      o_stat_gt_pwrgd0,
   output [3:0]      o_stat_chan_up1,
   output [3:0]      o_stat_lane_up1,
   output [3:0]      o_stat_gt_pwrgd1
);

// leda XV2_1002 off Internally generated asynchronous reset detected (block level). 
// leda XV4_1002 off Internally generated asynchronous reset detected (block level). 
// leda XV2P_1002 off Internally generated asynchronous reset detected (block level). 
// leda XV2P_1003 off Multiple asynchronous resets detected in this unit/module.
// leda XV2_1003 off Multiple asynchronous resets detected in this unit/module.
// leda XV4_1003 off Multiple asynchronous resets detected in this unit/module.

   /*AUTOWIRE*/
   // Beginning of automatic wires (for undeclared instantiated-module outputs)
   wire                 _nc_p0_s0_axi_arready;    // From aurora_userio_p0_0 of aurora_64b66b_25p4G.v
   wire                 _nc_p0_s0_axi_awready;    // From aurora_userio_p0_0 of aurora_64b66b_25p4G.v
   wire [1:0]           _nc_p0_s0_axi_bresp;      // From aurora_userio_p0_0 of aurora_64b66b_25p4G.v
   wire                 _nc_p0_s0_axi_bvalid;     // From aurora_userio_p0_0 of aurora_64b66b_25p4G.v
   wire [31:0]          _nc_p0_s0_axi_rdata;      // From aurora_userio_p0_0 of aurora_64b66b_25p4G.v
   wire [1:0]           _nc_p0_s0_axi_rresp;      // From aurora_userio_p0_0 of aurora_64b66b_25p4G.v
   wire                 _nc_p0_s0_axi_rvalid;     // From aurora_userio_p0_0 of aurora_64b66b_25p4G.v
   wire                 _nc_p0_s0_axi_wready;     // From aurora_userio_p0_0 of aurora_64b66b_25p4G.v
   wire                 _nc_p1_s0_axi_arready;    // From aurora_userio_p0_1 of aurora_64b66b_25p4G.v
   wire                 _nc_p1_s0_axi_awready;    // From aurora_userio_p0_1 of aurora_64b66b_25p4G.v
   wire [1:0]           _nc_p1_s0_axi_bresp;      // From aurora_userio_p0_1 of aurora_64b66b_25p4G.v
   wire                 _nc_p1_s0_axi_bvalid;     // From aurora_userio_p0_1 of aurora_64b66b_25p4G.v
   wire [31:0]          _nc_p1_s0_axi_rdata;      // From aurora_userio_p0_1 of aurora_64b66b_25p4G.v
   wire [1:0]           _nc_p1_s0_axi_rresp;      // From aurora_userio_p0_1 of aurora_64b66b_25p4G.v
   wire                 _nc_p1_s0_axi_rvalid;     // From aurora_userio_p0_1 of aurora_64b66b_25p4G.v
   wire                 _nc_p1_s0_axi_wready;     // From aurora_userio_p0_1 of aurora_64b66b_25p4G.v
   wire                 _nc_p2_s0_axi_arready;    // From aurora_userio_p0_2 of aurora_64b66b_25p4G.v
   wire                 _nc_p2_s0_axi_awready;    // From aurora_userio_p0_2 of aurora_64b66b_25p4G.v
   wire [1:0]           _nc_p2_s0_axi_bresp;      // From aurora_userio_p0_2 of aurora_64b66b_25p4G.v
   wire                 _nc_p2_s0_axi_bvalid;     // From aurora_userio_p0_2 of aurora_64b66b_25p4G.v
   wire [31:0]          _nc_p2_s0_axi_rdata;      // From aurora_userio_p0_2 of aurora_64b66b_25p4G.v
   wire [1:0]           _nc_p2_s0_axi_rresp;      // From aurora_userio_p0_2 of aurora_64b66b_25p4G.v
   wire                 _nc_p2_s0_axi_rvalid;     // From aurora_userio_p0_2 of aurora_64b66b_25p4G.v
   wire                 _nc_p2_s0_axi_wready;     // From aurora_userio_p0_2 of aurora_64b66b_25p4G.v
   wire                 _nc_p3_s0_axi_arready;    // From aurora_userio_p0_3 of aurora_64b66b_25p4G.v
   wire                 _nc_p3_s0_axi_awready;    // From aurora_userio_p0_3 of aurora_64b66b_25p4G.v
   wire [1:0]           _nc_p3_s0_axi_bresp;      // From aurora_userio_p0_3 of aurora_64b66b_25p4G.v
   wire                 _nc_p3_s0_axi_bvalid;     // From aurora_userio_p0_3 of aurora_64b66b_25p4G.v
   wire [31:0]          _nc_p3_s0_axi_rdata;      // From aurora_userio_p0_3 of aurora_64b66b_25p4G.v
   wire [1:0]           _nc_p3_s0_axi_rresp;      // From aurora_userio_p0_3 of aurora_64b66b_25p4G.v
   wire                 _nc_p3_s0_axi_rvalid;     // From aurora_userio_p0_3 of aurora_64b66b_25p4G.v
   wire                 _nc_p3_s0_axi_wready;     // From aurora_userio_p0_3 of aurora_64b66b_25p4G.v
   wire                 _nc_p0_s1_axi_arready;    // From aurora_userio_p1_0 of aurora_64b66b_25p4G.v
   wire                 _nc_p0_s1_axi_awready;    // From aurora_userio_p1_0 of aurora_64b66b_25p4G.v
   wire [1:0]           _nc_p0_s1_axi_bresp;      // From aurora_userio_p1_0 of aurora_64b66b_25p4G.v
   wire                 _nc_p0_s1_axi_bvalid;     // From aurora_userio_p1_0 of aurora_64b66b_25p4G.v
   wire [31:0]          _nc_p0_s1_axi_rdata;      // From aurora_userio_p1_0 of aurora_64b66b_25p4G.v
   wire [1:0]           _nc_p0_s1_axi_rresp;      // From aurora_userio_p1_0 of aurora_64b66b_25p4G.v
   wire                 _nc_p0_s1_axi_rvalid;     // From aurora_userio_p1_0 of aurora_64b66b_25p4G.v
   wire                 _nc_p0_s1_axi_wready;     // From aurora_userio_p1_0 of aurora_64b66b_25p4G.v
   wire                 _nc_p1_s1_axi_arready;    // From aurora_userio_p1_1 of aurora_64b66b_25p4G.v
   wire                 _nc_p1_s1_axi_awready;    // From aurora_userio_p1_1 of aurora_64b66b_25p4G.v
   wire [1:0]           _nc_p1_s1_axi_bresp;      // From aurora_userio_p1_1 of aurora_64b66b_25p4G.v
   wire                 _nc_p1_s1_axi_bvalid;     // From aurora_userio_p1_1 of aurora_64b66b_25p4G.v
   wire [31:0]          _nc_p1_s1_axi_rdata;      // From aurora_userio_p1_1 of aurora_64b66b_25p4G.v
   wire [1:0]           _nc_p1_s1_axi_rresp;      // From aurora_userio_p1_1 of aurora_64b66b_25p4G.v
   wire                 _nc_p1_s1_axi_rvalid;     // From aurora_userio_p1_1 of aurora_64b66b_25p4G.v
   wire                 _nc_p1_s1_axi_wready;     // From aurora_userio_p1_1 of aurora_64b66b_25p4G.v
   wire                 _nc_p2_s1_axi_arready;    // From aurora_userio_p1_2 of aurora_64b66b_25p4G.v
   wire                 _nc_p2_s1_axi_awready;    // From aurora_userio_p1_2 of aurora_64b66b_25p4G.v
   wire [1:0]           _nc_p2_s1_axi_bresp;      // From aurora_userio_p1_2 of aurora_64b66b_25p4G.v
   wire                 _nc_p2_s1_axi_bvalid;     // From aurora_userio_p1_2 of aurora_64b66b_25p4G.v
   wire [31:0]          _nc_p2_s1_axi_rdata;      // From aurora_userio_p1_2 of aurora_64b66b_25p4G.v
   wire [1:0]           _nc_p2_s1_axi_rresp;      // From aurora_userio_p1_2 of aurora_64b66b_25p4G.v
   wire                 _nc_p2_s1_axi_rvalid;     // From aurora_userio_p1_2 of aurora_64b66b_25p4G.v
   wire                 _nc_p2_s1_axi_wready;     // From aurora_userio_p1_2 of aurora_64b66b_25p4G.v
   wire                 _nc_p3_s1_axi_arready;    // From aurora_userio_p1_3 of aurora_64b66b_25p4G.v
   wire                 _nc_p3_s1_axi_awready;    // From aurora_userio_p1_3 of aurora_64b66b_25p4G.v
   wire [1:0]           _nc_p3_s1_axi_bresp;      // From aurora_userio_p1_3 of aurora_64b66b_25p4G.v
   wire                 _nc_p3_s1_axi_bvalid;     // From aurora_userio_p1_3 of aurora_64b66b_25p4G.v
   wire [31:0]          _nc_p3_s1_axi_rdata;      // From aurora_userio_p1_3 of aurora_64b66b_25p4G.v
   wire [1:0]           _nc_p3_s1_axi_rresp;      // From aurora_userio_p1_3 of aurora_64b66b_25p4G.v
   wire                 _nc_p3_s1_axi_rvalid;     // From aurora_userio_p1_3 of aurora_64b66b_25p4G.v
   wire                 _nc_p3_s1_axi_wready;     // From aurora_userio_p1_3 of aurora_64b66b_25p4G.v
   // End of automatics

   
   wire [3:0]                   userio_tx_out_clk0, userio_tx_out_clk1;
   reg [3:0]                    userio_mmcm_not_locked_out0, userio_mmcm_not_locked_out1;
   wire [3:0]                   userio_clk0, userio_clk1;
   wire [3:0]                   userio_bufg_gt_clr_out0, userio_bufg_gt_clr_out1;

   wire [3:0]                   userio_reset_logic0, userio_reset_logic1;
   reg [3:0]                    userio_reset0, userio_reset1;
   wire [3:0]                   userio_gt_reset0, userio_gt_reset1;

   wire [3:0]                   userio_gt_pll_lock0, userio_gt_pll_lock1;
   wire [3:0]                   userio_link_reset_out0, userio_link_reset_out1;

   wire                         qpll0_lock, qpll1_lock;
   wire                         qpll0_outclk, qpll1_outclk;
   wire                         qpll0_outrefclk, qpll1_outrefclk;
   wire                         qpll0_refclklost, qpll1_refclklost;
   wire [3:0]                   qpll0_reset, qpll1_reset;

   // Clocking
   aurora_64b66b_25p4G_gt_common_wrapper
     ultrascale_gt_common_0 (/*AUTOINST*/
                             // Outputs
                             .qpll0_lock        (qpll0_lock),
                             .qpll0_outclk      (qpll0_outclk),
                             .qpll0_outrefclk   (qpll0_outrefclk),
                             .qpll0_refclklost  (qpll0_refclklost),
                             // Inputs
                             .qpll0_refclk      (qsfp0_refclk),
                             .qpll0_reset       (|qpll0_reset),
                             .qpll0_lock_detclk (i_init_clk));
   aurora_64b66b_25p4G_gt_common_wrapper
     ultrascale_gt_common_1 (/*AUTOINST*/
                             // Outputs
                             .qpll0_lock        (qpll1_lock),
                             .qpll0_outclk      (qpll1_outclk),
                             .qpll0_outrefclk   (qpll1_outrefclk),
                             .qpll0_refclklost  (qpll1_refclklost),
                             // Inputs
                             .qpll0_refclk      (qsfp1_refclk),
                             .qpll0_reset       (|qpll1_reset),
                             .qpll0_lock_detclk (i_init_clk));
   
   
   // Aurora IP Instantiation (QSFP0 Links)
   genvar link;
   generate for (link = 0; link < 4; link=link+1) begin : g0_link
      
      if (link == 0) begin : p0_0
         if (link < QSFP0_WIDTH) begin : aurora_if
            
            // FIXME: Does each core need its own BUFG?
            BUFG_GT user_clk_0_0_buf (
                                          .CE(1'b1),
                                          .CEMASK(1'b0),
                                          .CLR(userio_bufg_gt_clr_out0[0]),
                                          .CLRMASK(1'b0),
                                          .DIV(3'b0),
                                          .I(userio_tx_out_clk0[0]),
                                          .O(userio_clk0[0]));

            // leda B_1400 off Asynchronous reset/set/load signal is not a primary input to the current unit
            always @(posedge userio_clk0[0], posedge userio_bufg_gt_clr_out0[0]) begin
               if (userio_bufg_gt_clr_out0[0]) begin
                  userio_mmcm_not_locked_out0[0] <= 1'b0;
               end else begin
                  userio_mmcm_not_locked_out0[0] <= 1'b1;
               end
            end
            // leda B_1400 on Asynchronous reset/set/load signal is not a primary input to the current unit

            assign o_user_clk0[0] = userio_clk0[0];

            // FIXME: userio_reset must be asserted for 128*userio_clk(?) cycles. How do we ensure this?
            userio_reset_logic userio_reset_logic_0_0 (.i_user_clk(i_init_clk), //userio_clk0[0]
                                                       .i_init_clk(i_init_clk),
                                                       .i_reset(i_reset),
                                                       .i_sys_reset(i_reset), //o_sys_reset0[0]
                                                       .i_gt_reset(i_gt_reset),
                                                       .o_sys_reset(userio_reset_logic0[0]),
                                                       .o_gt_reset(userio_gt_reset0[0]));


            always @ (posedge i_init_clk) begin //userio_clk0[0]
               userio_reset0[0] <= userio_reset_logic0[0] | !userio_gt_pll_lock0[0] | userio_link_reset_out0[0];
            end
            
            aurora_64b66b_25p4G aurora_userio_p0_0
            (/*AUTOINST*/
             // Outputs
             .s_axi_tx_tready           (o_s0_axi_tx_tready[0]),
             .m_axi_rx_tdata            (o_m0_axi_rx_tdata[63:0]),
             .m_axi_rx_tkeep            (o_m0_axi_rx_tkeep[7:0]),
             .m_axi_rx_tlast            (o_m0_axi_rx_tlast[0]),
             .m_axi_rx_tvalid           (o_m0_axi_rx_tvalid[0]),
             .txp                       (cp_qsfp0_txp[1]),
             .txn                       (cp_qsfp0_txn[1]),
             .hard_err                  (qsfp0_fatal_alarm[1]),
             .soft_err                  (qsfp0_corr_alarm[1]),
             .channel_up                (o_stat_chan_up0[0]),
             .lane_up                   (o_stat_lane_up0[0]),
             .gt_to_common_qpllreset_out(qpll0_reset[0]),
             .s_axi_rdata               (_nc_p0_s0_axi_rdata[31:0]),
             .s_axi_awready             (_nc_p0_s0_axi_awready),
             .s_axi_wready              (_nc_p0_s0_axi_wready),
             .s_axi_bvalid              (_nc_p0_s0_axi_bvalid),
             .s_axi_bresp               (_nc_p0_s0_axi_bresp[1:0]),
             .s_axi_rresp               (_nc_p0_s0_axi_rresp[1:0]),
             .s_axi_arready             (_nc_p0_s0_axi_arready),
             .s_axi_rvalid              (_nc_p0_s0_axi_rvalid),
             .link_reset_out            (userio_link_reset_out0[0]),
             .gt_powergood              (o_stat_gt_pwrgd0[0]),
             .gt_pll_lock               (userio_gt_pll_lock0[0]),
             .sys_reset_out             (o_sys_reset0[0]),
             .bufg_gt_clr_out           (userio_bufg_gt_clr_out0[0]),
             .tx_out_clk                (userio_tx_out_clk0[0]),
             // Inputs
             .s_axi_tx_tdata            (i_s0_axi_tx_tdata[63:0]),
             .s_axi_tx_tkeep            (i_s0_axi_tx_tkeep[7:0]),
             .s_axi_tx_tlast            (i_s0_axi_tx_tlast[0]),
             .s_axi_tx_tvalid           (i_s0_axi_tx_tvalid[0]),
             .rxp                       (qsfp0_cp_rxp[1]),
             .rxn                       (qsfp0_cp_rxn[1]),
             .refclk1_in                (qsfp0_refclk),
             .mmcm_not_locked           (!userio_mmcm_not_locked_out0[0]),
             .user_clk                  (userio_clk0[0]),
             .sync_clk                  (userio_clk0[0]),
             .reset_pb                  (userio_reset0[0]),
             .gt_rxcdrovrden_in         (1'b0),
             .power_down                (1'b0),
             .loopback                  (3'b0),
             .pma_init                  (userio_gt_reset0[0]),
             .gt_qpllclk_quad1_in       (qpll0_outclk),
             .gt_qpllrefclk_quad1_in    (qpll0_outrefclk),
             .gt_qplllock_quad1_in      (qpll0_lock),
             .gt_qpllrefclklost_quad1   (qpll0_refclklost),
             .s_axi_awaddr              (32'b0),
             .s_axi_araddr              (32'b0),
             .s_axi_wdata               (32'b0),
             .s_axi_wstrb               (4'b0),
             .s_axi_awvalid             (1'b0),
             .s_axi_rready              (1'b0),
             .s_axi_bready              (1'b0),
             .s_axi_arvalid             (1'b0),
             .s_axi_wvalid              (1'b0),
             .init_clk                  (i_init_clk));
         end else begin : tieoff
            assign cp_qsfp0_txp[1] = 1'b0;
            assign cp_qsfp0_txn[1] = 1'b1;

            assign qsfp0_fatal_alarm[1] = 1'b0;
            assign qsfp0_corr_alarm[1] = 1'b0;

            assign o_sys_reset0[0] = 1'b0;
            assign qpll0_reset[0] = 1'b0;

            assign o_stat_chan_up0[0] = 1'b0;
            assign o_stat_lane_up0[0] = 1'b0;
            assign o_stat_gt_pwrgd0[0] = 1'b0;

            assign o_user_clk0[0] = 1'b0;

            assign o_s0_axi_tx_tready[0] = 'b0;
            assign o_m0_axi_rx_tdata[63:0] = 'b0;
            assign o_m0_axi_rx_tkeep[7:0] = 'b0;
            assign o_m0_axi_rx_tlast[0] = 'b0;
            assign o_m0_axi_rx_tvalid[0] = 'b0;
         end
      end

      
      if (link == 1) begin : p0_1
         if (link < QSFP0_WIDTH) begin : aurora_if
            
            // FIXME: Does each core need its own BUFG?
            BUFG_GT user_clk_0_1_buf (
                                          .CE(1'b1),
                                          .CEMASK(1'b0),
                                          .CLR(userio_bufg_gt_clr_out0[1]),
                                          .CLRMASK(1'b0),
                                          .DIV(3'b0),
                                          .I(userio_tx_out_clk0[1]),
                                          .O(userio_clk0[1]));

            // leda B_1400 off Asynchronous reset/set/load signal is not a primary input to the current unit
            always @(posedge userio_clk0[1], posedge userio_bufg_gt_clr_out0[1]) begin
               if (userio_bufg_gt_clr_out0[1]) begin
                  userio_mmcm_not_locked_out0[1] <= 1'b0;
               end else begin
                  userio_mmcm_not_locked_out0[1] <= 1'b1;
               end
            end
            // leda B_1400 on Asynchronous reset/set/load signal is not a primary input to the current unit

            assign o_user_clk0[1] = userio_clk0[1];

            // FIXME: userio_reset must be asserted for 128*userio_clk(?) cycles. How do we ensure this?
            userio_reset_logic userio_reset_logic_0_1 (.i_user_clk(i_init_clk), //userio_clk0[1]
                                                       .i_init_clk(i_init_clk),
                                                       .i_reset(i_reset),
                                                       .i_sys_reset(i_reset), //o_sys_reset0[1]
                                                       .i_gt_reset(i_gt_reset),
                                                       .o_sys_reset(userio_reset_logic0[1]),
                                                       .o_gt_reset(userio_gt_reset0[1]));


            always @ (posedge i_init_clk) begin //userio_clk0[1]
               userio_reset0[1] <= userio_reset_logic0[1] | !userio_gt_pll_lock0[1] | userio_link_reset_out0[1];
            end
            
            aurora_64b66b_25p4G aurora_userio_p0_1
            (/*AUTOINST*/
             // Outputs
             .s_axi_tx_tready           (o_s0_axi_tx_tready[1]),
             .m_axi_rx_tdata            (o_m0_axi_rx_tdata[127:64]),
             .m_axi_rx_tkeep            (o_m0_axi_rx_tkeep[15:8]),
             .m_axi_rx_tlast            (o_m0_axi_rx_tlast[1]),
             .m_axi_rx_tvalid           (o_m0_axi_rx_tvalid[1]),
             .txp                       (cp_qsfp0_txp[2]),
             .txn                       (cp_qsfp0_txn[2]),
             .hard_err                  (qsfp0_fatal_alarm[2]),
             .soft_err                  (qsfp0_corr_alarm[2]),
             .channel_up                (o_stat_chan_up0[1]),
             .lane_up                   (o_stat_lane_up0[1]),
             .gt_to_common_qpllreset_out(qpll0_reset[1]),
             .s_axi_rdata               (_nc_p1_s0_axi_rdata[31:0]),
             .s_axi_awready             (_nc_p1_s0_axi_awready),
             .s_axi_wready              (_nc_p1_s0_axi_wready),
             .s_axi_bvalid              (_nc_p1_s0_axi_bvalid),
             .s_axi_bresp               (_nc_p1_s0_axi_bresp[1:0]),
             .s_axi_rresp               (_nc_p1_s0_axi_rresp[1:0]),
             .s_axi_arready             (_nc_p1_s0_axi_arready),
             .s_axi_rvalid              (_nc_p1_s0_axi_rvalid),
             .link_reset_out            (userio_link_reset_out0[1]),
             .gt_powergood              (o_stat_gt_pwrgd0[1]),
             .gt_pll_lock               (userio_gt_pll_lock0[1]),
             .sys_reset_out             (o_sys_reset0[1]),
             .bufg_gt_clr_out           (userio_bufg_gt_clr_out0[1]),
             .tx_out_clk                (userio_tx_out_clk0[1]),
             // Inputs
             .s_axi_tx_tdata            (i_s0_axi_tx_tdata[127:64]),
             .s_axi_tx_tkeep            (i_s0_axi_tx_tkeep[15:8]),
             .s_axi_tx_tlast            (i_s0_axi_tx_tlast[1]),
             .s_axi_tx_tvalid           (i_s0_axi_tx_tvalid[1]),
             .rxp                       (qsfp0_cp_rxp[2]),
             .rxn                       (qsfp0_cp_rxn[2]),
             .refclk1_in                (qsfp0_refclk),
             .mmcm_not_locked           (!userio_mmcm_not_locked_out0[1]),
             .user_clk                  (userio_clk0[1]),
             .sync_clk                  (userio_clk0[1]),
             .reset_pb                  (userio_reset0[1]),
             .gt_rxcdrovrden_in         (1'b0),
             .power_down                (1'b0),
             .loopback                  (3'b0),
             .pma_init                  (userio_gt_reset0[1]),
             .gt_qpllclk_quad1_in       (qpll0_outclk),
             .gt_qpllrefclk_quad1_in    (qpll0_outrefclk),
             .gt_qplllock_quad1_in      (qpll0_lock),
             .gt_qpllrefclklost_quad1   (qpll0_refclklost),
             .s_axi_awaddr              (32'b0),
             .s_axi_araddr              (32'b0),
             .s_axi_wdata               (32'b0),
             .s_axi_wstrb               (4'b0),
             .s_axi_awvalid             (1'b0),
             .s_axi_rready              (1'b0),
             .s_axi_bready              (1'b0),
             .s_axi_arvalid             (1'b0),
             .s_axi_wvalid              (1'b0),
             .init_clk                  (i_init_clk));
         end else begin : tieoff
            assign cp_qsfp0_txp[2] = 1'b0;
            assign cp_qsfp0_txn[2] = 1'b1;

            assign qsfp0_fatal_alarm[2] = 1'b0;
            assign qsfp0_corr_alarm[2] = 1'b0;

            assign o_sys_reset0[0] = 1'b0;
            assign qpll0_reset[1] = 1'b0;

            assign o_stat_chan_up0[1] = 1'b0;
            assign o_stat_lane_up0[1] = 1'b0;
            assign o_stat_gt_pwrgd0[1] = 1'b0;

            assign o_user_clk0[1] = 1'b0;

            assign o_s0_axi_tx_tready[1] = 'b0;
            assign o_m0_axi_rx_tdata[127:64] = 'b0;
            assign o_m0_axi_rx_tkeep[15:8] = 'b0;
            assign o_m0_axi_rx_tlast[1] = 'b0;
            assign o_m0_axi_rx_tvalid[1] = 'b0;
         end
      end

      
      if (link == 2) begin : p0_2
         if (link < QSFP0_WIDTH) begin : aurora_if
            
            // FIXME: Does each core need its own BUFG?
            BUFG_GT user_clk_0_2_buf (
                                          .CE(1'b1),
                                          .CEMASK(1'b0),
                                          .CLR(userio_bufg_gt_clr_out0[2]),
                                          .CLRMASK(1'b0),
                                          .DIV(3'b0),
                                          .I(userio_tx_out_clk0[2]),
                                          .O(userio_clk0[2]));

            // leda B_1400 off Asynchronous reset/set/load signal is not a primary input to the current unit
            always @(posedge userio_clk0[2], posedge userio_bufg_gt_clr_out0[2]) begin
               if (userio_bufg_gt_clr_out0[2]) begin
                  userio_mmcm_not_locked_out0[2] <= 1'b0;
               end else begin
                  userio_mmcm_not_locked_out0[2] <= 1'b1;
               end
            end
            // leda B_1400 on Asynchronous reset/set/load signal is not a primary input to the current unit

            assign o_user_clk0[2] = userio_clk0[2];

            // FIXME: userio_reset must be asserted for 128*userio_clk(?) cycles. How do we ensure this?
            userio_reset_logic userio_reset_logic_0_2 (.i_user_clk(i_init_clk), //userio_clk0[2]
                                                       .i_init_clk(i_init_clk),
                                                       .i_reset(i_reset),
                                                       .i_sys_reset(i_reset), //o_sys_reset0[2]
                                                       .i_gt_reset(i_gt_reset),
                                                       .o_sys_reset(userio_reset_logic0[2]),
                                                       .o_gt_reset(userio_gt_reset0[2]));


            always @ (posedge i_init_clk) begin //userio_clk0[2]
               userio_reset0[2] <= userio_reset_logic0[2] | !userio_gt_pll_lock0[2] | userio_link_reset_out0[2];
            end
            
            aurora_64b66b_25p4G aurora_userio_p0_2
            (/*AUTOINST*/
             // Outputs
             .s_axi_tx_tready           (o_s0_axi_tx_tready[2]),
             .m_axi_rx_tdata            (o_m0_axi_rx_tdata[191:128]),
             .m_axi_rx_tkeep            (o_m0_axi_rx_tkeep[23:16]),
             .m_axi_rx_tlast            (o_m0_axi_rx_tlast[2]),
             .m_axi_rx_tvalid           (o_m0_axi_rx_tvalid[2]),
             .txp                       (cp_qsfp0_txp[3]),
             .txn                       (cp_qsfp0_txn[3]),
             .hard_err                  (qsfp0_fatal_alarm[3]),
             .soft_err                  (qsfp0_corr_alarm[3]),
             .channel_up                (o_stat_chan_up0[2]),
             .lane_up                   (o_stat_lane_up0[2]),
             .gt_to_common_qpllreset_out(qpll0_reset[2]),
             .s_axi_rdata               (_nc_p2_s0_axi_rdata[31:0]),
             .s_axi_awready             (_nc_p2_s0_axi_awready),
             .s_axi_wready              (_nc_p2_s0_axi_wready),
             .s_axi_bvalid              (_nc_p2_s0_axi_bvalid),
             .s_axi_bresp               (_nc_p2_s0_axi_bresp[1:0]),
             .s_axi_rresp               (_nc_p2_s0_axi_rresp[1:0]),
             .s_axi_arready             (_nc_p2_s0_axi_arready),
             .s_axi_rvalid              (_nc_p2_s0_axi_rvalid),
             .link_reset_out            (userio_link_reset_out0[2]),
             .gt_powergood              (o_stat_gt_pwrgd0[2]),
             .gt_pll_lock               (userio_gt_pll_lock0[2]),
             .sys_reset_out             (o_sys_reset0[2]),
             .bufg_gt_clr_out           (userio_bufg_gt_clr_out0[2]),
             .tx_out_clk                (userio_tx_out_clk0[2]),
             // Inputs
             .s_axi_tx_tdata            (i_s0_axi_tx_tdata[191:128]),
             .s_axi_tx_tkeep            (i_s0_axi_tx_tkeep[23:16]),
             .s_axi_tx_tlast            (i_s0_axi_tx_tlast[2]),
             .s_axi_tx_tvalid           (i_s0_axi_tx_tvalid[2]),
             .rxp                       (qsfp0_cp_rxp[3]),
             .rxn                       (qsfp0_cp_rxn[3]),
             .refclk1_in                (qsfp0_refclk),
             .mmcm_not_locked           (!userio_mmcm_not_locked_out0[2]),
             .user_clk                  (userio_clk0[2]),
             .sync_clk                  (userio_clk0[2]),
             .reset_pb                  (userio_reset0[2]),
             .gt_rxcdrovrden_in         (1'b0),
             .power_down                (1'b0),
             .loopback                  (3'b0),
             .pma_init                  (userio_gt_reset0[2]),
             .gt_qpllclk_quad1_in       (qpll0_outclk),
             .gt_qpllrefclk_quad1_in    (qpll0_outrefclk),
             .gt_qplllock_quad1_in      (qpll0_lock),
             .gt_qpllrefclklost_quad1   (qpll0_refclklost),
             .s_axi_awaddr              (32'b0),
             .s_axi_araddr              (32'b0),
             .s_axi_wdata               (32'b0),
             .s_axi_wstrb               (4'b0),
             .s_axi_awvalid             (1'b0),
             .s_axi_rready              (1'b0),
             .s_axi_bready              (1'b0),
             .s_axi_arvalid             (1'b0),
             .s_axi_wvalid              (1'b0),
             .init_clk                  (i_init_clk));
         end else begin : tieoff
            assign cp_qsfp0_txp[3] = 1'b0;
            assign cp_qsfp0_txn[3] = 1'b1;

            assign qsfp0_fatal_alarm[3] = 1'b0;
            assign qsfp0_corr_alarm[3] = 1'b0;

            assign o_sys_reset0[2] = 1'b0;
            assign qpll0_reset[2] = 1'b0;

            assign o_stat_chan_up0[2] = 1'b0;
            assign o_stat_lane_up0[2] = 1'b0;
            assign o_stat_gt_pwrgd0[2] = 1'b0;

            assign o_user_clk0[2] = 1'b0;

            assign o_s0_axi_tx_tready[2] = 'b0;
            assign o_m0_axi_rx_tdata[191:128] = 'b0;
            assign o_m0_axi_rx_tkeep[23:16] = 'b0;
            assign o_m0_axi_rx_tlast[2] = 'b0;
            assign o_m0_axi_rx_tvalid[2] = 'b0;
         end
      end

      
      if (link == 3) begin : p0_3
         if (link < QSFP0_WIDTH) begin : aurora_if
            
            // FIXME: Does each core need its own BUFG?
            BUFG_GT user_clk_0_3_buf (
                                          .CE(1'b1),
                                          .CEMASK(1'b0),
                                          .CLR(userio_bufg_gt_clr_out0[3]),
                                          .CLRMASK(1'b0),
                                          .DIV(3'b0),
                                          .I(userio_tx_out_clk0[3]),
                                          .O(userio_clk0[3]));

            // leda B_1400 off Asynchronous reset/set/load signal is not a primary input to the current unit
            always @(posedge userio_clk0[3], posedge userio_bufg_gt_clr_out0[3]) begin
               if (userio_bufg_gt_clr_out0[3]) begin
                  userio_mmcm_not_locked_out0[3] <= 1'b0;
               end else begin
                  userio_mmcm_not_locked_out0[3] <= 1'b1;
               end
            end
            // leda B_1400 on Asynchronous reset/set/load signal is not a primary input to the current unit

            assign o_user_clk0[3] = userio_clk0[3];

            // FIXME: userio_reset must be asserted for 128*userio_clk(?) cycles. How do we ensure this?
            userio_reset_logic userio_reset_logic_0_3 (.i_user_clk(i_init_clk), //userio_clk0[3]
                                                       .i_init_clk(i_init_clk),
                                                       .i_reset(i_reset),
                                                       .i_sys_reset(i_reset), //o_sys_reset0[3]
                                                       .i_gt_reset(i_gt_reset),
                                                       .o_sys_reset(userio_reset_logic0[3]),
                                                       .o_gt_reset(userio_gt_reset0[3]));


            always @ (posedge i_init_clk) begin //userio_clk0[3]
               userio_reset0[3] <= userio_reset_logic0[3] | !userio_gt_pll_lock0[3] | userio_link_reset_out0[3];
            end
            
            aurora_64b66b_25p4G aurora_userio_p0_3
            (/*AUTOINST*/
             // Outputs
             .s_axi_tx_tready           (o_s0_axi_tx_tready[3]),
             .m_axi_rx_tdata            (o_m0_axi_rx_tdata[255:192]),
             .m_axi_rx_tkeep            (o_m0_axi_rx_tkeep[31:24]),
             .m_axi_rx_tlast            (o_m0_axi_rx_tlast[3]),
             .m_axi_rx_tvalid           (o_m0_axi_rx_tvalid[3]),
             .txp                       (cp_qsfp0_txp[4]),
             .txn                       (cp_qsfp0_txn[4]),
             .hard_err                  (qsfp0_fatal_alarm[4]),
             .soft_err                  (qsfp0_corr_alarm[4]),
             .channel_up                (o_stat_chan_up0[3]),
             .lane_up                   (o_stat_lane_up0[3]),
             .gt_to_common_qpllreset_out(qpll0_reset[3]),
             .s_axi_rdata               (_nc_p3_s0_axi_rdata[31:0]),
             .s_axi_awready             (_nc_p3_s0_axi_awready),
             .s_axi_wready              (_nc_p3_s0_axi_wready),
             .s_axi_bvalid              (_nc_p3_s0_axi_bvalid),
             .s_axi_bresp               (_nc_p3_s0_axi_bresp[1:0]),
             .s_axi_rresp               (_nc_p3_s0_axi_rresp[1:0]),
             .s_axi_arready             (_nc_p3_s0_axi_arready),
             .s_axi_rvalid              (_nc_p3_s0_axi_rvalid),
             .link_reset_out            (userio_link_reset_out0[3]),
             .gt_powergood              (o_stat_gt_pwrgd0[3]),
             .gt_pll_lock               (userio_gt_pll_lock0[3]),
             .sys_reset_out             (o_sys_reset0[3]),
             .bufg_gt_clr_out           (userio_bufg_gt_clr_out0[3]),
             .tx_out_clk                (userio_tx_out_clk0[3]),
             // Inputs
             .s_axi_tx_tdata            (i_s0_axi_tx_tdata[255:192]),
             .s_axi_tx_tkeep            (i_s0_axi_tx_tkeep[31:24]),
             .s_axi_tx_tlast            (i_s0_axi_tx_tlast[3]),
             .s_axi_tx_tvalid           (i_s0_axi_tx_tvalid[3]),
             .rxp                       (qsfp0_cp_rxp[4]),
             .rxn                       (qsfp0_cp_rxn[4]),
             .refclk1_in                (qsfp0_refclk),
             .mmcm_not_locked           (!userio_mmcm_not_locked_out0[3]),
             .user_clk                  (userio_clk0[3]),
             .sync_clk                  (userio_clk0[3]),
             .reset_pb                  (userio_reset0[3]),
             .gt_rxcdrovrden_in         (1'b0),
             .power_down                (1'b0),
             .loopback                  (3'b0),
             .pma_init                  (userio_gt_reset0[3]),
             .gt_qpllclk_quad1_in       (qpll0_outclk),
             .gt_qpllrefclk_quad1_in    (qpll0_outrefclk),
             .gt_qplllock_quad1_in      (qpll0_lock),
             .gt_qpllrefclklost_quad1   (qpll0_refclklost),
             .s_axi_awaddr              (32'b0),
             .s_axi_araddr              (32'b0),
             .s_axi_wdata               (32'b0),
             .s_axi_wstrb               (4'b0),
             .s_axi_awvalid             (1'b0),
             .s_axi_rready              (1'b0),
             .s_axi_bready              (1'b0),
             .s_axi_arvalid             (1'b0),
             .s_axi_wvalid              (1'b0),
             .init_clk                  (i_init_clk));
         end else begin : tieoff
            assign cp_qsfp0_txp[4] = 1'b0;
            assign cp_qsfp0_txn[4] = 1'b1;

            assign qsfp0_fatal_alarm[4] = 1'b0;
            assign qsfp0_corr_alarm[4] = 1'b0;

            assign o_sys_reset0[3] = 1'b0;
            assign qpll0_reset[3] = 1'b0;

            assign o_stat_chan_up0[3] = 1'b0;
            assign o_stat_lane_up0[3] = 1'b0;
            assign o_stat_gt_pwrgd0[3] = 1'b0;

            assign o_user_clk0[3] = 1'b0;

            assign o_s0_axi_tx_tready[3] = 'b0;
            assign o_m0_axi_rx_tdata[255:192] = 'b0;
            assign o_m0_axi_rx_tkeep[31:24] = 'b0;
            assign o_m0_axi_rx_tlast[3] = 'b0;
            assign o_m0_axi_rx_tvalid[3] = 'b0;
         end
      end

   end endgenerate

   generate for (link = 0; link < 4; link=link+1) begin : g1_link

      if (link == 0) begin : p1_0
         if (link < QSFP1_WIDTH) begin : aurora_if
            
            // FIXME: Does each core need its own BUFG?
            BUFG_GT user_clk_1_0_buf (
                                          .CE(1'b1),
                                          .CEMASK(1'b0),
                                          .CLR(userio_bufg_gt_clr_out1[0]),
                                          .CLRMASK(1'b0),
                                          .DIV(3'b0),
                                          .I(userio_tx_out_clk1[0]),
                                          .O(userio_clk1[0]));

            // leda B_1400 off Asynchronous reset/set/load signal is not a primary input to the current unit
            always @(posedge userio_clk1[0], posedge userio_bufg_gt_clr_out1[0]) begin
               if (userio_bufg_gt_clr_out1[0]) begin
                  userio_mmcm_not_locked_out1[0] <= 1'b0;
               end else begin
                  userio_mmcm_not_locked_out1[0] <= 1'b1;
               end
            end
            // leda B_1400 on Asynchronous reset/set/load signal is not a primary input to the current unit

            assign o_user_clk1[0] = userio_clk1[0];

            // FIXME: userio_reset must be asserted for 128*userio_clk(?) cycles. How do we ensure this?
            userio_reset_logic userio_reset_logic_1_0 (.i_user_clk(i_init_clk), //userio_clk1[0]
                                                       .i_init_clk(i_init_clk),
                                                       .i_reset(i_reset),
                                                       .i_sys_reset(i_reset), //o_sys_reset1[0]
                                                       .i_gt_reset(i_gt_reset),
                                                       .o_sys_reset(userio_reset_logic1[0]),
                                                       .o_gt_reset(userio_gt_reset1[0]));


            always @ (posedge i_init_clk) begin //userio_clk1[0]
               userio_reset1[0] <= userio_reset_logic1[0] | !userio_gt_pll_lock1[0] | userio_link_reset_out1[0];
            end
            
            aurora_64b66b_25p4G aurora_userio_p1_0
            (/*AUTOINST*/
             // Outputs
             .s_axi_tx_tready           (o_s1_axi_tx_tready[0]),
             .m_axi_rx_tdata            (o_m1_axi_rx_tdata[63:0]),
             .m_axi_rx_tkeep            (o_m1_axi_rx_tkeep[7:0]),
             .m_axi_rx_tlast            (o_m1_axi_rx_tlast[0]),
             .m_axi_rx_tvalid           (o_m1_axi_rx_tvalid[0]),
             .txp                       (cp_qsfp1_txp[1]),
             .txn                       (cp_qsfp1_txn[1]),
             .hard_err                  (qsfp1_fatal_alarm[1]),
             .soft_err                  (qsfp1_corr_alarm[1]),
             .channel_up                (o_stat_chan_up1[0]),
             .lane_up                   (o_stat_lane_up1[0]),
             .gt_to_common_qpllreset_out(qpll1_reset[0]),
             .s_axi_rdata               (_nc_p0_s1_axi_rdata[31:0]),
             .s_axi_awready             (_nc_p0_s1_axi_awready),
             .s_axi_wready              (_nc_p0_s1_axi_wready),
             .s_axi_bvalid              (_nc_p0_s1_axi_bvalid),
             .s_axi_bresp               (_nc_p0_s1_axi_bresp[1:0]),
             .s_axi_rresp               (_nc_p0_s1_axi_rresp[1:0]),
             .s_axi_arready             (_nc_p0_s1_axi_arready),
             .s_axi_rvalid              (_nc_p0_s1_axi_rvalid),
             .link_reset_out            (userio_link_reset_out1[0]),
             .gt_powergood              (o_stat_gt_pwrgd1[0]),
             .gt_pll_lock               (userio_gt_pll_lock1[0]),
             .sys_reset_out             (o_sys_reset1[0]),
             .bufg_gt_clr_out           (userio_bufg_gt_clr_out1[0]),
             .tx_out_clk                (userio_tx_out_clk1[0]),
             // Inputs
             .s_axi_tx_tdata            (i_s1_axi_tx_tdata[63:0]),
             .s_axi_tx_tkeep            (i_s1_axi_tx_tkeep[7:0]),
             .s_axi_tx_tlast            (i_s1_axi_tx_tlast[0]),
             .s_axi_tx_tvalid           (i_s1_axi_tx_tvalid[0]),
             .rxp                       (qsfp1_cp_rxp[1]),
             .rxn                       (qsfp1_cp_rxn[1]),
             .refclk1_in                (qsfp1_refclk),
             .mmcm_not_locked           (!userio_mmcm_not_locked_out1[0]),
             .user_clk                  (userio_clk1[0]),
             .sync_clk                  (userio_clk1[0]),
             .reset_pb                  (userio_reset1[0]),
             .gt_rxcdrovrden_in         (1'b0),
             .power_down                (1'b0),
             .loopback                  (3'b0),
             .pma_init                  (userio_gt_reset1[0]),
             .gt_qpllclk_quad1_in       (qpll1_outclk),
             .gt_qpllrefclk_quad1_in    (qpll1_outrefclk),
             .gt_qplllock_quad1_in      (qpll1_lock),
             .gt_qpllrefclklost_quad1   (qpll1_refclklost),
             .s_axi_awaddr              (32'b0),
             .s_axi_araddr              (32'b0),
             .s_axi_wdata               (32'b0),
             .s_axi_wstrb               (4'b0),
             .s_axi_awvalid             (1'b0),
             .s_axi_rready              (1'b0),
             .s_axi_bready              (1'b0),
             .s_axi_arvalid             (1'b0),
             .s_axi_wvalid              (1'b0),
             .init_clk                  (i_init_clk));
         end else begin : tieoff
            assign cp_qsfp1_txp[1] = 1'b0;
            assign cp_qsfp1_txn[1] = 1'b1;

            assign qsfp1_fatal_alarm[1] = 1'b0;
            assign qsfp1_corr_alarm[1] = 1'b0;

            assign o_sys_reset1[0] = 1'b0;
            assign qpll1_reset[0] = 1'b0;

            assign o_stat_chan_up1[0] = 1'b0;
            assign o_stat_lane_up1[0] = 1'b0;
            assign o_stat_gt_pwrgd1[0] = 1'b0;

            assign o_user_clk1[0] = 1'b0;

            assign o_s1_axi_tx_tready[0] = 'b0;
            assign o_m1_axi_rx_tdata[63:0] = 'b0;
            assign o_m1_axi_rx_tkeep[7:0] = 'b0;
            assign o_m1_axi_rx_tlast[0] = 'b0;
            assign o_m1_axi_rx_tvalid[0] = 'b0;
         end
      end

      
      if (link == 1) begin : p1_1
         if (link < QSFP1_WIDTH) begin : aurora_if
            
            // FIXME: Does each core need its own BUFG?
            BUFG_GT user_clk_1_1_buf (
                                          .CE(1'b1),
                                          .CEMASK(1'b0),
                                          .CLR(userio_bufg_gt_clr_out1[1]),
                                          .CLRMASK(1'b0),
                                          .DIV(3'b0),
                                          .I(userio_tx_out_clk1[1]),
                                          .O(userio_clk1[1]));

            // leda B_1400 off Asynchronous reset/set/load signal is not a primary input to the current unit
            always @(posedge userio_clk1[1], posedge userio_bufg_gt_clr_out1[1]) begin
               if (userio_bufg_gt_clr_out1[1]) begin
                  userio_mmcm_not_locked_out1[1] <= 1'b0;
               end else begin
                  userio_mmcm_not_locked_out1[1] <= 1'b1;
               end
            end
            // leda B_1400 on Asynchronous reset/set/load signal is not a primary input to the current unit

            assign o_user_clk1[1] = userio_clk1[1];

            // FIXME: userio_reset must be asserted for 128*userio_clk(?) cycles. How do we ensure this?
            userio_reset_logic userio_reset_logic_1_1 (.i_user_clk(i_init_clk), //userio_clk1[1]
                                                       .i_init_clk(i_init_clk),
                                                       .i_reset(i_reset),
                                                       .i_sys_reset(i_reset), //o_sys_reset1[1]
                                                       .i_gt_reset(i_gt_reset),
                                                       .o_sys_reset(userio_reset_logic1[1]),
                                                       .o_gt_reset(userio_gt_reset1[1]));


            always @ (posedge i_init_clk) begin //userio_clk1[1]
               userio_reset1[1] <= userio_reset_logic1[1] | !userio_gt_pll_lock1[1] | userio_link_reset_out1[1];
            end
            
            aurora_64b66b_25p4G aurora_userio_p1_1
            (/*AUTOINST*/
             // Outputs
             .s_axi_tx_tready           (o_s1_axi_tx_tready[1]),
             .m_axi_rx_tdata            (o_m1_axi_rx_tdata[127:64]),
             .m_axi_rx_tkeep            (o_m1_axi_rx_tkeep[15:8]),
             .m_axi_rx_tlast            (o_m1_axi_rx_tlast[1]),
             .m_axi_rx_tvalid           (o_m1_axi_rx_tvalid[1]),
             .txp                       (cp_qsfp1_txp[2]),
             .txn                       (cp_qsfp1_txn[2]),
             .hard_err                  (qsfp1_fatal_alarm[2]),
             .soft_err                  (qsfp1_corr_alarm[2]),
             .channel_up                (o_stat_chan_up1[1]),
             .lane_up                   (o_stat_lane_up1[1]),
             .gt_to_common_qpllreset_out(qpll1_reset[1]),
             .s_axi_rdata               (_nc_p1_s1_axi_rdata[31:0]),
             .s_axi_awready             (_nc_p1_s1_axi_awready),
             .s_axi_wready              (_nc_p1_s1_axi_wready),
             .s_axi_bvalid              (_nc_p1_s1_axi_bvalid),
             .s_axi_bresp               (_nc_p1_s1_axi_bresp[1:0]),
             .s_axi_rresp               (_nc_p1_s1_axi_rresp[1:0]),
             .s_axi_arready             (_nc_p1_s1_axi_arready),
             .s_axi_rvalid              (_nc_p1_s1_axi_rvalid),
             .link_reset_out            (userio_link_reset_out1[1]),
             .gt_powergood              (o_stat_gt_pwrgd1[1]),
             .gt_pll_lock               (userio_gt_pll_lock1[1]),
             .sys_reset_out             (o_sys_reset1[1]),
             .bufg_gt_clr_out           (userio_bufg_gt_clr_out1[1]),
             .tx_out_clk                (userio_tx_out_clk1[1]),
             // Inputs
             .s_axi_tx_tdata            (i_s1_axi_tx_tdata[127:64]),
             .s_axi_tx_tkeep            (i_s1_axi_tx_tkeep[15:8]),
             .s_axi_tx_tlast            (i_s1_axi_tx_tlast[1]),
             .s_axi_tx_tvalid           (i_s1_axi_tx_tvalid[1]),
             .rxp                       (qsfp1_cp_rxp[2]),
             .rxn                       (qsfp1_cp_rxn[2]),
             .refclk1_in                (qsfp1_refclk),
             .mmcm_not_locked           (!userio_mmcm_not_locked_out1[1]),
             .user_clk                  (userio_clk1[1]),
             .sync_clk                  (userio_clk1[1]),
             .reset_pb                  (userio_reset1[1]),
             .gt_rxcdrovrden_in         (1'b0),
             .power_down                (1'b0),
             .loopback                  (3'b0),
             .pma_init                  (userio_gt_reset1[1]),
             .gt_qpllclk_quad1_in       (qpll1_outclk),
             .gt_qpllrefclk_quad1_in    (qpll1_outrefclk),
             .gt_qplllock_quad1_in      (qpll1_lock),
             .gt_qpllrefclklost_quad1   (qpll1_refclklost),
             .s_axi_awaddr              (32'b0),
             .s_axi_araddr              (32'b0),
             .s_axi_wdata               (32'b0),
             .s_axi_wstrb               (4'b0),
             .s_axi_awvalid             (1'b0),
             .s_axi_rready              (1'b0),
             .s_axi_bready              (1'b0),
             .s_axi_arvalid             (1'b0),
             .s_axi_wvalid              (1'b0),
             .init_clk                  (i_init_clk));
         end else begin : tieoff
            assign cp_qsfp1_txp[2] = 1'b0;
            assign cp_qsfp1_txn[2] = 1'b1;

            assign qsfp1_fatal_alarm[2] = 1'b0;
            assign qsfp1_corr_alarm[2] = 1'b0;

            assign o_sys_reset1[1] = 1'b0;
            assign qpll1_reset[1] = 1'b0;

            assign o_stat_chan_up1[1] = 1'b0;
            assign o_stat_lane_up1[1] = 1'b0;
            assign o_stat_gt_pwrgd1[1] = 1'b0;

            assign o_user_clk1[1] = 1'b0;

            assign o_s1_axi_tx_tready[1] = 'b0;
            assign o_m1_axi_rx_tdata[127:64] = 'b0;
            assign o_m1_axi_rx_tkeep[15:8] = 'b0;
            assign o_m1_axi_rx_tlast[1] = 'b0;
            assign o_m1_axi_rx_tvalid[1] = 'b0;
         end
      end

      
      if (link == 2) begin : p1_2
         if (link < QSFP1_WIDTH) begin : aurora_if
            
            // FIXME: Does each core need its own BUFG?
            BUFG_GT user_clk_1_2_buf (
                                          .CE(1'b1),
                                          .CEMASK(1'b0),
                                          .CLR(userio_bufg_gt_clr_out1[2]),
                                          .CLRMASK(1'b0),
                                          .DIV(3'b0),
                                          .I(userio_tx_out_clk1[2]),
                                          .O(userio_clk1[2]));

            // leda B_1400 off Asynchronous reset/set/load signal is not a primary input to the current unit
            always @(posedge userio_clk1[2], posedge userio_bufg_gt_clr_out1[2]) begin
               if (userio_bufg_gt_clr_out1[2]) begin
                  userio_mmcm_not_locked_out1[2] <= 1'b0;
               end else begin
                  userio_mmcm_not_locked_out1[2] <= 1'b1;
               end
            end
            // leda B_1400 on Asynchronous reset/set/load signal is not a primary input to the current unit

            assign o_user_clk1[2] = userio_clk1[2];

            // FIXME: userio_reset must be asserted for 128*userio_clk(?) cycles. How do we ensure this?
            userio_reset_logic userio_reset_logic_1_2 (.i_user_clk(i_init_clk), //userio_clk1[2]
                                                       .i_init_clk(i_init_clk),
                                                       .i_reset(i_reset),
                                                       .i_sys_reset(i_reset), //o_sys_reset1[2]
                                                       .i_gt_reset(i_gt_reset),
                                                       .o_sys_reset(userio_reset_logic1[2]),
                                                       .o_gt_reset(userio_gt_reset1[2]));


            always @ (posedge i_init_clk) begin //userio_clk1[2]
               userio_reset1[2] <= userio_reset_logic1[2] | !userio_gt_pll_lock1[2] | userio_link_reset_out1[2];
            end
            
            aurora_64b66b_25p4G aurora_userio_p1_2
            (/*AUTOINST*/
             // Outputs
             .s_axi_tx_tready           (o_s1_axi_tx_tready[2]),
             .m_axi_rx_tdata            (o_m1_axi_rx_tdata[191:128]),
             .m_axi_rx_tkeep            (o_m1_axi_rx_tkeep[23:16]),
             .m_axi_rx_tlast            (o_m1_axi_rx_tlast[2]),
             .m_axi_rx_tvalid           (o_m1_axi_rx_tvalid[2]),
             .txp                       (cp_qsfp1_txp[3]),
             .txn                       (cp_qsfp1_txn[3]),
             .hard_err                  (qsfp1_fatal_alarm[3]),
             .soft_err                  (qsfp1_corr_alarm[3]),
             .channel_up                (o_stat_chan_up1[2]),
             .lane_up                   (o_stat_lane_up1[2]),
             .gt_to_common_qpllreset_out(qpll1_reset[2]),
             .s_axi_rdata               (_nc_p2_s1_axi_rdata[31:0]),
             .s_axi_awready             (_nc_p2_s1_axi_awready),
             .s_axi_wready              (_nc_p2_s1_axi_wready),
             .s_axi_bvalid              (_nc_p2_s1_axi_bvalid),
             .s_axi_bresp               (_nc_p2_s1_axi_bresp[1:0]),
             .s_axi_rresp               (_nc_p2_s1_axi_rresp[1:0]),
             .s_axi_arready             (_nc_p2_s1_axi_arready),
             .s_axi_rvalid              (_nc_p2_s1_axi_rvalid),
             .link_reset_out            (userio_link_reset_out1[2]),
             .gt_powergood              (o_stat_gt_pwrgd1[2]),
             .gt_pll_lock               (userio_gt_pll_lock1[2]),
             .sys_reset_out             (o_sys_reset1[2]),
             .bufg_gt_clr_out           (userio_bufg_gt_clr_out1[2]),
             .tx_out_clk                (userio_tx_out_clk1[2]),
             // Inputs
             .s_axi_tx_tdata            (i_s1_axi_tx_tdata[191:128]),
             .s_axi_tx_tkeep            (i_s1_axi_tx_tkeep[23:16]),
             .s_axi_tx_tlast            (i_s1_axi_tx_tlast[2]),
             .s_axi_tx_tvalid           (i_s1_axi_tx_tvalid[2]),
             .rxp                       (qsfp1_cp_rxp[3]),
             .rxn                       (qsfp1_cp_rxn[3]),
             .refclk1_in                (qsfp1_refclk),
             .mmcm_not_locked           (!userio_mmcm_not_locked_out1[2]),
             .user_clk                  (userio_clk1[2]),
             .sync_clk                  (userio_clk1[2]),
             .reset_pb                  (userio_reset1[2]),
             .gt_rxcdrovrden_in         (1'b0),
             .power_down                (1'b0),
             .loopback                  (3'b0),
             .pma_init                  (userio_gt_reset1[2]),
             .gt_qpllclk_quad1_in       (qpll1_outclk),
             .gt_qpllrefclk_quad1_in    (qpll1_outrefclk),
             .gt_qplllock_quad1_in      (qpll1_lock),
             .gt_qpllrefclklost_quad1   (qpll1_refclklost),
             .s_axi_awaddr              (32'b0),
             .s_axi_araddr              (32'b0),
             .s_axi_wdata               (32'b0),
             .s_axi_wstrb               (4'b0),
             .s_axi_awvalid             (1'b0),
             .s_axi_rready              (1'b0),
             .s_axi_bready              (1'b0),
             .s_axi_arvalid             (1'b0),
             .s_axi_wvalid              (1'b0),
             .init_clk                  (i_init_clk));
         end else begin : tieoff
            assign cp_qsfp1_txp[3] = 1'b0;
            assign cp_qsfp1_txn[3] = 1'b1;

            assign qsfp1_fatal_alarm[3] = 1'b0;
            assign qsfp1_corr_alarm[3] = 1'b0;

            assign o_sys_reset1[2] = 1'b0;
            assign qpll1_reset[2] = 1'b0;

            assign o_stat_chan_up1[2] = 1'b0;
            assign o_stat_lane_up1[2] = 1'b0;
            assign o_stat_gt_pwrgd1[2] = 1'b0;

            assign o_user_clk1[2] = 1'b0;

            assign o_s1_axi_tx_tready[2] = 'b0;
            assign o_m1_axi_rx_tdata[191:128] = 'b0;
            assign o_m1_axi_rx_tkeep[23:16] = 'b0;
            assign o_m1_axi_rx_tlast[2] = 'b0;
            assign o_m1_axi_rx_tvalid[2] = 'b0;
         end
      end

      
      if (link == 3) begin : p1_3
         if (link < QSFP1_WIDTH) begin : aurora_if
            
            // FIXME: Does each core need its own BUFG?
            BUFG_GT user_clk_1_3_buf (
                                          .CE(1'b1),
                                          .CEMASK(1'b0),
                                          .CLR(userio_bufg_gt_clr_out1[3]),
                                          .CLRMASK(1'b0),
                                          .DIV(3'b0),
                                          .I(userio_tx_out_clk1[3]),
                                          .O(userio_clk1[3]));

            // leda B_1400 off Asynchronous reset/set/load signal is not a primary input to the current unit
            always @(posedge userio_clk1[3], posedge userio_bufg_gt_clr_out1[3]) begin
               if (userio_bufg_gt_clr_out1[3]) begin
                  userio_mmcm_not_locked_out1[3] <= 1'b0;
               end else begin
                  userio_mmcm_not_locked_out1[3] <= 1'b1;
               end
            end
            // leda B_1400 on Asynchronous reset/set/load signal is not a primary input to the current unit

            assign o_user_clk1[3] = userio_clk1[3];

            // FIXME: userio_reset must be asserted for 128*userio_clk(?) cycles. How do we ensure this?
            userio_reset_logic userio_reset_logic_1_3 (.i_user_clk(i_init_clk), //userio_clk1[3]
                                                         .i_init_clk(i_init_clk),
                                                         .i_reset(i_reset),
                                                         .i_sys_reset(i_reset), //o_sys_reset1[3]
                                                         .i_gt_reset(i_gt_reset),
                                                         .o_sys_reset(userio_reset_logic1[3]),
                                                         .o_gt_reset(userio_gt_reset1[3]));


            always @ (posedge i_init_clk) begin //userio_clk1[3]
               userio_reset1[3] <= userio_reset_logic1[3] | !userio_gt_pll_lock1[3] | userio_link_reset_out1[3];
            end
            
            aurora_64b66b_25p4G aurora_userio_p1_3
            (/*AUTOINST*/
             // Outputs
             .s_axi_tx_tready           (o_s1_axi_tx_tready[3]),
             .m_axi_rx_tdata            (o_m1_axi_rx_tdata[255:192]),
             .m_axi_rx_tkeep            (o_m1_axi_rx_tkeep[31:24]),
             .m_axi_rx_tlast            (o_m1_axi_rx_tlast[3]),
             .m_axi_rx_tvalid           (o_m1_axi_rx_tvalid[3]),
             .txp                       (cp_qsfp1_txp[4]),
             .txn                       (cp_qsfp1_txn[4]),
             .hard_err                  (qsfp1_fatal_alarm[4]),
             .soft_err                  (qsfp1_corr_alarm[4]),
             .channel_up                (o_stat_chan_up1[3]),
             .lane_up                   (o_stat_lane_up1[3]),
             .gt_to_common_qpllreset_out(qpll1_reset[3]),
             .s_axi_rdata               (_nc_p3_s1_axi_rdata[31:0]),
             .s_axi_awready             (_nc_p3_s1_axi_awready),
             .s_axi_wready              (_nc_p3_s1_axi_wready),
             .s_axi_bvalid              (_nc_p3_s1_axi_bvalid),
             .s_axi_bresp               (_nc_p3_s1_axi_bresp[1:0]),
             .s_axi_rresp               (_nc_p3_s1_axi_rresp[1:0]),
             .s_axi_arready             (_nc_p3_s1_axi_arready),
             .s_axi_rvalid              (_nc_p3_s1_axi_rvalid),
             .link_reset_out            (userio_link_reset_out1[3]),
             .gt_powergood              (o_stat_gt_pwrgd1[3]),
             .gt_pll_lock               (userio_gt_pll_lock1[3]),
             .sys_reset_out             (o_sys_reset1[3]),
             .bufg_gt_clr_out           (userio_bufg_gt_clr_out1[3]),
             .tx_out_clk                (userio_tx_out_clk1[3]),
             // Inputs
             .s_axi_tx_tdata            (i_s1_axi_tx_tdata[255:192]),
             .s_axi_tx_tkeep            (i_s1_axi_tx_tkeep[31:24]),
             .s_axi_tx_tlast            (i_s1_axi_tx_tlast[3]),
             .s_axi_tx_tvalid           (i_s1_axi_tx_tvalid[3]),
             .rxp                       (qsfp1_cp_rxp[4]),
             .rxn                       (qsfp1_cp_rxn[4]),
             .refclk1_in                (qsfp1_refclk),
             .mmcm_not_locked           (!userio_mmcm_not_locked_out1[3]),
             .user_clk                  (userio_clk1[3]),
             .sync_clk                  (userio_clk1[3]),
             .reset_pb                  (userio_reset1[3]),
             .gt_rxcdrovrden_in         (1'b0),
             .power_down                (1'b0),
             .loopback                  (3'b0),
             .pma_init                  (userio_gt_reset1[3]),
             .gt_qpllclk_quad1_in       (qpll1_outclk),
             .gt_qpllrefclk_quad1_in    (qpll1_outrefclk),
             .gt_qplllock_quad1_in      (qpll1_lock),
             .gt_qpllrefclklost_quad1   (qpll1_refclklost),
             .s_axi_awaddr              (32'b0),
             .s_axi_araddr              (32'b0),
             .s_axi_wdata               (32'b0),
             .s_axi_wstrb               (4'b0),
             .s_axi_awvalid             (1'b0),
             .s_axi_rready              (1'b0),
             .s_axi_bready              (1'b0),
             .s_axi_arvalid             (1'b0),
             .s_axi_wvalid              (1'b0),
             .init_clk                  (i_init_clk));
         end else begin : tieoff
            assign cp_qsfp1_txp[4] = 1'b0;
            assign cp_qsfp1_txn[4] = 1'b1;

            assign qsfp1_fatal_alarm[4] = 1'b0;
            assign qsfp1_corr_alarm[4] = 1'b0;

            assign o_sys_reset1[3] = 1'b0;
            assign qpll1_reset[3] = 1'b0;

            assign o_stat_chan_up1[3] = 1'b0;
            assign o_stat_lane_up1[3] = 1'b0;
            assign o_stat_gt_pwrgd1[3] = 1'b0;

            assign o_user_clk1[3] = 1'b0;

            assign o_s1_axi_tx_tready[3] = 'b0;
            assign o_m1_axi_rx_tdata[255:192] = 'b0;
            assign o_m1_axi_rx_tkeep[31:24] = 'b0;
            assign o_m1_axi_rx_tlast[3] = 'b0;
            assign o_m1_axi_rx_tvalid[3] = 'b0;
         end
      end

   end endgenerate

// leda XV2_1002 on Internally generated asynchronous reset detected (block level). 
// leda XV4_1002 on Internally generated asynchronous reset detected (block level).
// leda XV2P_1002 on Internally generated asynchronous reset detected (block level). 
// leda XV2P_1003 on Multiple asynchronous resets detected in this unit/module.
// leda XV2_1003 on Multiple asynchronous resets detected in this unit/module.
// leda XV4_1003 on Multiple asynchronous resets detected in this unit/module.
   
endmodule



module userio_reset_logic #()
(
   input        i_user_clk,
   input        i_init_clk,
   input        i_reset,
   input        i_sys_reset,
   input        i_gt_reset,
   output       o_sys_reset,
   output       o_gt_reset
);

   wire         sync_reset_reg;
   reg [3:0]    sync_reset_capt;
   
   sync2_1 sync_reset ( .clk(i_user_clk), .d(i_reset), .q(sync_reset_reg) );
   always @ (posedge i_user_clk) begin
      sync_reset_capt <= {sync_reset_capt[2:0], sync_reset_reg};
   end

   assign o_sys_reset = &sync_reset_capt;


   wire         sync_gt_reset_reg;
   reg [3:0]    sync_gt_reset_capt;
   
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
// verilog-library-directories:("." "../../coregen/vu7p/ip_user_files/ip/aurora_64b66b_25p4G") 
// End:
