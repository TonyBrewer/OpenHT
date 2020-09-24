`timescale 1 ns / 1 ps

module user_io_axi_wrapper #
(
   parameter QSFP0_WIDTH = 4,
   parameter QSFP1_WIDTH = 4,
   parameter NUM_UIO_PORTS = 8,
   parameter UIO_PORTS_WIDTH = 128
)(
   input [3:0]                                clk0,
   input [3:0]                                clk1,
   input                                      clk_per,
   input [3:0]                                reset0,
   input [3:0]                                reset1,
   input                                      reset_per,

   // AXI Connections (QSFP0)
   output [4*8-1:0]                           o_s0_axi_tx_tkeep,
   output [4*64-1:0]                          o_s0_axi_tx_tdata,
   output [4*1-1:0]                           o_s0_axi_tx_tlast,
   output [4*1-1:0]                           o_s0_axi_tx_tvalid,
   input [4*1-1:0]                            i_s0_axi_tx_tready,

   input [4*8-1:0]                            i_m0_axi_rx_tkeep,
   input [4*64-1:0]                           i_m0_axi_rx_tdata,
   input [4*1-1:0]                            i_m0_axi_rx_tlast,
   input [4*1-1:0]                            i_m0_axi_rx_tvalid,

   // AXI Connections (QSFP1)
   output [4*8-1:0]                           o_s1_axi_tx_tkeep,
   output [4*64-1:0]                          o_s1_axi_tx_tdata,
   output [4*1-1:0]                           o_s1_axi_tx_tlast,
   output [4*1-1:0]                           o_s1_axi_tx_tvalid,
   input [4*1-1:0]                            i_s1_axi_tx_tready,

   input [4*8-1:0]                            i_m1_axi_rx_tkeep,
   input [4*64-1:0]                           i_m1_axi_rx_tdata,
   input [4*1-1:0]                            i_m1_axi_rx_tlast,
   input [4*1-1:0]                            i_m1_axi_rx_tvalid,

   input [3:0]                                stat_chan_up0,
   input [3:0]                                stat_chan_up1,

   // user-facing ports
   input [NUM_UIO_PORTS*1-1 :0]               uio_rq_vld,
   input [NUM_UIO_PORTS*UIO_PORTS_WIDTH-1:0]  uio_rq_data,
   output [NUM_UIO_PORTS*1-1 :0]              uio_rq_afull,

   output [NUM_UIO_PORTS*1-1 :0]              uio_rs_vld,
   output [NUM_UIO_PORTS*UIO_PORTS_WIDTH-1:0] uio_rs_data,
   input [NUM_UIO_PORTS*1-1 :0]               uio_rs_afull
);

   // User IO Port <-> AXI
   // This will require reworking when changing NUM_UIO_PORTS / UIO_PORTS_WIDTH and/or QSFP*_WIDTH
       
   user_io_axi_conv aur_uio_P0_qsfp0_P0
     (// Outputs
      .o_s_axi_tx_tkeep        (o_s0_axi_tx_tkeep[(0+1)*8-1:0*8]),
      .o_s_axi_tx_tdata        (o_s0_axi_tx_tdata[(0+1)*64-1:0*64]),
      .o_s_axi_tx_tlast        (o_s0_axi_tx_tlast[0]),
      .o_s_axi_tx_tvalid       (o_s0_axi_tx_tvalid[0]),
      .uio_rq_afull            (uio_rq_afull[0]),
      .uio_rs_vld              (uio_rs_vld[0]),
      .uio_rs_data             (uio_rs_data[(0+1)*UIO_PORTS_WIDTH-1:0*UIO_PORTS_WIDTH]),
      // Inputs
      .clk                     (clk0[0]),
      .clk_per                 (clk_per),
      .reset                   (reset0[0]),
      .reset_per               (reset_per),
      .i_s_axi_tx_tready       (i_s0_axi_tx_tready[0]),
      .i_m_axi_rx_tkeep        (i_m0_axi_rx_tkeep[(0+1)*8-1:0*8]),
      .i_m_axi_rx_tdata        (i_m0_axi_rx_tdata[(0+1)*64-1:0*64]),
      .i_m_axi_rx_tlast        (i_m0_axi_rx_tlast[0]),
      .i_m_axi_rx_tvalid       (i_m0_axi_rx_tvalid[0]),
      .i_stat_chan_up          (stat_chan_up0[0]),
      .uio_rq_vld              (uio_rq_vld[0]),
      .uio_rq_data             (uio_rq_data[(0+1)*UIO_PORTS_WIDTH-1:0*UIO_PORTS_WIDTH]),
      .uio_rs_afull            (uio_rs_afull[0]));
       
   user_io_axi_conv aur_uio_P1_qsfp0_P1
     (// Outputs
      .o_s_axi_tx_tkeep        (o_s0_axi_tx_tkeep[(1+1)*8-1:1*8]),
      .o_s_axi_tx_tdata        (o_s0_axi_tx_tdata[(1+1)*64-1:1*64]),
      .o_s_axi_tx_tlast        (o_s0_axi_tx_tlast[1]),
      .o_s_axi_tx_tvalid       (o_s0_axi_tx_tvalid[1]),
      .uio_rq_afull            (uio_rq_afull[1]),
      .uio_rs_vld              (uio_rs_vld[1]),
      .uio_rs_data             (uio_rs_data[(1+1)*UIO_PORTS_WIDTH-1:1*UIO_PORTS_WIDTH]),
      // Inputs
      .clk                     (clk0[1]),
      .clk_per                 (clk_per),
      .reset                   (reset0[1]),
      .reset_per               (reset_per),
      .i_s_axi_tx_tready       (i_s0_axi_tx_tready[1]),
      .i_m_axi_rx_tkeep        (i_m0_axi_rx_tkeep[(1+1)*8-1:1*8]),
      .i_m_axi_rx_tdata        (i_m0_axi_rx_tdata[(1+1)*64-1:1*64]),
      .i_m_axi_rx_tlast        (i_m0_axi_rx_tlast[1]),
      .i_m_axi_rx_tvalid       (i_m0_axi_rx_tvalid[1]),
      .i_stat_chan_up          (stat_chan_up0[1]),
      .uio_rq_vld              (uio_rq_vld[1]),
      .uio_rq_data             (uio_rq_data[(1+1)*UIO_PORTS_WIDTH-1:1*UIO_PORTS_WIDTH]),
      .uio_rs_afull            (uio_rs_afull[1]));

   user_io_axi_conv aur_uio_P2_qsfp0_P2
     (// Outputs
      .o_s_axi_tx_tkeep        (o_s0_axi_tx_tkeep[(2+1)*8-1:2*8]),
      .o_s_axi_tx_tdata        (o_s0_axi_tx_tdata[(2+1)*64-1:2*64]),
      .o_s_axi_tx_tlast        (o_s0_axi_tx_tlast[2]),
      .o_s_axi_tx_tvalid       (o_s0_axi_tx_tvalid[2]),
      .uio_rq_afull            (uio_rq_afull[2]),
      .uio_rs_vld              (uio_rs_vld[2]),
      .uio_rs_data             (uio_rs_data[(2+1)*UIO_PORTS_WIDTH-1:2*UIO_PORTS_WIDTH]),
      // Inputs
      .clk                     (clk0[2]),
      .clk_per                 (clk_per),
      .reset                   (reset0[2]),
      .reset_per               (reset_per),
      .i_s_axi_tx_tready       (i_s0_axi_tx_tready[2]),
      .i_m_axi_rx_tkeep        (i_m0_axi_rx_tkeep[(2+1)*8-1:2*8]),
      .i_m_axi_rx_tdata        (i_m0_axi_rx_tdata[(2+1)*64-1:2*64]),
      .i_m_axi_rx_tlast        (i_m0_axi_rx_tlast[2]),
      .i_m_axi_rx_tvalid       (i_m0_axi_rx_tvalid[2]),
      .i_stat_chan_up          (stat_chan_up0[2]),
      .uio_rq_vld              (uio_rq_vld[2]),
      .uio_rq_data             (uio_rq_data[(2+1)*UIO_PORTS_WIDTH-1:2*UIO_PORTS_WIDTH]),
      .uio_rs_afull            (uio_rs_afull[2]));
       
   user_io_axi_conv aur_uio_P3_qsfp0_P3
     (// Outputs
      .o_s_axi_tx_tkeep        (o_s0_axi_tx_tkeep[(3+1)*8-1:3*8]),
      .o_s_axi_tx_tdata        (o_s0_axi_tx_tdata[(3+1)*64-1:3*64]),
      .o_s_axi_tx_tlast        (o_s0_axi_tx_tlast[3]),
      .o_s_axi_tx_tvalid       (o_s0_axi_tx_tvalid[3]),
      .uio_rq_afull            (uio_rq_afull[3]),
      .uio_rs_vld              (uio_rs_vld[3]),
      .uio_rs_data             (uio_rs_data[(3+1)*UIO_PORTS_WIDTH-1:3*UIO_PORTS_WIDTH]),
      // Inputs
      .clk                     (clk0[3]),
      .clk_per                 (clk_per),
      .reset                   (reset0[3]),
      .reset_per               (reset_per),
      .i_s_axi_tx_tready       (i_s0_axi_tx_tready[3]),
      .i_m_axi_rx_tkeep        (i_m0_axi_rx_tkeep[(3+1)*8-1:3*8]),
      .i_m_axi_rx_tdata        (i_m0_axi_rx_tdata[(3+1)*64-1:3*64]),
      .i_m_axi_rx_tlast        (i_m0_axi_rx_tlast[3]),
      .i_m_axi_rx_tvalid       (i_m0_axi_rx_tvalid[3]),
      .i_stat_chan_up          (stat_chan_up0[3]),
      .uio_rq_vld              (uio_rq_vld[3]),
      .uio_rq_data             (uio_rq_data[(3+1)*UIO_PORTS_WIDTH-1:3*UIO_PORTS_WIDTH]),
      .uio_rs_afull            (uio_rs_afull[3]));
       
   user_io_axi_conv aur_uio_P4_qsfp1_P0
     (// Outputs
      .o_s_axi_tx_tkeep        (o_s1_axi_tx_tkeep[(0+1)*8-1:0*8]),
      .o_s_axi_tx_tdata        (o_s1_axi_tx_tdata[(0+1)*64-1:0*64]),
      .o_s_axi_tx_tlast        (o_s1_axi_tx_tlast[0]),
      .o_s_axi_tx_tvalid       (o_s1_axi_tx_tvalid[0]),
      .uio_rq_afull            (uio_rq_afull[4]),
      .uio_rs_vld              (uio_rs_vld[4]),
      .uio_rs_data             (uio_rs_data[(4+1)*UIO_PORTS_WIDTH-1:4*UIO_PORTS_WIDTH]),
      // Inputs
      .clk                     (clk1[0]),
      .clk_per                 (clk_per),
      .reset                   (reset1[0]),
      .reset_per               (reset_per),
      .i_s_axi_tx_tready       (i_s1_axi_tx_tready[0]),
      .i_m_axi_rx_tkeep        (i_m1_axi_rx_tkeep[(0+1)*8-1:0*8]),
      .i_m_axi_rx_tdata        (i_m1_axi_rx_tdata[(0+1)*64-1:0*64]),
      .i_m_axi_rx_tlast        (i_m1_axi_rx_tlast[0]),
      .i_m_axi_rx_tvalid       (i_m1_axi_rx_tvalid[0]),
      .i_stat_chan_up          (stat_chan_up1[0]),
      .uio_rq_vld              (uio_rq_vld[4]),
      .uio_rq_data             (uio_rq_data[(4+1)*UIO_PORTS_WIDTH-1:4*UIO_PORTS_WIDTH]),
      .uio_rs_afull            (uio_rs_afull[4]));
       
   user_io_axi_conv aur_uio_P5_qsfp1_P1
     (// Outputs
      .o_s_axi_tx_tkeep        (o_s1_axi_tx_tkeep[(1+1)*8-1:1*8]),
      .o_s_axi_tx_tdata        (o_s1_axi_tx_tdata[(1+1)*64-1:1*64]),
      .o_s_axi_tx_tlast        (o_s1_axi_tx_tlast[1]),
      .o_s_axi_tx_tvalid       (o_s1_axi_tx_tvalid[1]),
      .uio_rq_afull            (uio_rq_afull[5]),
      .uio_rs_vld              (uio_rs_vld[5]),
      .uio_rs_data             (uio_rs_data[(5+1)*UIO_PORTS_WIDTH-1:5*UIO_PORTS_WIDTH]),
      // Inputs
      .clk                     (clk1[1]),
      .clk_per                 (clk_per),
      .reset                   (reset1[1]),
      .reset_per               (reset_per),
      .i_s_axi_tx_tready       (i_s1_axi_tx_tready[1]),
      .i_m_axi_rx_tkeep        (i_m1_axi_rx_tkeep[(1+1)*8-1:1*8]),
      .i_m_axi_rx_tdata        (i_m1_axi_rx_tdata[(1+1)*64-1:1*64]),
      .i_m_axi_rx_tlast        (i_m1_axi_rx_tlast[1]),
      .i_m_axi_rx_tvalid       (i_m1_axi_rx_tvalid[1]),
      .i_stat_chan_up          (stat_chan_up1[1]),
      .uio_rq_vld              (uio_rq_vld[5]),
      .uio_rq_data             (uio_rq_data[(5+1)*UIO_PORTS_WIDTH-1:5*UIO_PORTS_WIDTH]),
      .uio_rs_afull            (uio_rs_afull[5]));
       
   user_io_axi_conv aur_uio_P6_qsfp1_P2
     (// Outputs
      .o_s_axi_tx_tkeep        (o_s1_axi_tx_tkeep[(2+1)*8-1:2*8]),
      .o_s_axi_tx_tdata        (o_s1_axi_tx_tdata[(2+1)*64-1:2*64]),
      .o_s_axi_tx_tlast        (o_s1_axi_tx_tlast[2]),
      .o_s_axi_tx_tvalid       (o_s1_axi_tx_tvalid[2]),
      .uio_rq_afull            (uio_rq_afull[6]),
      .uio_rs_vld              (uio_rs_vld[6]),
      .uio_rs_data             (uio_rs_data[(6+1)*UIO_PORTS_WIDTH-1:6*UIO_PORTS_WIDTH]),
      // Inputs
      .clk                     (clk1[2]),
      .clk_per                 (clk_per),
      .reset                   (reset1[2]),
      .reset_per               (reset_per),
      .i_s_axi_tx_tready       (i_s1_axi_tx_tready[2]),
      .i_m_axi_rx_tkeep        (i_m1_axi_rx_tkeep[(2+1)*8-1:2*8]),
      .i_m_axi_rx_tdata        (i_m1_axi_rx_tdata[(2+1)*64-1:2*64]),
      .i_m_axi_rx_tlast        (i_m1_axi_rx_tlast[2]),
      .i_m_axi_rx_tvalid       (i_m1_axi_rx_tvalid[2]),
      .i_stat_chan_up          (stat_chan_up1[2]),
      .uio_rq_vld              (uio_rq_vld[6]),
      .uio_rq_data             (uio_rq_data[(6+1)*UIO_PORTS_WIDTH-1:6*UIO_PORTS_WIDTH]),
      .uio_rs_afull            (uio_rs_afull[6]));
       
   user_io_axi_conv aur_uio_P7_qsfp1_P3
     (// Outputs
      .o_s_axi_tx_tkeep        (o_s1_axi_tx_tkeep[(3+1)*8-1:3*8]),
      .o_s_axi_tx_tdata        (o_s1_axi_tx_tdata[(3+1)*64-1:3*64]),
      .o_s_axi_tx_tlast        (o_s1_axi_tx_tlast[3]),
      .o_s_axi_tx_tvalid       (o_s1_axi_tx_tvalid[3]),
      .uio_rq_afull            (uio_rq_afull[7]),
      .uio_rs_vld              (uio_rs_vld[7]),
      .uio_rs_data             (uio_rs_data[(7+1)*UIO_PORTS_WIDTH-1:7*UIO_PORTS_WIDTH]),
      // Inputs
      .clk                     (clk1[3]),
      .clk_per                 (clk_per),
      .reset                   (reset1[3]),
      .reset_per               (reset_per),
      .i_s_axi_tx_tready       (i_s1_axi_tx_tready[3]),
      .i_m_axi_rx_tkeep        (i_m1_axi_rx_tkeep[(3+1)*8-1:3*8]),
      .i_m_axi_rx_tdata        (i_m1_axi_rx_tdata[(3+1)*64-1:3*64]),
      .i_m_axi_rx_tlast        (i_m1_axi_rx_tlast[3]),
      .i_m_axi_rx_tvalid       (i_m1_axi_rx_tvalid[3]),
      .i_stat_chan_up          (stat_chan_up1[3]),
      .uio_rq_vld              (uio_rq_vld[7]),
      .uio_rq_data             (uio_rq_data[(7+1)*UIO_PORTS_WIDTH-1:7*UIO_PORTS_WIDTH]),
      .uio_rs_afull            (uio_rs_afull[7]));


endmodule
