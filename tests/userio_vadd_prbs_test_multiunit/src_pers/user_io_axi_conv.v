`timescale 1 ns / 1 ps

module user_io_axi_conv #
(
   parameter UIO_PORTS_WIDTH   = 128,
   parameter RQ_AXI_FIFO_DEPTH = 64,
   parameter RS_AXI_FIFO_DEPTH = 64
)(
   input 			clk,
   input 			clk_per,
   input 			reset,
   input 			reset_per,

   // AXI Connections 
   output [7:0] 		o_s_axi_tx_tkeep,
   output [63:0] 		o_s_axi_tx_tdata,
   output 			o_s_axi_tx_tlast,
   output 			o_s_axi_tx_tvalid,
   input 			i_s_axi_tx_tready,

   input [7:0] 			i_m_axi_rx_tkeep,
   input [63:0] 		i_m_axi_rx_tdata,
   input 			i_m_axi_rx_tlast,
   input 			i_m_axi_rx_tvalid,

   input 			i_stat_chan_up,

   // user-facing ports
   input 			uio_rq_vld,
   input [UIO_PORTS_WIDTH-1:0] 	uio_rq_data,
   output 			uio_rq_afull,

   output 			uio_rs_vld,
   output [UIO_PORTS_WIDTH-1:0] uio_rs_data,
   input 			uio_rs_afull
);

   // Request AXI Clock Crossing FIFO
   // FIXME: RATIO/SYNC???
   wire 		      r_axi_rq_pop;
   wire [UIO_PORTS_WIDTH-1:0] r_axi_rq_dout;
   wire 		      r_axi_rq_empty;
   wire 		      r_uio_rq_afull;

   wire 		      fifo_reset = reset | reset_per | !i_stat_chan_up;
      
   fifo #(.WIDTH(UIO_PORTS_WIDTH), .DEPTH(RQ_AXI_FIFO_DEPTH), .AFULLCNT(RQ_AXI_FIFO_DEPTH-6), .RATIO(3), .SYNCHRONOUS(0), .OSYNC_RESET(0))
   rq_axi_fifo (
		 .clk(clk_per),
		 .reset(fifo_reset),
		 .push(uio_rq_vld),
		 .din(uio_rq_data),
		 .afull(r_uio_rq_afull),
		 .full(), 
		 .cnt(),
		 .oclk(clk),
		 .pop(r_axi_rq_pop),
		 .dout(r_axi_rq_dout),
		 .empty(r_axi_rq_empty),
		 .rcnt()
   );

   // Request AXI Packet Formation
   // Using 512b/64B packets - tlast every 8 valid cycles
   reg [2:0] 	c_rq_axi_cyc_cnt;
   wire [2:0] 	r_rq_axi_cyc_cnt;
   wire         r_rq_tx_ready = (!r_axi_rq_empty & i_s_axi_tx_tready);
   wire [63:0] 	r_rq_tx_data  = (r_rq_axi_cyc_cnt[0] == 1'd0) ? r_axi_rq_dout[63:0] : r_axi_rq_dout[127:64];
   wire         r_rq_tx_last  = (r_rq_axi_cyc_cnt == 3'd7) ? 1'b1 : 1'b0;
      
   always @(*) begin
      c_rq_axi_cyc_cnt = r_rq_axi_cyc_cnt;

      // (integer overflow expected)
      if (r_rq_tx_ready) begin
	 c_rq_axi_cyc_cnt = r_rq_axi_cyc_cnt + 3'b1;
      end
   end
   
   dffr #(.WIDTH(3)) dff_rq_axi_cyc_cnt (.clk(clk), .rst(reset), .d(c_rq_axi_cyc_cnt), .q(r_rq_axi_cyc_cnt));

   // Request AXI FIFO Control
   assign r_axi_rq_pop = r_rq_tx_ready && (r_rq_axi_cyc_cnt[0] == 1'd1);
   

   // Request AXI Packet Output
   assign o_s_axi_tx_tkeep  = (r_rq_tx_ready) ?        8'hFF :  8'b0;
   assign o_s_axi_tx_tdata  = (r_rq_tx_ready) ? r_rq_tx_data : 64'h0;
   assign o_s_axi_tx_tlast  = (r_rq_tx_ready) ? r_rq_tx_last :  1'b0;
   assign o_s_axi_tx_tvalid = r_rq_tx_ready;

   // Hack: stop uio requests from coming in until channel is up?
   assign uio_rq_afull = r_uio_rq_afull | !i_stat_chan_up;
   



   // Response User IO Packet Formation
   // Expecting 512b/64B packets - tlast every 8 valid cycles
   //   NOTE!!!
   //   This design does not expect to need to backpressure AXI at all,
   //     and therefore makes simplifcations based on this assumption.
   //   This also implies that we expect no backpressure from the User
   //     IO port interface.  If your design requires flow control in
   //     this direction, you must account for this.
   //   We also expect that tkeep will always be 0xFF, and there will
   //     be no partial writes on the AXI bus.
   reg  	c_rs_axi_cyc_cnt;
   wire  	r_rs_axi_cyc_cnt;
   reg [127:0] 	c_rs_axi_data;
   wire [127:0] r_rs_axi_data;
   reg  	c_rs_axi_push;
   wire  	r_rs_axi_push;
      
   always @(*) begin
      c_rs_axi_cyc_cnt = r_rs_axi_cyc_cnt;

      c_rs_axi_data = r_rs_axi_data;
      c_rs_axi_push = 1'b0;

      if (i_m_axi_rx_tvalid) begin
	 if (r_rs_axi_cyc_cnt == 1'b0) begin
	    c_rs_axi_data[63:0] = i_m_axi_rx_tdata;
	 end else begin
	    c_rs_axi_data[127:64] = i_m_axi_rx_tdata;
	    c_rs_axi_push = 1'b1;
	 end
	 c_rs_axi_cyc_cnt = ~r_rs_axi_cyc_cnt;
      end
   end
   
   dffr #(.WIDTH(1))   dff_rs_axi_cyc_cnt (.clk(clk), .rst(reset), .d(c_rs_axi_cyc_cnt), .q(r_rs_axi_cyc_cnt));

   dffr #(.WIDTH(128)) dff_rs_axi_data (.clk(clk), .rst(reset), .d(c_rs_axi_data), .q(r_rs_axi_data));
   dffr #(.WIDTH(1))   dff_rs_axi_push (.clk(clk), .rst(reset), .d(c_rs_axi_push), .q(r_rs_axi_push));

   
   // Response User IO Clock Crossing FIFO
   // FIXME: RATIO/SYNC???
   wire 		      r_axi_rs_pop;
   wire 		      r_axi_rs_empty;
   
   fifo #(.WIDTH(UIO_PORTS_WIDTH), .DEPTH(RS_AXI_FIFO_DEPTH), .RATIO(3), .SYNCHRONOUS(0), .OSYNC_RESET(0))
   rs_axi_fifo (
		 .clk(clk),
		 .reset(fifo_reset),
		 .push(r_rs_axi_push),
		 .din(r_rs_axi_data),
		 .afull(),
		 .full(), 
		 .cnt(),
		 .oclk(clk_per),
		 .pop(uio_rs_vld),
		 .dout(uio_rs_data),
		 .empty(r_axi_rs_empty),
		 .rcnt()
   );

   // Response User IO Packet Output
   assign uio_rs_vld = !r_axi_rs_empty;
   
endmodule
