`timescale 1 ns / 1 ps

module user_io_sim_bbox #
(
   parameter NUM_UIO_PORTS   = 32,
   parameter UIO_PORTS_WIDTH = 32
)(
   input  		clk_per,
   input 		reset_per,

   // csr ports
   input  [15:0]        i_csr_addr,
   input  [63:0]        i_csr_data,
   input 		i_csr_wr_vld,
   input 		i_csr_rd_vld,

   output [63:0]        o_csr_data,
   output		o_csr_rd_ack,

   // user-facing ports
   input  [NUM_UIO_PORTS*1-1 :0] uio_rq_vld,
   input  [NUM_UIO_PORTS*UIO_PORTS_WIDTH-1:0] uio_rq_data,
   output [NUM_UIO_PORTS*1-1 :0] uio_rq_afull,

   output [NUM_UIO_PORTS*1-1 :0] uio_rs_vld,
   output [NUM_UIO_PORTS*UIO_PORTS_WIDTH-1:0] uio_rs_data,
   input  [NUM_UIO_PORTS*1-1 :0] uio_rs_afull
);

   // 1 AE / 8 PORTS
   wire 			uio_p0_rq_vld,   uio_p1_rq_vld,   uio_p2_rq_vld,   uio_p3_rq_vld;
   wire 			uio_p4_rq_vld,   uio_p5_rq_vld,   uio_p6_rq_vld,   uio_p7_rq_vld;
   wire [UIO_PORTS_WIDTH-1:0]	uio_p0_rq_data,  uio_p1_rq_data,  uio_p2_rq_data,  uio_p3_rq_data;
   wire [UIO_PORTS_WIDTH-1:0]	uio_p4_rq_data,  uio_p5_rq_data,  uio_p6_rq_data,  uio_p7_rq_data;
   wire 			uio_p0_rq_afull, uio_p1_rq_afull, uio_p2_rq_afull, uio_p3_rq_afull;
   wire 			uio_p4_rq_afull, uio_p5_rq_afull, uio_p6_rq_afull, uio_p7_rq_afull;
   wire 			uio_p0_rs_vld,   uio_p1_rs_vld,   uio_p2_rs_vld,   uio_p3_rs_vld;
   wire 			uio_p4_rs_vld,   uio_p5_rs_vld,   uio_p6_rs_vld,   uio_p7_rs_vld;
   wire [UIO_PORTS_WIDTH-1:0]	uio_p0_rs_data,  uio_p1_rs_data,  uio_p2_rs_data,  uio_p3_rs_data;
   wire [UIO_PORTS_WIDTH-1:0]	uio_p4_rs_data,  uio_p5_rs_data,  uio_p6_rs_data,  uio_p7_rs_data;
   wire 			uio_p0_rs_afull, uio_p1_rs_afull, uio_p2_rs_afull, uio_p3_rs_afull;
   wire 			uio_p4_rs_afull, uio_p5_rs_afull, uio_p6_rs_afull, uio_p7_rs_afull;

   assign uio_p0_rq_vld   = uio_rq_vld[0];
   assign uio_p0_rq_data  = uio_rq_data[1*UIO_PORTS_WIDTH-1:0*UIO_PORTS_WIDTH];
   assign uio_rq_afull[0] = uio_p0_rq_afull;
   
   assign uio_p1_rq_vld   = uio_rq_vld[1];
   assign uio_p1_rq_data  = uio_rq_data[2*UIO_PORTS_WIDTH-1:1*UIO_PORTS_WIDTH];
   assign uio_rq_afull[1] = uio_p1_rq_afull;
   
   assign uio_p2_rq_vld   = uio_rq_vld[2];
   assign uio_p2_rq_data  = uio_rq_data[3*UIO_PORTS_WIDTH-1:2*UIO_PORTS_WIDTH];
   assign uio_rq_afull[2] = uio_p2_rq_afull;
   
   assign uio_p3_rq_vld   = uio_rq_vld[3];
   assign uio_p3_rq_data  = uio_rq_data[4*UIO_PORTS_WIDTH-1:3*UIO_PORTS_WIDTH];
   assign uio_rq_afull[3] = uio_p3_rq_afull;

   assign uio_p4_rq_vld   = uio_rq_vld[4];
   assign uio_p4_rq_data  = uio_rq_data[5*UIO_PORTS_WIDTH-1:4*UIO_PORTS_WIDTH];
   assign uio_rq_afull[4] = uio_p4_rq_afull;
   
   assign uio_p5_rq_vld   = uio_rq_vld[5];
   assign uio_p5_rq_data  = uio_rq_data[6*UIO_PORTS_WIDTH-1:5*UIO_PORTS_WIDTH];
   assign uio_rq_afull[5] = uio_p5_rq_afull;
   
   assign uio_p6_rq_vld   = uio_rq_vld[6];
   assign uio_p6_rq_data  = uio_rq_data[7*UIO_PORTS_WIDTH-1:6*UIO_PORTS_WIDTH];
   assign uio_rq_afull[6] = uio_p6_rq_afull;
   
   assign uio_p7_rq_vld   = uio_rq_vld[7];
   assign uio_p7_rq_data  = uio_rq_data[8*UIO_PORTS_WIDTH-1:7*UIO_PORTS_WIDTH];
   assign uio_rq_afull[7] = uio_p7_rq_afull;

   assign uio_rs_vld[0]                                      = uio_p0_rs_vld;
   assign uio_rs_data[1*UIO_PORTS_WIDTH-1:0*UIO_PORTS_WIDTH] = uio_p0_rs_data;
   assign uio_p0_rs_afull                                    = uio_rs_afull[0];
   
   assign uio_rs_vld[1]                                      = uio_p1_rs_vld;
   assign uio_rs_data[2*UIO_PORTS_WIDTH-1:1*UIO_PORTS_WIDTH] = uio_p1_rs_data;
   assign uio_p1_rs_afull                                    = uio_rs_afull[1];
   
   assign uio_rs_vld[2]                                      = uio_p2_rs_vld;
   assign uio_rs_data[3*UIO_PORTS_WIDTH-1:2*UIO_PORTS_WIDTH] = uio_p2_rs_data;
   assign uio_p2_rs_afull                                    = uio_rs_afull[2];
   
   assign uio_rs_vld[3]                                      = uio_p3_rs_vld;
   assign uio_rs_data[4*UIO_PORTS_WIDTH-1:3*UIO_PORTS_WIDTH] = uio_p3_rs_data;
   assign uio_p3_rs_afull                                    = uio_rs_afull[3];

   assign uio_rs_vld[4]                                      = uio_p4_rs_vld;
   assign uio_rs_data[5*UIO_PORTS_WIDTH-1:4*UIO_PORTS_WIDTH] = uio_p4_rs_data;
   assign uio_p4_rs_afull                                    = uio_rs_afull[4];
   
   assign uio_rs_vld[5]                                      = uio_p5_rs_vld;
   assign uio_rs_data[6*UIO_PORTS_WIDTH-1:5*UIO_PORTS_WIDTH] = uio_p5_rs_data;
   assign uio_p5_rs_afull                                    = uio_rs_afull[5];
   
   assign uio_rs_vld[6]                                      = uio_p6_rs_vld;
   assign uio_rs_data[7*UIO_PORTS_WIDTH-1:6*UIO_PORTS_WIDTH] = uio_p6_rs_data;
   assign uio_p6_rs_afull                                    = uio_rs_afull[6];
   
   assign uio_rs_vld[7]                                      = uio_p7_rs_vld;
   assign uio_rs_data[8*UIO_PORTS_WIDTH-1:7*UIO_PORTS_WIDTH] = uio_p7_rs_data;
   assign uio_p7_rs_afull                                    = uio_rs_afull[7];
   

   // Port 0 Loop
   wire 			uio_p0_mid_empty;
   wire 			uio_p0_mid_afull;
   wire 			uio_p0_mid_xfer = !uio_p0_mid_empty & !uio_p0_mid_afull;
   wire [UIO_PORTS_WIDTH-1:0]	uio_p0_mid_data;
   fifo #(
	  .WIDTH(UIO_PORTS_WIDTH),
	  .DEPTH(64),
	  .AFULLCNT(64-6), // 6 required for adequate time to reach pers logic
	  .RAM_STYLE("block") //"distributed"/"block"
	  )
   fifo_rq_p0 (
	       .reset(reset_per),

	       // PUSH
	       .clk(clk_per),
	       .push(uio_p0_rq_vld),
	       .din(uio_p0_rq_data),

	       // POP
	       .oclk(clk_per),
	       .pop(uio_p0_mid_xfer),
	       .dout(uio_p0_mid_data),

	       // STATUS
	       .afull(uio_p0_rq_afull),
	       .empty(uio_p0_mid_empty),
      
	       // UNUSED
	       .full(),
	       .cnt(),
	       .rcnt()
	       );
   
   wire 			uio_p0_rs_empty;
   assign uio_p0_rs_vld = !uio_p0_rs_empty & !uio_p0_rs_afull;
   fifo #(
	  .WIDTH(UIO_PORTS_WIDTH),
	  .DEPTH(64),
	  .AFULLCNT(64-6),
	  .RAM_STYLE("block") //"distributed"/"block"
	  )
   fifo_rs_p0 (
	       .reset(reset_per),

	       // PUSH
	       .clk(clk_per),
	       .push(uio_p0_mid_xfer),
	       .din(uio_p0_mid_data),

	       // POP
	       .oclk(clk_per),
	       .pop(uio_p0_rs_vld),
	       .dout(uio_p0_rs_data),

	       // STATUS
	       .afull(uio_p0_mid_afull),
	       .empty(uio_p0_rs_empty),
      
	       // UNUSED
	       .full(),
	       .cnt(),
	       .rcnt()
	       );

   // Port 1 Loop
   wire 			uio_p1_mid_empty;
   wire 			uio_p1_mid_afull;
   wire 			uio_p1_mid_xfer = !uio_p1_mid_empty & !uio_p1_mid_afull;
   wire [UIO_PORTS_WIDTH-1:0]	uio_p1_mid_data;
   fifo #(
	  .WIDTH(UIO_PORTS_WIDTH),
	  .DEPTH(64),
	  .AFULLCNT(64-6), // 6 required for adequate time to reach pers logic
	  .RAM_STYLE("block") //"distributed"/"block"
	  )
   fifo_rq_p1 (
	       .reset(reset_per),

	       // PUSH
	       .clk(clk_per),
	       .push(uio_p1_rq_vld),
	       .din(uio_p1_rq_data),

	       // POP
	       .oclk(clk_per),
	       .pop(uio_p1_mid_xfer),
	       .dout(uio_p1_mid_data),

	       // STATUS
	       .afull(uio_p1_rq_afull),
	       .empty(uio_p1_mid_empty),
      
	       // UNUSED
	       .full(),
	       .cnt(),
	       .rcnt()
	       );
   
   wire 			uio_p1_rs_empty;
   assign uio_p1_rs_vld = !uio_p1_rs_empty & !uio_p1_rs_afull;
   fifo #(
	  .WIDTH(UIO_PORTS_WIDTH),
	  .DEPTH(64),
	  .AFULLCNT(64-6),
	  .RAM_STYLE("block") //"distributed"/"block"
	  )
   fifo_rs_p1 (
	       .reset(reset_per),

	       // PUSH
	       .clk(clk_per),
	       .push(uio_p1_mid_xfer),
	       .din(uio_p1_mid_data),

	       // POP
	       .oclk(clk_per),
	       .pop(uio_p1_rs_vld),
	       .dout(uio_p1_rs_data),

	       // STATUS
	       .afull(uio_p1_mid_afull),
	       .empty(uio_p1_rs_empty),
      
	       // UNUSED
	       .full(),
	       .cnt(),
	       .rcnt()
	       );

   // Port 2 Loop
   wire 			uio_p2_mid_empty;
   wire 			uio_p2_mid_afull;
   wire 			uio_p2_mid_xfer = !uio_p2_mid_empty & !uio_p2_mid_afull;
   wire [UIO_PORTS_WIDTH-1:0]	uio_p2_mid_data;
   fifo #(
	  .WIDTH(UIO_PORTS_WIDTH),
	  .DEPTH(64),
	  .AFULLCNT(64-6), // 6 required for adequate time to reach pers logic
	  .RAM_STYLE("block") //"distributed"/"block"
	  )
   fifo_rq_p2 (
	       .reset(reset_per),

	       // PUSH
	       .clk(clk_per),
	       .push(uio_p2_rq_vld),
	       .din(uio_p2_rq_data),

	       // POP
	       .oclk(clk_per),
	       .pop(uio_p2_mid_xfer),
	       .dout(uio_p2_mid_data),

	       // STATUS
	       .afull(uio_p2_rq_afull),
	       .empty(uio_p2_mid_empty),
      
	       // UNUSED
	       .full(),
	       .cnt(),
	       .rcnt()
	       );
   
   wire 			uio_p2_rs_empty;
   assign uio_p2_rs_vld = !uio_p2_rs_empty & !uio_p2_rs_afull;
   fifo #(
	  .WIDTH(UIO_PORTS_WIDTH),
	  .DEPTH(64),
	  .AFULLCNT(64-6),
	  .RAM_STYLE("block") //"distributed"/"block"
	  )
   fifo_rs_p2 (
	       .reset(reset_per),

	       // PUSH
	       .clk(clk_per),
	       .push(uio_p2_mid_xfer),
	       .din(uio_p2_mid_data),

	       // POP
	       .oclk(clk_per),
	       .pop(uio_p2_rs_vld),
	       .dout(uio_p2_rs_data),

	       // STATUS
	       .afull(uio_p2_mid_afull),
	       .empty(uio_p2_rs_empty),
      
	       // UNUSED
	       .full(),
	       .cnt(),
	       .rcnt()
	       );

   // Port 3 Loop
   wire 			uio_p3_mid_empty;
   wire 			uio_p3_mid_afull;
   wire 			uio_p3_mid_xfer = !uio_p3_mid_empty & !uio_p3_mid_afull;
   wire [UIO_PORTS_WIDTH-1:0]	uio_p3_mid_data;
   fifo #(
	  .WIDTH(UIO_PORTS_WIDTH),
	  .DEPTH(64),
	  .AFULLCNT(64-6), // 6 required for adequate time to reach pers logic
	  .RAM_STYLE("block") //"distributed"/"block"
	  )
   fifo_rq_p3 (
	       .reset(reset_per),

	       // PUSH
	       .clk(clk_per),
	       .push(uio_p3_rq_vld),
	       .din(uio_p3_rq_data),

	       // POP
	       .oclk(clk_per),
	       .pop(uio_p3_mid_xfer),
	       .dout(uio_p3_mid_data),

	       // STATUS
	       .afull(uio_p3_rq_afull),
	       .empty(uio_p3_mid_empty),
      
	       // UNUSED
	       .full(),
	       .cnt(),
	       .rcnt()
	       );
   
   wire 			uio_p3_rs_empty;
   assign uio_p3_rs_vld = !uio_p3_rs_empty & !uio_p3_rs_afull;
   fifo #(
	  .WIDTH(UIO_PORTS_WIDTH),
	  .DEPTH(64),
	  .AFULLCNT(64-6),
	  .RAM_STYLE("block") //"distributed"/"block"
	  )
   fifo_rs_p3 (
	       .reset(reset_per),

	       // PUSH
	       .clk(clk_per),
	       .push(uio_p3_mid_xfer),
	       .din(uio_p3_mid_data),

	       // POP
	       .oclk(clk_per),
	       .pop(uio_p3_rs_vld),
	       .dout(uio_p3_rs_data),

	       // STATUS
	       .afull(uio_p3_mid_afull),
	       .empty(uio_p3_rs_empty),
      
	       // UNUSED
	       .full(),
	       .cnt(),
	       .rcnt()
	       );

   // Port 4 Loop
   wire 			uio_p4_mid_empty;
   wire 			uio_p4_mid_afull;
   wire 			uio_p4_mid_xfer = !uio_p4_mid_empty & !uio_p4_mid_afull;
   wire [UIO_PORTS_WIDTH-1:0]	uio_p4_mid_data;
   fifo #(
	  .WIDTH(UIO_PORTS_WIDTH),
	  .DEPTH(64),
	  .AFULLCNT(64-6), // 6 required for adequate time to reach pers logic
	  .RAM_STYLE("block") //"distributed"/"block"
	  )
   fifo_rq_p4 (
	       .reset(reset_per),

	       // PUSH
	       .clk(clk_per),
	       .push(uio_p4_rq_vld),
	       .din(uio_p4_rq_data),

	       // POP
	       .oclk(clk_per),
	       .pop(uio_p4_mid_xfer),
	       .dout(uio_p4_mid_data),

	       // STATUS
	       .afull(uio_p4_rq_afull),
	       .empty(uio_p4_mid_empty),
      
	       // UNUSED
	       .full(),
	       .cnt(),
	       .rcnt()
	       );
   
   wire 			uio_p4_rs_empty;
   assign uio_p4_rs_vld = !uio_p4_rs_empty & !uio_p4_rs_afull;
   fifo #(
	  .WIDTH(UIO_PORTS_WIDTH),
	  .DEPTH(64),
	  .AFULLCNT(64-6),
	  .RAM_STYLE("block") //"distributed"/"block"
	  )
   fifo_rs_p4 (
	       .reset(reset_per),

	       // PUSH
	       .clk(clk_per),
	       .push(uio_p4_mid_xfer),
	       .din(uio_p4_mid_data),

	       // POP
	       .oclk(clk_per),
	       .pop(uio_p4_rs_vld),
	       .dout(uio_p4_rs_data),

	       // STATUS
	       .afull(uio_p4_mid_afull),
	       .empty(uio_p4_rs_empty),
      
	       // UNUSED
	       .full(),
	       .cnt(),
	       .rcnt()
	       );

   // Port 5 Loop
   wire 			uio_p5_mid_empty;
   wire 			uio_p5_mid_afull;
   wire 			uio_p5_mid_xfer = !uio_p5_mid_empty & !uio_p5_mid_afull;
   wire [UIO_PORTS_WIDTH-1:0]	uio_p5_mid_data;
   fifo #(
	  .WIDTH(UIO_PORTS_WIDTH),
	  .DEPTH(64),
	  .AFULLCNT(64-6), // 6 required for adequate time to reach pers logic
	  .RAM_STYLE("block") //"distributed"/"block"
	  )
   fifo_rq_p5 (
	       .reset(reset_per),

	       // PUSH
	       .clk(clk_per),
	       .push(uio_p5_rq_vld),
	       .din(uio_p5_rq_data),

	       // POP
	       .oclk(clk_per),
	       .pop(uio_p5_mid_xfer),
	       .dout(uio_p5_mid_data),

	       // STATUS
	       .afull(uio_p5_rq_afull),
	       .empty(uio_p5_mid_empty),
      
	       // UNUSED
	       .full(),
	       .cnt(),
	       .rcnt()
	       );
   
   wire 			uio_p5_rs_empty;
   assign uio_p5_rs_vld = !uio_p5_rs_empty & !uio_p5_rs_afull;
   fifo #(
	  .WIDTH(UIO_PORTS_WIDTH),
	  .DEPTH(64),
	  .AFULLCNT(64-6),
	  .RAM_STYLE("block") //"distributed"/"block"
	  )
   fifo_rs_p5 (
	       .reset(reset_per),

	       // PUSH
	       .clk(clk_per),
	       .push(uio_p5_mid_xfer),
	       .din(uio_p5_mid_data),

	       // POP
	       .oclk(clk_per),
	       .pop(uio_p5_rs_vld),
	       .dout(uio_p5_rs_data),

	       // STATUS
	       .afull(uio_p5_mid_afull),
	       .empty(uio_p5_rs_empty),
      
	       // UNUSED
	       .full(),
	       .cnt(),
	       .rcnt()
	       );

   // Port 6 Loop
   wire 			uio_p6_mid_empty;
   wire 			uio_p6_mid_afull;
   wire 			uio_p6_mid_xfer = !uio_p6_mid_empty & !uio_p6_mid_afull;
   wire [UIO_PORTS_WIDTH-1:0]	uio_p6_mid_data;
   fifo #(
	  .WIDTH(UIO_PORTS_WIDTH),
	  .DEPTH(64),
	  .AFULLCNT(64-6), // 6 required for adequate time to reach pers logic
	  .RAM_STYLE("block") //"distributed"/"block"
	  )
   fifo_rq_p6 (
	       .reset(reset_per),

	       // PUSH
	       .clk(clk_per),
	       .push(uio_p6_rq_vld),
	       .din(uio_p6_rq_data),

	       // POP
	       .oclk(clk_per),
	       .pop(uio_p6_mid_xfer),
	       .dout(uio_p6_mid_data),

	       // STATUS
	       .afull(uio_p6_rq_afull),
	       .empty(uio_p6_mid_empty),
      
	       // UNUSED
	       .full(),
	       .cnt(),
	       .rcnt()
	       );
   
   wire 			uio_p6_rs_empty;
   assign uio_p6_rs_vld = !uio_p6_rs_empty & !uio_p6_rs_afull;
   fifo #(
	  .WIDTH(UIO_PORTS_WIDTH),
	  .DEPTH(64),
	  .AFULLCNT(64-6),
	  .RAM_STYLE("block") //"distributed"/"block"
	  )
   fifo_rs_p6 (
	       .reset(reset_per),

	       // PUSH
	       .clk(clk_per),
	       .push(uio_p6_mid_xfer),
	       .din(uio_p6_mid_data),

	       // POP
	       .oclk(clk_per),
	       .pop(uio_p6_rs_vld),
	       .dout(uio_p6_rs_data),

	       // STATUS
	       .afull(uio_p6_mid_afull),
	       .empty(uio_p6_rs_empty),
      
	       // UNUSED
	       .full(),
	       .cnt(),
	       .rcnt()
	       );

   // Port 7 Loop
   wire 			uio_p7_mid_empty;
   wire 			uio_p7_mid_afull;
   wire 			uio_p7_mid_xfer = !uio_p7_mid_empty & !uio_p7_mid_afull;
   wire [UIO_PORTS_WIDTH-1:0]	uio_p7_mid_data;
   fifo #(
	  .WIDTH(UIO_PORTS_WIDTH),
	  .DEPTH(64),
	  .AFULLCNT(64-6), // 6 required for adequate time to reach pers logic
	  .RAM_STYLE("block") //"distributed"/"block"
	  )
   fifo_rq_p7 (
	       .reset(reset_per),

	       // PUSH
	       .clk(clk_per),
	       .push(uio_p7_rq_vld),
	       .din(uio_p7_rq_data),

	       // POP
	       .oclk(clk_per),
	       .pop(uio_p7_mid_xfer),
	       .dout(uio_p7_mid_data),

	       // STATUS
	       .afull(uio_p7_rq_afull),
	       .empty(uio_p7_mid_empty),
      
	       // UNUSED
	       .full(),
	       .cnt(),
	       .rcnt()
	       );
   
   wire 			uio_p7_rs_empty;
   assign uio_p7_rs_vld = !uio_p7_rs_empty & !uio_p7_rs_afull;
   fifo #(
	  .WIDTH(UIO_PORTS_WIDTH),
	  .DEPTH(64),
	  .AFULLCNT(64-6),
	  .RAM_STYLE("block") //"distributed"/"block"
	  )
   fifo_rs_p7 (
	       .reset(reset_per),

	       // PUSH
	       .clk(clk_per),
	       .push(uio_p7_mid_xfer),
	       .din(uio_p7_mid_data),

	       // POP
	       .oclk(clk_per),
	       .pop(uio_p7_rs_vld),
	       .dout(uio_p7_rs_data),

	       // STATUS
	       .afull(uio_p7_mid_afull),
	       .empty(uio_p7_rs_empty),
      
	       // UNUSED
	       .full(),
	       .cnt(),
	       .rcnt()
	       );

   
   // Status Link (back to personality)
   //   This link is unique in that it doesn't use the flow control
   //   pieces of the link, and is intended to be a continuous flow
   //   of information back to the personality.  For this reason,
   //   we're just using a few registers to capture the status
   //   (which should be stable for many clocks)

   sync2 #(.WIDTH(8*4))
   status_rs_p9 (
		 .clk(clk_per),
		 //.d({qsfp_fatal_alarm, qsfp_corr_alarm, stat_chan_up, stat_lane_up}),
		 .d({8'b0, 8'b0, 8'hFF, 8'hFF}),
		 .q(uio_rs_data[1055:1024])
		 );

   assign uio_rs_vld[8] = ~reset_per;
   

   // CSR Logic   
   reg 		c_rd_ack;
   wire 	r_rd_ack;
   reg [63:0] 	c_rd_data;
   wire [63:0] 	r_rd_data;
   
   reg [63:0] 	c_reg_0;
   wire [63:0] 	r_reg_0;
   
   reg [63:0] 	c_reg_1;
   wire [63:0] 	r_reg_1;

   localparam CSR_SCRATCH_REG0 = 16'h0000;
   localparam CSR_SCRATCH_REG1 = 16'h0008;
   
   
   always @(*) begin
      c_rd_ack  = 1'b0;
      c_rd_data = 64'b0;
            
      c_reg_0 = r_reg_0;
      c_reg_1 = r_reg_1;

      case(i_csr_addr)

	CSR_SCRATCH_REG0: begin
	   c_rd_ack  = i_csr_rd_vld;
	   c_rd_data = r_reg_0;

	   c_reg_0 = i_csr_wr_vld ? i_csr_data : r_reg_0;
	end

	CSR_SCRATCH_REG1: begin
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

   dff_aclr #(.WIDTH(64)) dff_bbox_csr_reg0 (.clk(clk_per), .clr(reset_per), .d(c_reg_0), .q(r_reg_0));
   dff_aclr #(.WIDTH(64)) dff_bbox_csr_reg1 (.clk(clk_per), .clr(reset_per), .d(c_reg_1), .q(r_reg_1));

   assign o_csr_rd_ack = r_rd_ack;
   assign o_csr_data   = r_rd_data;
   
   
endmodule