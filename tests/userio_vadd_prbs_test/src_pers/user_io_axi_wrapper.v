`timescale 1 ns / 1 ps

module user_io_axi_wrapper #
(
   parameter NUM_AUR_LINKS   = 8,
   parameter NUM_UIO_PORTS   = 8,
   parameter UIO_PORTS_WIDTH = 128
)(
   input [NUM_AUR_LINKS*1-1:0]		clk,
   input 				clk_per,
   input [NUM_AUR_LINKS*1-1:0]		reset,
   input 				reset_per,

   // AXI Connections 
   output [NUM_AUR_LINKS*8-1:0] 	o_s_axi_tx_tkeep,
   output [NUM_AUR_LINKS*64-1:0] 	o_s_axi_tx_tdata,
   output [NUM_AUR_LINKS*1-1:0] 	o_s_axi_tx_tlast,
   output [NUM_AUR_LINKS*1-1:0] 	o_s_axi_tx_tvalid,
   input [NUM_AUR_LINKS*1-1:0] 		i_s_axi_tx_tready,

   input [NUM_AUR_LINKS*8-1:0] 		i_m_axi_rx_tkeep,
   input [NUM_AUR_LINKS*64-1:0] 	i_m_axi_rx_tdata,
   input [NUM_AUR_LINKS*1-1:0] 		i_m_axi_rx_tlast,
   input [NUM_AUR_LINKS*1-1:0] 		i_m_axi_rx_tvalid,

   input [NUM_AUR_LINKS*1-1:0] 		stat_chan_up,

   // user-facing ports
   input  [NUM_UIO_PORTS*1-1 :0] uio_rq_vld,
   input  [NUM_UIO_PORTS*UIO_PORTS_WIDTH-1:0] uio_rq_data,
   output [NUM_UIO_PORTS*1-1 :0] uio_rq_afull,

   output [NUM_UIO_PORTS*1-1 :0] uio_rs_vld,
   output [NUM_UIO_PORTS*UIO_PORTS_WIDTH-1:0] uio_rs_data,
   input  [NUM_UIO_PORTS*1-1 :0] uio_rs_afull
);

   // User IO Port <-> AXI
   // Using equal numbers of User IO ports and AXI ports to make this simpler...
   genvar conv;
   generate for (conv = 0; conv < 8; conv=conv+1) begin : g0_conv

      if (conv == 0) begin : p0
         if (conv < NUM_AUR_LINKS) begin : conv_if
	    
            user_io_axi_conv aurora_userio_p0
              (/*AUTOINST*/
	       // Outputs
	       .o_s_axi_tx_tkeep	(o_s_axi_tx_tkeep[7:0]), // Templated
	       .o_s_axi_tx_tdata	(o_s_axi_tx_tdata[63:0]), // Templated
	       .o_s_axi_tx_tlast	(o_s_axi_tx_tlast[0]),	 // Templated
	       .o_s_axi_tx_tvalid	(o_s_axi_tx_tvalid[0]),	 // Templated
	       .uio_rq_afull		(uio_rq_afull[0]),	 // Templated
	       .uio_rs_vld		(uio_rs_vld[0]),	 // Templated
	       .uio_rs_data		(uio_rs_data[1*UIO_PORTS_WIDTH-1:0*UIO_PORTS_WIDTH]), // Templated
	       // Inputs
	       .clk			(clk[0]),		 // Templated
	       .clk_per			(clk_per),
	       .reset			(reset[0]),		 // Templated
	       .reset_per		(reset_per),
	       .i_s_axi_tx_tready	(i_s_axi_tx_tready[0]),	 // Templated
	       .i_m_axi_rx_tkeep	(i_m_axi_rx_tkeep[7:0]), // Templated
	       .i_m_axi_rx_tdata	(i_m_axi_rx_tdata[63:0]), // Templated
	       .i_m_axi_rx_tlast	(i_m_axi_rx_tlast[0]),	 // Templated
	       .i_m_axi_rx_tvalid	(i_m_axi_rx_tvalid[0]),	 // Templated
	       .i_stat_chan_up		(stat_chan_up[0]),	 // Templated
	       .uio_rq_vld		(uio_rq_vld[0]),	 // Templated
	       .uio_rq_data		(uio_rq_data[1*UIO_PORTS_WIDTH-1:0*UIO_PORTS_WIDTH]), // Templated
	       .uio_rs_afull		(uio_rs_afull[0]));	 // Templated
         end
      end


      if (conv == 1) begin : p1
         if (conv < NUM_AUR_LINKS) begin : conv_if
	    
            user_io_axi_conv aurora_userio_p1
              (/*AUTOINST*/
	       // Outputs
	       .o_s_axi_tx_tkeep	(o_s_axi_tx_tkeep[15:8]), // Templated
	       .o_s_axi_tx_tdata	(o_s_axi_tx_tdata[127:64]), // Templated
	       .o_s_axi_tx_tlast	(o_s_axi_tx_tlast[1]),	 // Templated
	       .o_s_axi_tx_tvalid	(o_s_axi_tx_tvalid[1]),	 // Templated
	       .uio_rq_afull		(uio_rq_afull[1]),	 // Templated
	       .uio_rs_vld		(uio_rs_vld[1]),	 // Templated
	       .uio_rs_data		(uio_rs_data[2*UIO_PORTS_WIDTH-1:1*UIO_PORTS_WIDTH]), // Templated
	       // Inputs
	       .clk			(clk[1]),		 // Templated
	       .clk_per			(clk_per),
	       .reset			(reset[1]),		 // Templated
	       .reset_per		(reset_per),
	       .i_s_axi_tx_tready	(i_s_axi_tx_tready[1]),	 // Templated
	       .i_m_axi_rx_tkeep	(i_m_axi_rx_tkeep[15:8]), // Templated
	       .i_m_axi_rx_tdata	(i_m_axi_rx_tdata[127:64]), // Templated
	       .i_m_axi_rx_tlast	(i_m_axi_rx_tlast[1]),	 // Templated
	       .i_m_axi_rx_tvalid	(i_m_axi_rx_tvalid[1]),	 // Templated
	       .i_stat_chan_up		(stat_chan_up[1]),	 // Templated
	       .uio_rq_vld		(uio_rq_vld[1]),	 // Templated
	       .uio_rq_data		(uio_rq_data[2*UIO_PORTS_WIDTH-1:1*UIO_PORTS_WIDTH]), // Templated
	       .uio_rs_afull		(uio_rs_afull[1]));	 // Templated
         end
      end


      if (conv == 2) begin : p2
         if (conv < NUM_AUR_LINKS) begin : conv_if
	    
            user_io_axi_conv aurora_userio_p2
              (/*AUTOINST*/
	       // Outputs
	       .o_s_axi_tx_tkeep	(o_s_axi_tx_tkeep[23:16]), // Templated
	       .o_s_axi_tx_tdata	(o_s_axi_tx_tdata[191:128]), // Templated
	       .o_s_axi_tx_tlast	(o_s_axi_tx_tlast[2]),	 // Templated
	       .o_s_axi_tx_tvalid	(o_s_axi_tx_tvalid[2]),	 // Templated
	       .uio_rq_afull		(uio_rq_afull[2]),	 // Templated
	       .uio_rs_vld		(uio_rs_vld[2]),	 // Templated
	       .uio_rs_data		(uio_rs_data[3*UIO_PORTS_WIDTH-1:2*UIO_PORTS_WIDTH]), // Templated
	       // Inputs
	       .clk			(clk[2]),		 // Templated
	       .clk_per			(clk_per),
	       .reset			(reset[2]),		 // Templated
	       .reset_per		(reset_per),
	       .i_s_axi_tx_tready	(i_s_axi_tx_tready[2]),	 // Templated
	       .i_m_axi_rx_tkeep	(i_m_axi_rx_tkeep[23:16]), // Templated
	       .i_m_axi_rx_tdata	(i_m_axi_rx_tdata[191:128]), // Templated
	       .i_m_axi_rx_tlast	(i_m_axi_rx_tlast[2]),	 // Templated
	       .i_m_axi_rx_tvalid	(i_m_axi_rx_tvalid[2]),	 // Templated
	       .i_stat_chan_up		(stat_chan_up[2]),	 // Templated
	       .uio_rq_vld		(uio_rq_vld[2]),	 // Templated
	       .uio_rq_data		(uio_rq_data[3*UIO_PORTS_WIDTH-1:2*UIO_PORTS_WIDTH]), // Templated
	       .uio_rs_afull		(uio_rs_afull[2]));	 // Templated
         end
      end


      if (conv == 3) begin : p3
         if (conv < NUM_AUR_LINKS) begin : conv_if
	    
            user_io_axi_conv aurora_userio_p3
              (/*AUTOINST*/
	       // Outputs
	       .o_s_axi_tx_tkeep	(o_s_axi_tx_tkeep[31:24]), // Templated
	       .o_s_axi_tx_tdata	(o_s_axi_tx_tdata[255:192]), // Templated
	       .o_s_axi_tx_tlast	(o_s_axi_tx_tlast[3]),	 // Templated
	       .o_s_axi_tx_tvalid	(o_s_axi_tx_tvalid[3]),	 // Templated
	       .uio_rq_afull		(uio_rq_afull[3]),	 // Templated
	       .uio_rs_vld		(uio_rs_vld[3]),	 // Templated
	       .uio_rs_data		(uio_rs_data[4*UIO_PORTS_WIDTH-1:3*UIO_PORTS_WIDTH]), // Templated
	       // Inputs
	       .clk			(clk[3]),		 // Templated
	       .clk_per			(clk_per),
	       .reset			(reset[3]),		 // Templated
	       .reset_per		(reset_per),
	       .i_s_axi_tx_tready	(i_s_axi_tx_tready[3]),	 // Templated
	       .i_m_axi_rx_tkeep	(i_m_axi_rx_tkeep[31:24]), // Templated
	       .i_m_axi_rx_tdata	(i_m_axi_rx_tdata[255:192]), // Templated
	       .i_m_axi_rx_tlast	(i_m_axi_rx_tlast[3]),	 // Templated
	       .i_m_axi_rx_tvalid	(i_m_axi_rx_tvalid[3]),	 // Templated
	       .i_stat_chan_up		(stat_chan_up[3]),	 // Templated
	       .uio_rq_vld		(uio_rq_vld[3]),	 // Templated
	       .uio_rq_data		(uio_rq_data[4*UIO_PORTS_WIDTH-1:3*UIO_PORTS_WIDTH]), // Templated
	       .uio_rs_afull		(uio_rs_afull[3]));	 // Templated
         end
      end


      if (conv == 4) begin : p4
         if (conv < NUM_AUR_LINKS) begin : conv_if
	    
            user_io_axi_conv aurora_userio_p4
              (/*AUTOINST*/
	       // Outputs
	       .o_s_axi_tx_tkeep	(o_s_axi_tx_tkeep[39:32]), // Templated
	       .o_s_axi_tx_tdata	(o_s_axi_tx_tdata[319:256]), // Templated
	       .o_s_axi_tx_tlast	(o_s_axi_tx_tlast[4]),	 // Templated
	       .o_s_axi_tx_tvalid	(o_s_axi_tx_tvalid[4]),	 // Templated
	       .uio_rq_afull		(uio_rq_afull[4]),	 // Templated
	       .uio_rs_vld		(uio_rs_vld[4]),	 // Templated
	       .uio_rs_data		(uio_rs_data[5*UIO_PORTS_WIDTH-1:4*UIO_PORTS_WIDTH]), // Templated
	       // Inputs
	       .clk			(clk[4]),		 // Templated
	       .clk_per			(clk_per),
	       .reset			(reset[4]),		 // Templated
	       .reset_per		(reset_per),
	       .i_s_axi_tx_tready	(i_s_axi_tx_tready[4]),	 // Templated
	       .i_m_axi_rx_tkeep	(i_m_axi_rx_tkeep[39:32]), // Templated
	       .i_m_axi_rx_tdata	(i_m_axi_rx_tdata[319:256]), // Templated
	       .i_m_axi_rx_tlast	(i_m_axi_rx_tlast[4]),	 // Templated
	       .i_m_axi_rx_tvalid	(i_m_axi_rx_tvalid[4]),	 // Templated
	       .i_stat_chan_up		(stat_chan_up[4]),	 // Templated
	       .uio_rq_vld		(uio_rq_vld[4]),	 // Templated
	       .uio_rq_data		(uio_rq_data[5*UIO_PORTS_WIDTH-1:4*UIO_PORTS_WIDTH]), // Templated
	       .uio_rs_afull		(uio_rs_afull[4]));	 // Templated
         end
      end


      if (conv == 5) begin : p5
         if (conv < NUM_AUR_LINKS) begin : conv_if
	    
            user_io_axi_conv aurora_userio_p5
              (/*AUTOINST*/
	       // Outputs
	       .o_s_axi_tx_tkeep	(o_s_axi_tx_tkeep[47:40]), // Templated
	       .o_s_axi_tx_tdata	(o_s_axi_tx_tdata[383:320]), // Templated
	       .o_s_axi_tx_tlast	(o_s_axi_tx_tlast[5]),	 // Templated
	       .o_s_axi_tx_tvalid	(o_s_axi_tx_tvalid[5]),	 // Templated
	       .uio_rq_afull		(uio_rq_afull[5]),	 // Templated
	       .uio_rs_vld		(uio_rs_vld[5]),	 // Templated
	       .uio_rs_data		(uio_rs_data[6*UIO_PORTS_WIDTH-1:5*UIO_PORTS_WIDTH]), // Templated
	       // Inputs
	       .clk			(clk[5]),		 // Templated
	       .clk_per			(clk_per),
	       .reset			(reset[5]),		 // Templated
	       .reset_per		(reset_per),
	       .i_s_axi_tx_tready	(i_s_axi_tx_tready[5]),	 // Templated
	       .i_m_axi_rx_tkeep	(i_m_axi_rx_tkeep[47:40]), // Templated
	       .i_m_axi_rx_tdata	(i_m_axi_rx_tdata[383:320]), // Templated
	       .i_m_axi_rx_tlast	(i_m_axi_rx_tlast[5]),	 // Templated
	       .i_m_axi_rx_tvalid	(i_m_axi_rx_tvalid[5]),	 // Templated
	       .i_stat_chan_up		(stat_chan_up[5]),	 // Templated
	       .uio_rq_vld		(uio_rq_vld[5]),	 // Templated
	       .uio_rq_data		(uio_rq_data[6*UIO_PORTS_WIDTH-1:5*UIO_PORTS_WIDTH]), // Templated
	       .uio_rs_afull		(uio_rs_afull[5]));	 // Templated
         end
      end


      if (conv == 6) begin : p6
         if (conv < NUM_AUR_LINKS) begin : conv_if
	    
            user_io_axi_conv aurora_userio_p6
              (/*AUTOINST*/
	       // Outputs
	       .o_s_axi_tx_tkeep	(o_s_axi_tx_tkeep[55:48]), // Templated
	       .o_s_axi_tx_tdata	(o_s_axi_tx_tdata[447:384]), // Templated
	       .o_s_axi_tx_tlast	(o_s_axi_tx_tlast[6]),	 // Templated
	       .o_s_axi_tx_tvalid	(o_s_axi_tx_tvalid[6]),	 // Templated
	       .uio_rq_afull		(uio_rq_afull[6]),	 // Templated
	       .uio_rs_vld		(uio_rs_vld[6]),	 // Templated
	       .uio_rs_data		(uio_rs_data[7*UIO_PORTS_WIDTH-1:6*UIO_PORTS_WIDTH]), // Templated
	       // Inputs
	       .clk			(clk[6]),		 // Templated
	       .clk_per			(clk_per),
	       .reset			(reset[6]),		 // Templated
	       .reset_per		(reset_per),
	       .i_s_axi_tx_tready	(i_s_axi_tx_tready[6]),	 // Templated
	       .i_m_axi_rx_tkeep	(i_m_axi_rx_tkeep[55:48]), // Templated
	       .i_m_axi_rx_tdata	(i_m_axi_rx_tdata[447:384]), // Templated
	       .i_m_axi_rx_tlast	(i_m_axi_rx_tlast[6]),	 // Templated
	       .i_m_axi_rx_tvalid	(i_m_axi_rx_tvalid[6]),	 // Templated
	       .i_stat_chan_up		(stat_chan_up[6]),	 // Templated
	       .uio_rq_vld		(uio_rq_vld[6]),	 // Templated
	       .uio_rq_data		(uio_rq_data[7*UIO_PORTS_WIDTH-1:6*UIO_PORTS_WIDTH]), // Templated
	       .uio_rs_afull		(uio_rs_afull[6]));	 // Templated
         end
      end


      if (conv == 7) begin : p7
         if (conv < NUM_AUR_LINKS) begin : conv_if
	    
            user_io_axi_conv aurora_userio_p7
              (/*AUTOINST*/
	       // Outputs
	       .o_s_axi_tx_tkeep	(o_s_axi_tx_tkeep[63:56]), // Templated
	       .o_s_axi_tx_tdata	(o_s_axi_tx_tdata[511:448]), // Templated
	       .o_s_axi_tx_tlast	(o_s_axi_tx_tlast[7]),	 // Templated
	       .o_s_axi_tx_tvalid	(o_s_axi_tx_tvalid[7]),	 // Templated
	       .uio_rq_afull		(uio_rq_afull[7]),	 // Templated
	       .uio_rs_vld		(uio_rs_vld[7]),	 // Templated
	       .uio_rs_data		(uio_rs_data[8*UIO_PORTS_WIDTH-1:7*UIO_PORTS_WIDTH]), // Templated
	       // Inputs
	       .clk			(clk[7]),		 // Templated
	       .clk_per			(clk_per),
	       .reset			(reset[7]),		 // Templated
	       .reset_per		(reset_per),
	       .i_s_axi_tx_tready	(i_s_axi_tx_tready[7]),	 // Templated
	       .i_m_axi_rx_tkeep	(i_m_axi_rx_tkeep[63:56]), // Templated
	       .i_m_axi_rx_tdata	(i_m_axi_rx_tdata[511:448]), // Templated
	       .i_m_axi_rx_tlast	(i_m_axi_rx_tlast[7]),	 // Templated
	       .i_m_axi_rx_tvalid	(i_m_axi_rx_tvalid[7]),	 // Templated
	       .i_stat_chan_up		(stat_chan_up[7]),	 // Templated
	       .uio_rq_vld		(uio_rq_vld[7]),	 // Templated
	       .uio_rq_data		(uio_rq_data[8*UIO_PORTS_WIDTH-1:7*UIO_PORTS_WIDTH]), // Templated
	       .uio_rs_afull		(uio_rs_afull[7]));	 // Templated
         end
      end

   end endgenerate

/* user_io_axi_conv AUTO_TEMPLATE (
      // Outputs
      .o_s_axi_tx_tkeep		(o_s_axi_tx_tkeep[@"(- (* (+ 1 @) 8) 1)":@"(* @ 8)"]),
      .o_s_axi_tx_tdata		(o_s_axi_tx_tdata[@"(- (* (+ 1 @) 64) 1)":@"(* @ 64)"]),
      .o_s_axi_tx_tlast		(o_s_axi_tx_tlast[@]),
      .o_s_axi_tx_tvalid	(o_s_axi_tx_tvalid[@]),
      .uio_rq_afull		(uio_rq_afull[@]),
      .uio_rs_vld		(uio_rs_vld[@]),
      .uio_rs_data		(uio_rs_data[@"(+ 1 @)"*UIO_PORTS_WIDTH-1:@*UIO_PORTS_WIDTH]),
      // Inputs
      .clk			(clk[@]),
      .reset			(reset[@]),
      .i_s_axi_tx_tready	(i_s_axi_tx_tready[@]),
      .i_m_axi_rx_tkeep		(i_m_axi_rx_tkeep[@"(- (* (+ 1 @) 8) 1)":@"(* @ 8)"]),
      .i_m_axi_rx_tdata		(i_m_axi_rx_tdata[@"(- (* (+ 1 @) 64) 1)":@"(* @ 64)"]),
      .i_m_axi_rx_tlast		(i_m_axi_rx_tlast[@]),
      .i_m_axi_rx_tvalid	(i_m_axi_rx_tvalid[@]),
      .i_stat_chan_up		(stat_chan_up[@]),
      .uio_rq_vld		(uio_rq_vld[@]),
      .uio_rq_data		(uio_rq_data[@"(+ 1 @)"*UIO_PORTS_WIDTH-1:@*UIO_PORTS_WIDTH]),
      .uio_rs_afull		(uio_rs_afull[@]));
   ); */

endmodule
