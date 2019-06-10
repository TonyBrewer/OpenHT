`timescale 1 ns / 1 ps

module user_io_bbox #
(
   parameter NUM_AUR_LINKS   = 8,
   parameter NUM_UIO_PORTS   = 10,
   parameter UIO_PORTS_WIDTH = 128
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
   
   input [4:1]          qsfp0_cp_rxp,
   input [4:1]          qsfp0_cp_rxn,
   output [4:1]         cp_qsfp0_txp,
   output [4:1]         cp_qsfp0_txn,
                 
   input [4:1]          qsfp1_cp_rxp,
   input [4:1]          qsfp1_cp_rxn,
   output [4:1]         cp_qsfp1_txp,
   output [4:1]         cp_qsfp1_txn,

   // optional alarm indications 
   output [4:1]         qsfp0_fatal_alarm,
   output [4:1]         qsfp0_corr_alarm,

   output [4:1]         qsfp1_fatal_alarm,
   output [4:1]         qsfp1_corr_alarm,

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

   
   // Aurora IP-driven Clk/Reset
   wire [NUM_AUR_LINKS*1-1:0] user_clk;
   wire [NUM_AUR_LINKS*1-1:0] sys_reset;
   
   // AXI
   wire [NUM_AUR_LINKS*64-1:0] m_axi_rx_tdata;
   wire [NUM_AUR_LINKS*8-1:0]  m_axi_rx_tkeep;
   wire [NUM_AUR_LINKS*1-1:0]  m_axi_rx_tlast;
   wire [NUM_AUR_LINKS*1-1:0]  m_axi_rx_tvalid;
   wire [NUM_AUR_LINKS*64-1:0] s_axi_tx_tdata;
   wire [NUM_AUR_LINKS*8-1:0]  s_axi_tx_tkeep;
   wire [NUM_AUR_LINKS*1-1:0]  s_axi_tx_tlast;
   wire [NUM_AUR_LINKS*1-1:0]  s_axi_tx_tready;
   wire [NUM_AUR_LINKS*1-1:0]  s_axi_tx_tvalid;

   // Status Lines
   wire [7:0] stat_chan_up;
   wire [7:0] stat_gt_pwrgd;
   wire [7:0] stat_lane_up;

   // User Port to AXI Wrapper
   user_io_axi_wrapper #(.NUM_AUR_LINKS(NUM_AUR_LINKS),
                         .NUM_UIO_PORTS(NUM_UIO_PORTS-1),
                         .UIO_PORTS_WIDTH(UIO_PORTS_WIDTH))
   user_io_axi_wrapper (/*AUTOINST*/
                        // Outputs
                        .o_s_axi_tx_tkeep(s_axi_tx_tkeep[NUM_AUR_LINKS*8-1:0]), // Templated
                        .o_s_axi_tx_tdata(s_axi_tx_tdata[NUM_AUR_LINKS*64-1:0]), // Templated
                        .o_s_axi_tx_tlast(s_axi_tx_tlast[NUM_AUR_LINKS*1-1:0]), // Templated
                        .o_s_axi_tx_tvalid(s_axi_tx_tvalid[NUM_AUR_LINKS*1-1:0]), // Templated
                        .uio_rq_afull   (uio_rq_afull[(NUM_UIO_PORTS-1)*1-1:0]), // Templated
                        .uio_rs_vld     (uio_rs_vld[(NUM_UIO_PORTS-1)*1-1:0]), // Templated
                        .uio_rs_data    (uio_rs_data[(NUM_UIO_PORTS-1)*UIO_PORTS_WIDTH-1:0]), // Templated
                        // Inputs
                        .clk            (user_clk[NUM_AUR_LINKS*1-1:0]), // Templated
                        .clk_per        (clk_per),
                        .reset          (sys_reset[NUM_AUR_LINKS*1-1:0]), // Templated
                        .reset_per      (reset_per),
                        .i_s_axi_tx_tready(s_axi_tx_tready[NUM_AUR_LINKS*1-1:0]), // Templated
                        .i_m_axi_rx_tkeep(m_axi_rx_tkeep[NUM_AUR_LINKS*8-1:0]), // Templated
                        .i_m_axi_rx_tdata(m_axi_rx_tdata[NUM_AUR_LINKS*64-1:0]), // Templated
                        .i_m_axi_rx_tlast(m_axi_rx_tlast[NUM_AUR_LINKS*1-1:0]), // Templated
                        .i_m_axi_rx_tvalid(m_axi_rx_tvalid[NUM_AUR_LINKS*1-1:0]), // Templated
                        .stat_chan_up   (stat_chan_up[NUM_AUR_LINKS*1-1:0]),
                        .uio_rq_vld     (uio_rq_vld[(NUM_UIO_PORTS-1)*1-1:0]), // Templated
                        .uio_rq_data    (uio_rq_data[(NUM_UIO_PORTS-1)*UIO_PORTS_WIDTH-1:0]), // Templated
                        .uio_rs_afull   (uio_rs_afull[(NUM_UIO_PORTS-1)*1-1:0])); // Templated

   
   // Mapped as follows:
   //   qsfp[7:0] = {qsfp1[4:1], qsfp0[4:1]}

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

   wire [7:0]   qsfp_refclk;

   assign qsfp_refclk = {qsfp1_refclk_gt, qsfp1_refclk_gt, qsfp1_refclk_gt, qsfp1_refclk_gt,
                         qsfp0_refclk_gt, qsfp0_refclk_gt, qsfp0_refclk_gt, qsfp0_refclk_gt};
                         
   
   // Mapped as follows:
   //   qsfp[7:0] = {qsfp1[4:1], qsfp0[4:1]}

   // Links/Alarms
   wire [7:0]   qsfp_cp_rxp;
   wire [7:0]   qsfp_cp_rxn;
   wire [7:0]   cp_qsfp_txp;
   wire [7:0]   cp_qsfp_txn;

   wire [7:0]   qsfp_fatal_alarm;
   wire [7:0]   qsfp_corr_alarm;

   assign qsfp_cp_rxp = {qsfp1_cp_rxp[4:1], qsfp0_cp_rxp[4:1]};
   assign qsfp_cp_rxn = {qsfp1_cp_rxn[4:1], qsfp0_cp_rxn[4:1]};
   assign cp_qsfp0_txp[4:1] = cp_qsfp_txp[3:0];
   assign cp_qsfp1_txp[4:1] = cp_qsfp_txp[7:4];
   assign cp_qsfp0_txn[4:1] = cp_qsfp_txn[3:0];
   assign cp_qsfp1_txn[4:1] = cp_qsfp_txn[7:4];
   
   assign qsfp0_fatal_alarm[4:1] = qsfp_fatal_alarm[3:0];
   assign qsfp1_fatal_alarm[4:1] = qsfp_fatal_alarm[7:4];
   assign qsfp0_corr_alarm[4:1]  = qsfp_corr_alarm[3:0];
   assign qsfp1_corr_alarm[4:1]  = qsfp_corr_alarm[7:4];
   
   // Aurora IP Wrapper
   user_io_xcvr_wrapper #(.NUM_AUR_LINKS(NUM_AUR_LINKS))
   user_io_xcvr_wrapper (/*AUTOINST*/
                         // Outputs
                         .o_sys_reset           (sys_reset[NUM_AUR_LINKS*1-1:0]), // Templated
                         .o_user_clk            (user_clk[NUM_AUR_LINKS*1-1:0]), // Templated
                         .cp_qsfp_txp           (cp_qsfp_txp[7:0]), // Templated
                         .cp_qsfp_txn           (cp_qsfp_txn[7:0]), // Templated
                         .o_s_axi_tx_tready     (s_axi_tx_tready[NUM_AUR_LINKS*1-1:0]), // Templated
                         .o_m_axi_rx_tkeep      (m_axi_rx_tkeep[NUM_AUR_LINKS*8-1:0]), // Templated
                         .o_m_axi_rx_tdata      (m_axi_rx_tdata[NUM_AUR_LINKS*64-1:0]), // Templated
                         .o_m_axi_rx_tlast      (m_axi_rx_tlast[NUM_AUR_LINKS*1-1:0]), // Templated
                         .o_m_axi_rx_tvalid     (m_axi_rx_tvalid[NUM_AUR_LINKS*1-1:0]), // Templated
                         .qsfp_fatal_alarm      (qsfp_fatal_alarm[7:0]),
                         .qsfp_corr_alarm       (qsfp_corr_alarm[7:0]),
                         .o_stat_chan_up        (stat_chan_up[7:0]), // Templated
                         .o_stat_lane_up        (stat_lane_up[7:0]), // Templated
                         .o_stat_gt_pwrgd       (stat_gt_pwrgd[7:0]), // Templated
                         // Inputs
                         .i_init_clk            (init_clk_bufg),         // Templated
                         .i_reset               (reset_per),     // Templated
                         .i_gt_reset            (reset_per),     // Templated
                         .qsfp_refclk           (qsfp_refclk[7:0]), // Templated
                         .qsfp_cp_rxp           (qsfp_cp_rxp[7:0]), // Templated
                         .qsfp_cp_rxn           (qsfp_cp_rxn[7:0]), // Templated
                         .i_s_axi_tx_tkeep      (s_axi_tx_tkeep[NUM_AUR_LINKS*8-1:0]), // Templated
                         .i_s_axi_tx_tdata      (s_axi_tx_tdata[NUM_AUR_LINKS*64-1:0]), // Templated
                         .i_s_axi_tx_tlast      (s_axi_tx_tlast[NUM_AUR_LINKS*1-1:0]), // Templated
                         .i_s_axi_tx_tvalid     (s_axi_tx_tvalid[NUM_AUR_LINKS*1-1:0])); // Templated
   

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
           c_rd_data = {48'b0, stat_chan_up, stat_lane_up};
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

   sync2 #(.WIDTH(128))
   status_rs_p9 (
                 .clk(clk_per),
                 .d({96'd0, qsfp_fatal_alarm, qsfp_corr_alarm, stat_chan_up, stat_lane_up}),
                 .q(uio_rs_data[(NUM_UIO_PORTS-1)*UIO_PORTS_WIDTH-1:(NUM_UIO_PORTS-2)*UIO_PORTS_WIDTH])
                 );
   assign uio_rs_vld[NUM_UIO_PORTS-2] = ~reset_per;

   sync2 #(.WIDTH(128))
   status_rs_p10 (
                  .clk(clk_per),
                  .d({96'd0, qsfp_fatal_alarm, qsfp_corr_alarm, stat_chan_up, stat_lane_up}),
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

/* user_io_xcvr_wrapper AUTO_TEMPLATE (
                         // Outputs
                         .o_sys_reset           (sys_reset[]),
                         .o_user_clk            (user_clk[]),
                         .cp_qsfp_txp           (cp_qsfp_txp[]),
                         .cp_qsfp_txn           (cp_qsfp_txn[]),
                         .o_gt_to_qpll_reset    (qpll_reset[]),
                         .o_s_axi_\(.*\)        (s_axi_\1[]),
                         .o_m_axi_\(.*\)        (m_axi_\1[]),
                         .o_stat_\(.*\)         (stat_\1[]),
                         // Inputs
                         .i_init_clk            (clk_per),
                         .i_reset               (reset_per),
                         .i_gt_reset            (reset_per),
                         .qsfp_refclk           (qsfp_refclk[]),
                         .qsfp_cp_rxp           (qsfp_cp_rxp[]),
                         .qsfp_cp_rxn           (qsfp_cp_rxn[]),
                         .i_gt_qpllclk          (qpll_outclk[]),
                         .i_gt_qpllrefclk       (qpll_outrefclk[]),
                         .i_qpll_lock           (qpll_lock[]),
                         .i_qpll_refclklost     (qpll_refclklost[]),
                         .i_s_axi_\(.*\)        (s_axi_\1[]),
   ); */

/* user_io_axi_wrapper AUTO_TEMPLATE (
                         .uio_rq_afull  (uio_rq_afull[(NUM_UIO_PORTS-1)*1-1:0]),
                         .uio_rs_vld    (uio_rs_vld[(NUM_UIO_PORTS-1)*1-1:0]),
                         .uio_rs_data   (uio_rs_data[(NUM_UIO_PORTS-1)*UIO_PORTS_WIDTH-1:0]),
                         .uio_rq_vld    (uio_rq_vld[(NUM_UIO_PORTS-1)*1-1:0]),
                         .uio_rq_data   (uio_rq_data[(NUM_UIO_PORTS-1)*UIO_PORTS_WIDTH-1:0]),
                         .uio_rs_afull  (uio_rs_afull[(NUM_UIO_PORTS-1)*1-1:0]),
                         // Outputs
                         .o_s_axi_\(.*\)        (s_axi_\1[]),
                         .i_s_axi_\(.*\)        (s_axi_\1[]),
                         // Inputs
                         .clk                   (user_clk[]),
                         .reset                 (sys_reset[]),
                         .i_m_axi_\(.*\)        (m_axi_\1[]),
   ); */
   
endmodule
