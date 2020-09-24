`timescale 1 ns / 1 ps

module user_io_bbox #
(
   parameter NUM_UIO_PORTS   = 9,
   parameter UIO_PORTS_WIDTH = 128,
   parameter QSFP0_WIDTH = 4,
   parameter QSFP1_WIDTH = 4
)(
   input                clk_per,
   input                reset_per,
   
   input                qsfp0_refclk_p,
   input                qsfp0_refclk_n,
   input                qsfp1_refclk_p,
   input                qsfp1_refclk_n,

   // QSFP0 I2C signals
   inout                qsfp0_sda,
   inout                qsfp0_scl,
   // QSFP1 I2C signals
   inout                qsfp1_sda,
   inout                qsfp1_scl,
   
   input [QSFP0_WIDTH:1] qsfp0_cp_rxp,
   input [QSFP0_WIDTH:1] qsfp0_cp_rxn,
   output [QSFP0_WIDTH:1] cp_qsfp0_txp,
   output [QSFP0_WIDTH:1] cp_qsfp0_txn,
                 
   input [QSFP1_WIDTH:1] qsfp1_cp_rxp,
   input [QSFP1_WIDTH:1] qsfp1_cp_rxn,
   output [QSFP1_WIDTH:1] cp_qsfp1_txp,
   output [QSFP1_WIDTH:1] cp_qsfp1_txn,

   // optional alarm indications 
   output [QSFP0_WIDTH:1] qsfp0_fatal_alarm,
   output [QSFP0_WIDTH:1] qsfp0_corr_alarm,

   output [QSFP1_WIDTH:1] qsfp1_fatal_alarm,
   output [QSFP1_WIDTH:1] qsfp1_corr_alarm,

   // csr ports
   input  [15:0]        i_csr_addr,
   input  [63:0]        i_csr_data,
   input                i_csr_wr_vld,
   input                i_csr_rd_vld,

   output [63:0]        o_csr_data,
   output               o_csr_rd_ack,

   // user-facing ports
   input  [NUM_UIO_PORTS*1-1:0]                 uio_rq_vld,
   input  [NUM_UIO_PORTS*UIO_PORTS_WIDTH-1:0]   uio_rq_data,
   output [NUM_UIO_PORTS*1-1:0]                 uio_rq_afull,

   output [NUM_UIO_PORTS*1-1:0]                 uio_rs_vld,
   output [NUM_UIO_PORTS*UIO_PORTS_WIDTH-1:0]   uio_rs_data,
   input  [NUM_UIO_PORTS*1-1:0]                 uio_rs_afull
);

   // Should be nothing here
   /*AUTOWIRE*/

   
   // QSFP
   wire [4:1] qsfp0_cp_rxp_int, qsfp1_cp_rxp_int;
   wire [4:1] qsfp0_cp_rxn_int, qsfp1_cp_rxn_int;
   wire [4:1] cp_qsfp0_txp_int, cp_qsfp1_txp_int;
   wire [4:1] cp_qsfp0_txn_int, cp_qsfp1_txn_int;
   wire [4:1] qsfp0_fatal_alarm_int, qsfp0_corr_alarm_int;
   wire [4:1] qsfp1_fatal_alarm_int, qsfp1_corr_alarm_int;
   
   // Aurora IP-driven Clk/Reset
   wire [3:0] user_clk0, user_clk1;
   wire [3:0] sys_reset0, sys_reset1;
   
   // AXI
   wire [4*64-1:0] m0_axi_rx_tdata, m1_axi_rx_tdata;
   wire [4*8-1:0]  m0_axi_rx_tkeep, m1_axi_rx_tkeep;
   wire [4*1-1:0]  m0_axi_rx_tlast, m1_axi_rx_tlast;
   wire [4*1-1:0]  m0_axi_rx_tvalid, m1_axi_rx_tvalid;
   wire [4*64-1:0] s0_axi_tx_tdata, s1_axi_tx_tdata;
   wire [4*8-1:0]  s0_axi_tx_tkeep, s1_axi_tx_tkeep;
   wire [4*1-1:0]  s0_axi_tx_tlast, s1_axi_tx_tlast;
   wire [4*1-1:0]  s0_axi_tx_tready, s1_axi_tx_tready;
   wire [4*1-1:0]  s0_axi_tx_tvalid, s1_axi_tx_tvalid;

   // Status Lines
   wire [3:0] stat_chan_up0, stat_chan_up1;
   wire [3:0] stat_gt_pwrgd0, stat_gt_pwrgd1;
   wire [3:0] stat_lane_up0, stat_lane_up1;

   // User Port to AXI Wrapper
   user_io_axi_wrapper #(.QSFP0_WIDTH(QSFP0_WIDTH),
                         .QSFP1_WIDTH(QSFP1_WIDTH),
                         .NUM_UIO_PORTS(NUM_UIO_PORTS-1),
                         .UIO_PORTS_WIDTH(UIO_PORTS_WIDTH))
   user_io_axi_wrapper (// Outputs
                        .o_s0_axi_tx_tkeep  (s0_axi_tx_tkeep[4*8-1:0]),
                        .o_s0_axi_tx_tdata  (s0_axi_tx_tdata[4*64-1:0]),
                        .o_s0_axi_tx_tlast  (s0_axi_tx_tlast[4*1-1:0]),
                        .o_s0_axi_tx_tvalid (s0_axi_tx_tvalid[4*1-1:0]),
                        .o_s1_axi_tx_tkeep  (s1_axi_tx_tkeep[4*8-1:0]),
                        .o_s1_axi_tx_tdata  (s1_axi_tx_tdata[4*64-1:0]),
                        .o_s1_axi_tx_tlast  (s1_axi_tx_tlast[4*1-1:0]),
                        .o_s1_axi_tx_tvalid (s1_axi_tx_tvalid[4*1-1:0]),
                        .uio_rq_afull       (uio_rq_afull[(NUM_UIO_PORTS-1)*1-1:0]),
                        .uio_rs_vld         (uio_rs_vld[(NUM_UIO_PORTS-1)*1-1:0]),
                        .uio_rs_data        (uio_rs_data[(NUM_UIO_PORTS-1)*UIO_PORTS_WIDTH-1:0]),
                        // Inputs
                        .clk0               (user_clk0[3:0]),
                        .clk1               (user_clk1[3:0]),
                        .clk_per            (clk_per),
                        .reset0             (sys_reset0[3:0]),
                        .reset1             (sys_reset1[3:0]),
                        .reset_per          (reset_per),
                        .i_s0_axi_tx_tready (s0_axi_tx_tready[4*1-1:0]),
                        .i_m0_axi_rx_tkeep  (m0_axi_rx_tkeep[4*8-1:0]),
                        .i_m0_axi_rx_tdata  (m0_axi_rx_tdata[4*64-1:0]),
                        .i_m0_axi_rx_tlast  (m0_axi_rx_tlast[4*1-1:0]),
                        .i_m0_axi_rx_tvalid (m0_axi_rx_tvalid[4*1-1:0]),
                        .i_s1_axi_tx_tready (s1_axi_tx_tready[4*1-1:0]),
                        .i_m1_axi_rx_tkeep  (m1_axi_rx_tkeep[4*8-1:0]),
                        .i_m1_axi_rx_tdata  (m1_axi_rx_tdata[4*64-1:0]),
                        .i_m1_axi_rx_tlast  (m1_axi_rx_tlast[4*1-1:0]),
                        .i_m1_axi_rx_tvalid (m1_axi_rx_tvalid[4*1-1:0]),
                        .stat_chan_up0      (stat_chan_up0[3:0]),
                        .stat_chan_up1      (stat_chan_up1[3:0]),
                        .uio_rq_vld         (uio_rq_vld[(NUM_UIO_PORTS-1)*1-1:0]),
                        .uio_rq_data        (uio_rq_data[(NUM_UIO_PORTS-1)*UIO_PORTS_WIDTH-1:0]),
                        .uio_rs_afull       (uio_rs_afull[(NUM_UIO_PORTS-1)*1-1:0]));

   // Clocking
   wire       qsfp0_refclk_gt, qsfp0_refclk;
   IBUFDS_GTE4 qsfp0clkbuf (
      .O(qsfp0_refclk_gt),
      .I(qsfp0_refclk_p),
      .IB(qsfp0_refclk_n),
      .ODIV2(qsfp0_refclk),
      .CEB(1'b0)
   );
   wire qsfp1_refclk_gt, qsfp1_refclk;
   IBUFDS_GTE4 qsfp1clkbuf (
      .O(qsfp1_refclk_gt),
      .I(qsfp1_refclk_p),
      .IB(qsfp1_refclk_n),
      .ODIV2(qsfp1_refclk),
      .CEB(1'b0)
   );
   
   wire init_clk_bufg;
   BUFG_GT bufg_gt_initclk (
                            .CE (1'b1),
                            .CEMASK (1'd0),
                            .CLR (reset_per),
                            .CLRMASK (1'd0),
                            .DIV (3'd0),
                            .I (qsfp0_refclk),
                            .O (init_clk_bufg)
                            );
   
   // Aurora IP Wrapper
   assign qsfp0_cp_rxp_int = (QSFP0_WIDTH == 4) ? qsfp0_cp_rxp[4:1] :
                             (QSFP0_WIDTH == 3) ? {1'h0, qsfp0_cp_rxp[3:1]} :
                             (QSFP0_WIDTH == 2) ? {2'h0, qsfp0_cp_rxp[2:1]} :
                             (QSFP0_WIDTH == 1) ? {3'h0, qsfp0_cp_rxp[1]} :
                             4'h0;
   assign qsfp0_cp_rxn_int = (QSFP0_WIDTH == 4) ? qsfp0_cp_rxn[4:1] :
                             (QSFP0_WIDTH == 3) ? {1'h1, qsfp0_cp_rxn[3:1]} :
                             (QSFP0_WIDTH == 2) ? {2'h3, qsfp0_cp_rxn[2:1]} :
                             (QSFP0_WIDTH == 1) ? {3'h7, qsfp0_cp_rxn[1]} :
                             4'hF;
   assign qsfp1_cp_rxp_int = (QSFP1_WIDTH == 4) ? qsfp1_cp_rxp[4:1] :
                             (QSFP1_WIDTH == 3) ? {1'h0, qsfp1_cp_rxp[3:1]} :
                             (QSFP1_WIDTH == 2) ? {2'h0, qsfp1_cp_rxp[2:1]} :
                             (QSFP1_WIDTH == 1) ? {3'h0, qsfp1_cp_rxp[1]} :
                             4'h0;
   assign qsfp1_cp_rxn_int = (QSFP1_WIDTH == 4) ? qsfp1_cp_rxn[4:1] :
                             (QSFP1_WIDTH == 3) ? {1'h1, qsfp1_cp_rxn[3:1]} :
                             (QSFP1_WIDTH == 2) ? {2'h3, qsfp1_cp_rxn[2:1]} :
                             (QSFP1_WIDTH == 1) ? {3'h7, qsfp1_cp_rxn[1]} :
                             4'hF;
   assign cp_qsfp0_txp = (QSFP0_WIDTH == 0) ? 4'h0 : cp_qsfp0_txp_int[QSFP0_WIDTH:1];
   assign cp_qsfp0_txn = (QSFP0_WIDTH == 0) ? 4'hF : cp_qsfp0_txn_int[QSFP0_WIDTH:1];
   assign cp_qsfp1_txp = (QSFP1_WIDTH == 0) ? 4'h0 : cp_qsfp1_txp_int[QSFP1_WIDTH:1];
   assign cp_qsfp1_txn = (QSFP1_WIDTH == 0) ? 4'hF : cp_qsfp1_txn_int[QSFP1_WIDTH:1];
   assign qsfp0_fatal_alarm = (QSFP0_WIDTH == 0) ? 4'h0 : qsfp0_fatal_alarm_int[QSFP0_WIDTH:1];
   assign qsfp0_corr_alarm  = (QSFP0_WIDTH == 0) ? 4'h0 : qsfp0_corr_alarm_int[QSFP0_WIDTH:1];
   assign qsfp1_fatal_alarm = (QSFP1_WIDTH == 0) ? 4'h0 : qsfp1_fatal_alarm_int[QSFP1_WIDTH:1];
   assign qsfp1_corr_alarm  = (QSFP1_WIDTH == 0) ? 4'h0 : qsfp1_corr_alarm_int[QSFP1_WIDTH:1];
   
   user_io_xcvr_wrapper #(.QSFP0_WIDTH(QSFP0_WIDTH),
                          .QSFP1_WIDTH(QSFP1_WIDTH))
   user_io_xcvr_wrapper (// Outputs
                         .o_sys_reset0          (sys_reset0[3:0]),
                         .o_sys_reset1          (sys_reset1[3:0]),
                         .o_user_clk0           (user_clk0[3:0]),
                         .o_user_clk1           (user_clk1[3:0]),
                         .cp_qsfp0_txp          (cp_qsfp0_txp_int[4:1]),
                         .cp_qsfp0_txn          (cp_qsfp0_txn_int[4:1]),
                         .cp_qsfp1_txp          (cp_qsfp1_txp_int[4:1]),
                         .cp_qsfp1_txn          (cp_qsfp1_txn_int[4:1]),
                         .o_s0_axi_tx_tready    (s0_axi_tx_tready[4*1-1:0]),
                         .o_m0_axi_rx_tkeep     (m0_axi_rx_tkeep[4*8-1:0]),
                         .o_m0_axi_rx_tdata     (m0_axi_rx_tdata[4*64-1:0]),
                         .o_m0_axi_rx_tlast     (m0_axi_rx_tlast[4*1-1:0]),
                         .o_m0_axi_rx_tvalid    (m0_axi_rx_tvalid[4*1-1:0]),
                         .o_s1_axi_tx_tready    (s1_axi_tx_tready[4*1-1:0]),
                         .o_m1_axi_rx_tkeep     (m1_axi_rx_tkeep[4*8-1:0]),
                         .o_m1_axi_rx_tdata     (m1_axi_rx_tdata[4*64-1:0]),
                         .o_m1_axi_rx_tlast     (m1_axi_rx_tlast[4*1-1:0]),
                         .o_m1_axi_rx_tvalid    (m1_axi_rx_tvalid[4*1-1:0]),
                         .qsfp0_fatal_alarm     (qsfp0_fatal_alarm_int[4:1]),
                         .qsfp0_corr_alarm      (qsfp0_corr_alarm_int[4:1]),
                         .qsfp1_fatal_alarm     (qsfp1_fatal_alarm_int[4:1]),
                         .qsfp1_corr_alarm      (qsfp1_corr_alarm_int[4:1]),
                         .o_stat_chan_up0       (stat_chan_up0[3:0]),
                         .o_stat_lane_up0       (stat_lane_up0[3:0]),
                         .o_stat_gt_pwrgd0      (stat_gt_pwrgd0[3:0]),
                         .o_stat_chan_up1       (stat_chan_up1[3:0]),
                         .o_stat_lane_up1       (stat_lane_up1[3:0]),
                         .o_stat_gt_pwrgd1      (stat_gt_pwrgd1[3:0]),
                         // Inputs
                         .i_init_clk            (init_clk_bufg),        
                         .i_reset               (reset_per),    
                         .i_gt_reset            (reset_per),    
                         .qsfp0_refclk          (qsfp0_refclk_gt),
                         .qsfp1_refclk          (qsfp1_refclk_gt),
                         .qsfp0_cp_rxp          (qsfp0_cp_rxp_int[4:1]),
                         .qsfp0_cp_rxn          (qsfp0_cp_rxn_int[4:1]),
                         .qsfp1_cp_rxp          (qsfp1_cp_rxp_int[4:1]),
                         .qsfp1_cp_rxn          (qsfp1_cp_rxn_int[4:1]),
                         .i_s0_axi_tx_tkeep     (s0_axi_tx_tkeep[4*8-1:0]),
                         .i_s0_axi_tx_tdata     (s0_axi_tx_tdata[4*64-1:0]),
                         .i_s0_axi_tx_tlast     (s0_axi_tx_tlast[4*1-1:0]),
                         .i_s0_axi_tx_tvalid    (s0_axi_tx_tvalid[4*1-1:0]),
                         .i_s1_axi_tx_tkeep     (s1_axi_tx_tkeep[4*8-1:0]),
                         .i_s1_axi_tx_tdata     (s1_axi_tx_tdata[4*64-1:0]),
                         .i_s1_axi_tx_tlast     (s1_axi_tx_tlast[4*1-1:0]),
                         .i_s1_axi_tx_tvalid    (s1_axi_tx_tvalid[4*1-1:0]));
   

   // CSR Logic   
   reg          c_rd_ack;
   wire         r_rd_ack;
   reg [63:0]   c_rd_data;
   wire [63:0]  r_rd_data;
   
   reg [63:0]   c_reg_1;
   wire [63:0]  r_reg_1;

   localparam CSR_STAT_REG    = 16'h0000;
   localparam CSR_SCRATCH_REG = 16'h0008;
   
   
   always @(*) begin
      c_rd_ack  = 1'b0;
      c_rd_data = 64'b0;
            
      c_reg_1 = r_reg_1;

      case(i_csr_addr)

        CSR_STAT_REG: begin
           c_rd_ack  = i_csr_rd_vld;
           c_rd_data = {48'b0, stat_chan_up1, stat_chan_up0, stat_lane_up1, stat_lane_up0};
        end

        CSR_SCRATCH_REG: begin
           c_rd_ack  = i_csr_rd_vld;
           c_rd_data = r_reg_1;

           c_reg_1 = i_csr_wr_vld ? i_csr_data : r_reg_1;
        end

        default: begin
           c_rd_ack  = i_csr_rd_vld;
           c_rd_data = 64'hdeadbeefdeadbeef;
        end

      endcase
   end

   dff_aclr #(.WIDTH(1))  dff_bbox_csr_ack  (.clk(clk_per), .clr(reset_per), .d(c_rd_ack),  .q(r_rd_ack));
   dff_aclr #(.WIDTH(64)) dff_bbox_csr_rdat (.clk(clk_per), .clr(reset_per), .d(c_rd_data), .q(r_rd_data));

   dff_aclr #(.WIDTH(64)) dff_bbox_csr_reg1 (.clk(clk_per), .clr(reset_per), .d(c_reg_1), .q(r_reg_1));

   assign o_csr_rd_ack = r_rd_ack;
   assign o_csr_data   = r_rd_data;

   
   // Status Link (back to personality)
   //   This link is unique in that it doesn't use the flow control
   //   pieces of the link, and is intended to be a continuous flow
   //   of information back to the personality.  For this reason,
   //   we're just using a few registers to capture the status
   //   (which should be stable for many clocks)

   sync2 #(.WIDTH(UIO_PORTS_WIDTH))
   status_rs_p9 (
                 .clk(clk_per),
                 .d({{UIO_PORTS_WIDTH-32{1'b0}},
                     (QSFP0_WIDTH == 4) ? qsfp0_fatal_alarm[4:1] : (QSFP0_WIDTH == 0) ? 4'b0 : {{4-QSFP0_WIDTH{1'b0}}, qsfp0_fatal_alarm[QSFP0_WIDTH:1]},
                     (QSFP1_WIDTH == 4) ? qsfp1_fatal_alarm[4:1] : (QSFP1_WIDTH == 0) ? 4'b0 : {{4-QSFP1_WIDTH{1'b0}}, qsfp1_fatal_alarm[QSFP1_WIDTH:1]},
                     (QSFP0_WIDTH == 4) ? qsfp0_corr_alarm[4:1] : (QSFP0_WIDTH == 0) ? 4'b0 : {{4-QSFP0_WIDTH{1'b0}}, qsfp0_corr_alarm[QSFP0_WIDTH:1]},
                     (QSFP1_WIDTH == 4) ? qsfp1_corr_alarm[4:1] : (QSFP1_WIDTH == 0) ? 4'b0 : {{4-QSFP1_WIDTH{1'b0}}, qsfp1_corr_alarm[QSFP1_WIDTH:1]},
                     stat_chan_up1, stat_chan_up0,
                     stat_lane_up1, stat_lane_up0}),
                 .q(uio_rs_data[NUM_UIO_PORTS*UIO_PORTS_WIDTH-1:(NUM_UIO_PORTS-1)*UIO_PORTS_WIDTH])
                 );

   assign uio_rs_vld[NUM_UIO_PORTS-1] = ~reset_per;

   // Tie off unused QSFP I2C ports to pull up I2C nets
   wire [1:0] _qsfp0_nc0_;
   IOBUF qsfp0sdatieoff (
      .IO(qsfp0_sda),
      .I(1'b1),
      .T(1'b0),
      .O(_qsfp0_nc0_[0])
   );
   IOBUF qsfp0scltieoff (
      .IO(qsfp0_scl),
      .I(1'b1),
      .T(1'b0),
      .O(_qsfp0_nc0_[1])
   );
   wire [1:0] _qsfp1_nc0_;
   IOBUF qsfp1sdatieoff (
      .IO(qsfp1_sda),
      .I(1'b1),
      .T(1'b0),
      .O(_qsfp1_nc0_[0])
   );
   IOBUF qsfp1scltieoff (
      .IO(qsfp1_scl),
      .I(1'b1),
      .T(1'b0),
      .O(_qsfp1_nc0_[1])
   );
   
endmodule
