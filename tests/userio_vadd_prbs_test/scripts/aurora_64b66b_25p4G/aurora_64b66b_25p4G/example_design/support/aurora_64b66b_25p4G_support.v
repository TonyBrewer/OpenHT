 ///////////////////////////////////////////////////////////////////////////////
 //
 // Project:  Aurora 64B/66B
 // Company:  Xilinx
 //
 //
 //
 // (c) Copyright 2008 - 2014 Xilinx, Inc. All rights reserved.
 //
 // This file contains confidential and proprietary information
 // of Xilinx, Inc. and is protected under U.S. and
 // international copyright and other intellectual property
 // laws.
 //
 // DISCLAIMER
 // This disclaimer is not a license and does not grant any
 // rights to the materials distributed herewith. Except as
 // otherwise provided in a valid license issued to you by
 // Xilinx, and to the maximum extent permitted by applicable
 // law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
 // WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
 // AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
 // BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
 // INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
 // (2) Xilinx shall not be liable (whether in contract or tort,
 // including negligence, or under any other theory of
 // liability) for any loss or damage of any kind or nature
 // related to, arising under or in connection with these
 // materials, including for any direct, or any indirect,
 // special, incidental, or consequential loss or damage
 // (including loss of data, profits, goodwill, or any type of
 // loss or damage suffered as a result of any action brought
 // by a third party) even if such damage or loss was
 // reasonably foreseeable or Xilinx had been advised of the
 // possibility of the same.
 //
 // CRITICAL APPLICATIONS
 // Xilinx products are not designed or intended to be fail-
 // safe, or for use in any application requiring fail-safe
 // performance, such as life-support or safety devices or
 // systems, Class III medical devices, nuclear facilities,
 // applications related to the deployment of airbags, or any
 // other applications that could lead to death, personal
 // injury, or severe property or environmental damage
 // (individually and collectively, "Critical
 // Applications"). Customer assumes the sole risk and
 // liability of any use of Xilinx products in Critical
 // Applications, subject only to applicable laws and
 // regulations governing limitations on product liability.
 //
 // THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
 // PART OF THIS FILE AT ALL TIMES.

 //
 ///////////////////////////////////////////////////////////////////////////////
 //
 //
 //  Description: This is 8 series support file for Aurora.
 //
 //
 ///////////////////////////////////////////////////////////////////////////////

// ultrascale aurora_support file

`timescale 1 ps / 1 ps

   (* core_generation_info = "aurora_64b66b_25p4G,aurora_64b66b_v11_2_2,{c_aurora_lanes=1,c_column_used=left,c_gt_clock_1=GTYQ0,c_gt_clock_2=None,c_gt_loc_1=1,c_gt_loc_10=X,c_gt_loc_11=X,c_gt_loc_12=X,c_gt_loc_13=X,c_gt_loc_14=X,c_gt_loc_15=X,c_gt_loc_16=X,c_gt_loc_17=X,c_gt_loc_18=X,c_gt_loc_19=X,c_gt_loc_2=X,c_gt_loc_20=X,c_gt_loc_21=X,c_gt_loc_22=X,c_gt_loc_23=X,c_gt_loc_24=X,c_gt_loc_25=X,c_gt_loc_26=X,c_gt_loc_27=X,c_gt_loc_28=X,c_gt_loc_29=X,c_gt_loc_3=X,c_gt_loc_30=X,c_gt_loc_31=X,c_gt_loc_32=X,c_gt_loc_33=X,c_gt_loc_34=X,c_gt_loc_35=X,c_gt_loc_36=X,c_gt_loc_37=X,c_gt_loc_38=X,c_gt_loc_39=X,c_gt_loc_4=X,c_gt_loc_40=X,c_gt_loc_41=X,c_gt_loc_42=X,c_gt_loc_43=X,c_gt_loc_44=X,c_gt_loc_45=X,c_gt_loc_46=X,c_gt_loc_47=X,c_gt_loc_48=X,c_gt_loc_5=X,c_gt_loc_6=X,c_gt_loc_7=X,c_gt_loc_8=X,c_gt_loc_9=X,c_lane_width=4,c_line_rate=25.4,c_gt_type=GTYE4,c_qpll=true,c_nfc=false,c_nfc_mode=IMM,c_refclk_frequency=100.0,c_simplex=false,c_simplex_mode=TX,c_stream=false,c_ufc=false,c_user_k=false,flow_mode=None,interface_mode=Framing,dataflow_config=Duplex}" *)
(* DowngradeIPIdentifiedWarnings="yes" *)
 module aurora_64b66b_25p4G_support
  (
       //-----------------------------------------------------------------------
       // TX AXI Interface
       input  [0:63]     s_axi_tx_tdata,
       input  [0:7]      s_axi_tx_tkeep,
       input             s_axi_tx_tlast,
       input             s_axi_tx_tvalid,
       output            s_axi_tx_tready,
       //-----------------------------------------------------------------------
       // RX AXI Interface
       output [0:63]     m_axi_rx_tdata,
       output [0:7]      m_axi_rx_tkeep,
       output            m_axi_rx_tlast,
       output            m_axi_rx_tvalid,
       //-----------------------------------------------------------------------
       //-----------------------------------------------------------------------
       //-----------------------------------------------------------------------
       //-----------------------------------------------------------------------
       // GTX Serial I/O
       input              rxp,
       input              rxn,
       output             txp,
       output             txn,
       // Error Detection Interface
       output             hard_err,
       output             soft_err,
       // Status
       output             channel_up,
       output             lane_up,
       //-----------------------------------------------------------------------
       //-----------------------------------------------------------------------
       // System Interface
       output              user_clk_out,
       output              sync_clk_out,
       //-----------------------------------------------------------------------
       input              reset_pb, 
       input              gt_rxcdrovrden_in,
       input              power_down,
       input   [2:0]      loopback,
       input              pma_init,

       //-------------------- AXI4-Lite Interface -------------------------------
       //-------------------- Write Address Channel --------------------------
       input  [31:0]   s_axi_awaddr,
       input  [31:0]   s_axi_araddr,
       input  [31:0]   s_axi_wdata,
       input  [3:0]   s_axi_wstrb,
       input           s_axi_awvalid,
       input           s_axi_rready,
       output  [31:0]  s_axi_rdata,
       output          s_axi_awready,
       output          s_axi_wready,
       output          s_axi_bvalid,
       output  [1:0]   s_axi_bresp,
       output  [1:0]   s_axi_rresp,
       input           s_axi_bready,
       output          s_axi_arready,
       output          s_axi_rvalid,
       input           s_axi_arvalid,
       input           s_axi_wvalid,

       //-----------------------------------------------------------------------
       input              init_clk,
       output              link_reset_out,
       output              gt_pll_lock,
       output              sys_reset_out,


       // GTX Reference Clock Interface
 
       input              gt_refclk1_p,
       input              gt_refclk1_n,

       //-----------------------------------------------------------------------

       output bufg_gt_clr_out,
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
       output                 mmcm_not_locked_out,
       output              tx_out_clk

 );
//********************************Wire Declarations**********************************

     //System Interface
       //wire                 mmcm_not_locked_out ;
       wire                 powerdown_i ;

       wire                  pma_init_i;
       wire                  pma_init_sync;

     // clock
       //(* KEEP = "TRUE" *) wire               user_clk_out;
       //(* KEEP = "TRUE" *) wire               sync_clk_out;


       wire               drp_clk_i;
       wire    [9:0] drpaddr_in_i;
       wire    [15:0]     drpdi_in_i;
       wire    [15:0]     drpdo_out_i;
       wire               drprdy_out_i;
       wire               drpen_in_i;
       wire               drpwe_in_i;


       wire    [31:0]     s_axi_awaddr_i;
       wire    [31:0]     s_axi_araddr_i;
       wire    [31:0]     s_axi_wdata_i;
       wire    [31:0]     s_axi_rdata_i;
       wire    [3:0]     s_axi_wstrb_i;
       wire               s_axi_awvalid_i;
       wire               s_axi_arvalid_i;
       wire               s_axi_wvalid_i;
       wire               s_axi_rvalid_i;
       wire               s_axi_bvalid_i;
       wire    [1:0]      s_axi_bresp_i;
       wire    [1:0]      s_axi_rresp_i;
       wire               s_axi_bready_i;
       wire               s_axi_awready_i;
       wire               s_axi_arready_i;
       wire               s_axi_wready_i;
       wire               s_axi_rready_i;
       wire               link_reset_i;
       wire               sysreset_from_vio_i;
       wire               gtreset_from_vio_i;
       wire               rx_cdrovrden_i;
       wire               gt_reset_i;
       wire               gt_reset_i_tmp;



//---{

//---}
    wire                     refclk1_in;
    wire                     sysreset_from_support;
     wire                    tied_to_ground_i;
     wire    [280:0]          tied_to_ground_vec_i;
     wire                      tied_to_vcc_i;
     assign tied_to_ground_i             = 1'b0;
     assign tied_to_ground_vec_i         = 281'h0;
     assign tied_to_vcc_i                = 1'b1;
//------------------------------------------------------------------------------


 //*********************************Main Body of Code**********************************
     // System Interface
     assign  power_down_i      =   1'b0;
    // AXI4 Lite Interface
     assign  s_axi_awaddr_i    =  32'h0;
     assign  s_axi_wdata_i     =  16'h0;
     assign  s_axi_awvalid_i   =  1'b0;
     assign  s_axi_wvalid_i    =  1'b0;
     assign  s_axi_wstrb_i    =  'd0;
     assign  s_axi_arvalid_i   =  1'b0;
     assign  s_axi_rvalid_i    =  1'b0;


//___________________________Module Instantiations______________________________

//------------------------------------------------------------------------------
// GT common is present only when QPLL is used (i.e. speed is more than 6.5g)
//------------------------------------------------------------------------------
//                      ____________
//    qpll0/1_refclk   |            | gt_qpllclk_quad1_i    (upto 4 based upon no. of Quads)
//  ------------------>|            |------------------>
//                     |            | gt_qpllrefclk_quad1_i (upto 4 based upon no. of Quads)
//      qpll0/1_reset  |  GT Common |------------------>
//  ------------------>|            | gt_qplllock_quad1_i        --"--
//                     |            |------------------>
//  qpll0/1_lock_detclk|            | gt_qpllrefclklost_quad1_i  --"--
//  ------------------>|            |------------------>
//                     |            |
//                     |____________|
//
//
//
//------------------------------------------------------------------------------
  // add physical instance of GT ultrascale common module here
    //--------------------------------------------------------------------------
    // no. of channels are max. 4, single instance of GT Common
    //----------------------------------------------------------------------
        wire qpll0_lock_quad1_out;
        wire qpll0_refclklost1_out;
        wire gt_qpllclk_quad1_i;
        wire gt_qpllrefclk_quad1_i;
        wire gt_qpllrefclklost_quad1_i;
        wire gt_qplllock_quad1_i;

        aurora_64b66b_25p4G_gt_common_wrapper ultrascale_gt_common_1
        (
         .qpll0_refclk                (refclk1_in),                   // input
         .qpll0_reset                 (gt_to_common_qpllreset_i),     // input
       //.qpll0_lock_detclk           (init_clk_out),                 // input
	 .qpll0_lock_detclk           (init_clk),                     // input

         .qpll0_lock                  (qpll0_lock_quad1_out),         // output
         .qpll0_outclk                (gt_qpllclk_quad1_i),           // output
         .qpll0_outrefclk             (gt_qpllrefclk_quad1_i),        // output
         .qpll0_refclklost            (qpll0_refclklost1_out)
        );

       assign gt_qplllock_quad1_i       = qpll0_lock_quad1_out;
       assign gt_qpllrefclklost_quad1_i = qpll0_refclklost1_out;

    //----------------------------------------------------------------------
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------


//------------------------------------------------------------------------------

 //--- Instance of GT Ultrascale differential buffer ---------//
 

 
  IBUFDS_GTE4 IBUFDS_GTE4_refclk1 
  (
    .I     (gt_refclk1_p),
    .IB    (gt_refclk1_n),
    .CEB   (1'b0),
    .O     (refclk1_in),
    .ODIV2 ()
  );



     //(
     //    .CLK_LOCKED    (gt_pll_lock),
     // );

//------------------------------------------------------------------------------
//                      ____________
//                     |            | 
//                     |            |
//                     |            | USER_CLK
//                     |  clocking  |------------------>
//     CLK             |   module   | SYNC_CLK
//  ------------------>|            |------------------>
//    CLK_LOCKED       |            | QPLL_NOT_LOCKED
//  ------------------>|            |------------------>
//                     |            |
//                     |____________|
//
//
//
//------------------------------------------------------------------------------

     // Instantiate a clock module for clock division.
     aurora_64b66b_25p4G_CLOCK_MODULE clock_module_i
     (
        // .INIT_CLK_P                    (init_clk_p),
        // .INIT_CLK_N                    (init_clk_n),
       //.INIT_CLK_IN                   (INIT_CLK_IN)   ,   // i/p 
         .CLK                           (tx_out_clk)    ,   // i/p // this is output from GT module txoutclk_out lane 0
         .CLK_LOCKED                    (bufg_gt_clr_out)   ,   // i/p // connect to bufg_gt_clr_out in GT module

         //.INIT_CLK_O                    (init_clk_out)    ,   // o/p
         .USER_CLK                      (user_clk_out)    ,   // o/p // this is sync_clk/2 (use as userclk2_clk)
         .SYNC_CLK                      (sync_clk_out)    ,   // o/p // this is sync clk   (use as userclk_clk)
         .MMCM_NOT_LOCKED    (mmcm_not_locked_out) // o/p // connect to gtwiz_userclk_tx_active_out
     );

  //  outputs

  //assign init_clk_out          =  init_clk_out;
  //assign user_clk_out          =  user_clk_out;
  //assign sync_clk_out          =  sync_clk_out;
  //assign mmcm_not_locked_out   =  mmcm_not_locked_out;
  assign tx_lock               =  gt_pll_lock;

//------------------------------------------------------------------------------
//                      ____________
//    RESET            |            | SYSTEM_RESET
//  ------------------>|            |------------------>
//    USER_CLK         |            | GT_RESET_OUT
//  ------------------>|   Reset    |------------------>
//     INIT_CLK        |   module   |
//  ------------------>|            |
//    GT_RESET_IN      |            |
//  ------------------>|            |
//                     |____________|
//
//------------------------------------------------------------------------------
      wire sysreset_to_core_sync;

       assign sysreset_to_core_sync = reset_pb;

       assign pma_init_sync = pma_init;

     // Instantiate reset module to generate system reset
     aurora_64b66b_25p4G_SUPPORT_RESET_LOGIC support_reset_logic_i
     (
         .RESET(sysreset_to_core_sync),
         .USER_CLK              (user_clk_out),               // input
         .INIT_CLK              (init_clk),               // input
         .GT_RESET_IN           (pma_init_sync),                 // input

         .SYSTEM_RESET          (sysreset_from_support),
         .GT_RESET_OUT          (pma_init_i)                // output
     );

//----- Instance of _xci or the component declaration in non-shared mode -----[
aurora_64b66b_25p4G  aurora_64b66b_25p4G_i
     (
         // TX AXI4-S Interface
         .s_axi_tx_tdata              (s_axi_tx_tdata),

         .s_axi_tx_tlast              (s_axi_tx_tlast),
         .s_axi_tx_tkeep              (s_axi_tx_tkeep),

         .s_axi_tx_tvalid             (s_axi_tx_tvalid),
         .s_axi_tx_tready             (s_axi_tx_tready),

         // RX AXI4-S Interface
         .m_axi_rx_tdata                (m_axi_rx_tdata),
         .m_axi_rx_tlast                (m_axi_rx_tlast),
         .m_axi_rx_tkeep                (m_axi_rx_tkeep),
         .m_axi_rx_tvalid               (m_axi_rx_tvalid),





         // GTX Serial I/O
         .rxp                   (rxp),
         .rxn                   (rxn),
         .txp                   (txp),
         .txn                   (txn),

         //GTX Reference Clock Interface
         .refclk1_in            (refclk1_in),
         .hard_err              (hard_err),
         .soft_err              (soft_err),

         // Status
         .channel_up            (channel_up),
         .lane_up               (lane_up),


         // System Interface
         .mmcm_not_locked       (!mmcm_not_locked_out),
         .user_clk              (user_clk_out),

         .sync_clk              (sync_clk_out),

         .reset_pb(sysreset_from_support),
         .gt_rxcdrovrden_in     (gt_rxcdrovrden_in),
         .power_down            (power_down),

         .loopback              (loopback),

         .pma_init              (pma_init_i),
         .gt_pll_lock           (gt_pll_lock),
//---{

        /// Assumption: GT common is always out of Ultrascale GT for qpll0 based cases
        // connect the QPLL reference clock from the GT Common output
        .gt_qpllclk_quad1_in                     (gt_qpllclk_quad1_i    ),//(gtrefclk00_in_0),
        .gt_qpllrefclk_quad1_in                  (gt_qpllrefclk_quad1_i ),
        .gt_qplllock_quad1_in                    (gt_qplllock_quad1_i   ),
        .gt_qpllrefclklost_quad1                 (gt_qpllrefclklost_quad1_i),





         .gt_to_common_qpllreset_out                    (gt_to_common_qpllreset_i    ),
         //.gt_qplllock_in             (gt_qplllock_i        ),
         //.gt_qpllrefclklost_in       (gt_qpllrefclklost_i  ),
//---}

     // ---------- AXI4-Lite input signals ---------------
         .s_axi_awaddr        (s_axi_awaddr),
         .s_axi_awvalid       (s_axi_awvalid),
         .s_axi_awready       (s_axi_awready),
         .s_axi_wdata         (s_axi_wdata),
         .s_axi_wstrb         (s_axi_wstrb),
         .s_axi_wvalid        (s_axi_wvalid),
         .s_axi_wready        (s_axi_wready),
         .s_axi_bvalid        (s_axi_bvalid),
         .s_axi_bready        (s_axi_bready),
       .s_axi_bresp        (s_axi_bresp),
         .s_axi_araddr        (s_axi_araddr),
         .s_axi_arvalid       (s_axi_arvalid),
         .s_axi_arready       (s_axi_arready),
         .s_axi_rdata         (s_axi_rdata),
         .s_axi_rvalid        (s_axi_rvalid),
         .s_axi_rready        (s_axi_rready),
       .s_axi_rresp        (s_axi_rresp),


         //.init_clk                              (init_clk_out),
	 .init_clk                              (init_clk),
         .link_reset_out                        (link_reset_out),


       .bufg_gt_clr_out                   (bufg_gt_clr_out),// connect to clk locked port of clock module

         .sys_reset_out                   (sys_reset_out),
         .tx_out_clk                      (tx_out_clk)
     );
//----- Instance of _xci -----]



 endmodule
