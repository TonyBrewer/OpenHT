 ///////////////////////////////////////////////////////////////////////////////
 //
 // Project:  Aurora 64B/66B
 // Company:  Xilinx
 //
 //
 //
 // (c) Copyright 2008 - 2009 Xilinx, Inc. All rights reserved.
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
 //  TX_LL_CONTROL_SM
 //
 //
 //  Description: This module provides the transmitter state machine
 //               control logic to connect the LocalLink interface to
 //               the Aurora Channel
 //
 //
 //
 //
 ///////////////////////////////////////////////////////////////////////////////
`timescale 1 ps / 1 ps
 
(* DowngradeIPIdentifiedWarnings="yes" *)
 module  aurora_64b66b_25p78G_TX_LL_CONTROL_SM
 (
// S_AXI_TX Interface
     s_axi_tx_tlast,
     s_axi_tx_tkeep,
     s_axi_tx_tvalid,
     s_axi_tx_tready,

 
     
     // Clock Compensation Interface
     DO_CC,
 
     // Global Logic Interface
     CHANNEL_UP,
 
     // TX_LL Control Module Interface
 
 
     // Aurora Lane Interface
     GEN_SEP,
     GEN_SEP7,
     SEP_NB,
     GEN_CC,
 
 
 
 
     // GTX Interface
     TXDATAVALID_IN,
 
     // System Interface
     USER_CLK
 
 );
 
 `define DLY #1
 
 
     parameter  STRB_WIDTH = 8;
 
 //***********************************Port Declarations*******************************
 
 
       input                    s_axi_tx_tvalid; 
       output                   s_axi_tx_tready; 
       input                    s_axi_tx_tlast; 
       input         [0:7]      s_axi_tx_tkeep; 
 
 
 
 
 
     // Clock Compensation Interface
       input                    DO_CC; 
 
 
     // Global Logic Interface
       input                    CHANNEL_UP; 
 
     // TX_LL Control Module Interface
 
     // Aurora Lane Interface
       output                   GEN_SEP; 
       output                   GEN_SEP7; 
       output        [0:2]      SEP_NB; 
       output                   GEN_CC; 
 
     // GTX Interface
       input                    TXDATAVALID_IN; 
     // System Interface
       input                    USER_CLK; 
 
 
 
 //**************************External Register Declarations****************************
 
       reg                      GEN_SEP; 
       reg                      GEN_SEP7; 
       reg           [0:2]      SEP_NB; 
 
 
 
 
 //**************************Internal Register Declarations****************************
 
       reg                      do_cc_r = 1'b0; 
       reg                      do_cc_r2 = 1'b0; 
       reg                      extend_cc_r;
       reg                      gen_cc_r; 
 
     // Data state registers
       reg                     idle_r; 
       reg                     data_r; 
       reg                     partial_data_r; 
       reg                     full_data_r; 
       reg                     tlast_txdv_coincide_r;
 
       reg           [0:2]      gen_sep_nb_r; 
       reg                      gen_sep_r; 
       reg                      gen_sep7_r; 
     
 
 
       reg                      datavalid_in_r; 
       reg                      datavalid_in_r2; 
       reg                      channel_full_r;
       reg                      channel_full_r2;
       reg                      datavalid_pdu;
       reg                      s_axi_tx_tready;
       reg                      found_rem; 
       reg           [0:2]      sep_nb; 
       reg [(STRB_WIDTH-1):0]   i;  
 //*********************************Wire Declarations**********************************
 
       wire                     next_idle_c; 
       wire                     next_data_c; 
       wire                     next_partial_data_c; 
       wire                     next_full_data_c; 
       wire                     s_axi_tx_tready_c; 
 
       wire                     channel_full_c; 
       wire                     pdu_ok_c; 
       wire                     gen_sep_c; 
       wire                     gen_sep7_c; 
       wire                     eof_with_datavalid_c; 
 
       reg           [0:2]      gen_sep_nb_c; 
       wire                      in_frame;
       wire                      sc_frame;
       wire                      ic_frame;
       wire                      wait_for_sep0_tx;
       wire           sep0_lane0;
       wire           sep_lane0;
       reg            sep0_lane0_r;
 //*********************************Main Body of Code**********************************
 
 
     //___________________________Clock Compensation________________________________
 
 
     // DO_CC signal has to be extended for one more cycle when TXDATAVALID_IN  
     // coincides with DO_CC occurance
     // extend_cc_r extends the pulse by one cycle during TXDATAVALID_IN cycle
 
     always @(posedge USER_CLK)
         if(!TXDATAVALID_IN & DO_CC)
               extend_cc_r <=  `DLY    1'b1;
         else if(!DO_CC)
               extend_cc_r <=  `DLY    1'b0;
 
     always @(posedge USER_CLK)
               do_cc_r <=  `DLY    DO_CC | extend_cc_r;
 
     always @(posedge USER_CLK)
               do_cc_r2 <= `DLY do_cc_r;
 
     // Asserted when non single cycle frame is available to transmit
     assign in_frame = (s_axi_tx_tready & s_axi_tx_tvalid & !s_axi_tx_tlast);
     
     // Asserted when single cycle frame is available to transmit
     assign sc_frame = (s_axi_tx_tready & s_axi_tx_tvalid & s_axi_tx_tlast);
 
     // Asserted for invalid cycle 
    //assign ic_frame = (!s_axi_tx_tvalid);
    assign ic_frame = !(s_axi_tx_tvalid && s_axi_tx_tready);

     // The aurora channel is 'full' if there is more than enough data to fit into
     assign  channel_full_c          =   (s_axi_tx_tlast & (s_axi_tx_tkeep == {8{1'b1}}) & s_axi_tx_tvalid);

     assign  pdu_ok_c                =  CHANNEL_UP & !(DO_CC | do_cc_r) & (TXDATAVALID_IN & datavalid_pdu);//(TXDATAVALID_IN & datavalid_in_r & datavalid_in_r2); 
     
 
     //_____________________________Control State Machine__________________________________
 
     // The Control state machine handles the encapsulation and transmission of user
     // data.  It can use the channel when there is no CC, NFC message, UFC header,
     // UFC message or remote NFC request. 
 
     // State Registers
     always @(posedge USER_CLK)
         if(!CHANNEL_UP)
         begin
             idle_r              <=  `DLY    1'b1;
             data_r              <=  `DLY    1'b0;
             partial_data_r      <=  `DLY    1'b0; 
             full_data_r         <=  `DLY    1'b0; 
         end
         else if(pdu_ok_c)
         begin
             idle_r              <=  `DLY    next_idle_c;
             data_r              <=  `DLY    next_data_c;
             partial_data_r      <=  `DLY    next_partial_data_c; 
             full_data_r         <=  `DLY    next_full_data_c   ; 
         end
 
 
     // Next State Logic
 
     // Default state. Remains in idle_r till s_axi_tx_tvalid is asserted along with
     // When no frame is available to the channel, the states return back to 
     // this state
     assign  next_idle_c             =   ((idle_r & ic_frame) |
                                         (data_r & ic_frame) |
                                         (partial_data_r & ic_frame) |
                                         (full_data_r & ic_frame)) & !wait_for_sep0_tx ;
 
     // data_r state is set when s_axi_tx_tvalid from TX AXI4-Stream interface is asserted
     // and s_axi_tx_tlast is not asserted 
     assign  next_data_c             =   ((idle_r & in_frame) |
                                         (data_r & in_frame) |
                                         (partial_data_r & in_frame) |
                                         (full_data_r & in_frame)) & !wait_for_sep0_tx;
 
 
     // partial_data_r state is set for last packet data which is partial data
     // or single cycle packet with partially filled data  
     assign  next_partial_data_c     =   ((idle_r & sc_frame & !channel_full_c) |
                                         (data_r & sc_frame & !channel_full_c) |
                                         (full_data_r & sc_frame & !channel_full_c) |
                                         (partial_data_r & sc_frame & !channel_full_c)) & !wait_for_sep0_tx;
 
 
     // full_data_r state is set for last packet data which is full data
     // or single cycle packet with full data  
     assign  next_full_data_c        =   (idle_r & sc_frame & channel_full_c) |
                                         (data_r & sc_frame & channel_full_c) |
                                         (partial_data_r & sc_frame & channel_full_c) | 
                                         (full_data_r & sc_frame & channel_full_c) | 
                                         wait_for_sep0_tx;
 

     assign wait_for_sep0_tx = (sep0_lane0_r & !full_data_r)|(full_data_r & sep0_lane0_r & !GEN_SEP);
 
 

 
     // TXDATAVALID_IN signal is registered for delay in the data flow 
     always @(posedge USER_CLK)
     begin
           datavalid_in_r     <=  `DLY  TXDATAVALID_IN;
           datavalid_in_r2    <=  `DLY  datavalid_in_r;
           channel_full_r  <= `DLY    channel_full_c;
           channel_full_r2 <= `DLY    channel_full_r ;
           datavalid_pdu   <= `DLY  !(!datavalid_in_r & datavalid_in_r2 & channel_full_r2);
     end

     //  s_axi_tx_tready_c  is the critical path in this module.  It must be deasserted 
     // whenever an event occurs that prevents the pdu state machine from using the
     // Aurora channel to transmit PDUs.
     assign  s_axi_tx_tready_c  =  (pdu_ok_c & !(sep0_lane0 & s_axi_tx_tready) & !DO_CC & !wait_for_sep0_tx


                                   );

     always @(posedge USER_CLK)
         if(!CHANNEL_UP)     s_axi_tx_tready    <=  `DLY    1'b0;
         else                s_axi_tx_tready    <=  `DLY    s_axi_tx_tready_c & TXDATAVALID_IN;

 
 
     // Drive the GEN_SEP/GEN_SEP7 signal when in an EOF state with the PDU state machine active.
     always @(posedge USER_CLK)
     begin
         if(!CHANNEL_UP)
         begin     
           GEN_SEP      <=  `DLY  {1{1'b0}};
           GEN_SEP7     <=  `DLY  {1{1'b0}};
         end
         else if((gen_sep_c == {1{1'b1}}) & !datavalid_in_r)
            GEN_SEP     <=  `DLY  gen_sep_c;
         else if((gen_sep_r == {1{1'b1}}) & !datavalid_in_r2)
            GEN_SEP     <=  `DLY  gen_sep_r;
     // Additional states to latch GEN_SEP when datavalid  & CC coincides
         else if((gen_sep_r == {1{1'b1}}) & |GEN_CC)
            GEN_SEP     <=  `DLY  gen_sep_r;
         else if((|GEN_SEP) & ( (|GEN_CC)))
            GEN_SEP     <=  `DLY    GEN_SEP;
         else if ((gen_sep7_c == {1{1'b1}}) & !datavalid_in_r)
            GEN_SEP7    <=  `DLY gen_sep7_c;
         else if ((gen_sep7_r == {1{1'b1}}) & !datavalid_in_r2)
            GEN_SEP7    <=  `DLY gen_sep7_r;
         else if(!datavalid_in_r2 & (|GEN_SEP))
            GEN_SEP     <=  `DLY    GEN_SEP;
         else if(!datavalid_in_r2 & (|GEN_SEP7))
            GEN_SEP7     <=  `DLY    GEN_SEP7;
         else
         begin
            GEN_SEP     <=  `DLY    gen_sep_c;
            GEN_SEP7    <=  `DLY    gen_sep7_c;
         end
     end

   //  // We generate the gen_sep_nb_c signal based on the REM signal and the EOF signal.
     generate
       genvar b;
       for (b=0; b<1; b=b+1) begin: gen_sep_nb
 
       	  always @(s_axi_tx_tkeep[b*8:b*8+7] or full_data_r or sep0_lane0_r)
       	      if (full_data_r || sep0_lane0_r)
       	      	  gen_sep_nb_c[b*3:b*3+2] = 3'd0;
       	      else
       	      case (s_axi_tx_tkeep[b*8:b*8+7])
       	          8'b10000000 : gen_sep_nb_c[b*3:b*3+2] = 3'd1; 
       	          8'b11000000 : gen_sep_nb_c[b*3:b*3+2] = 3'd2; 
       	          8'b11100000 : gen_sep_nb_c[b*3:b*3+2] = 3'd3; 
       	          8'b11110000 : gen_sep_nb_c[b*3:b*3+2] = 3'd4; 
       	          8'b11111000 : gen_sep_nb_c[b*3:b*3+2] = 3'd5; 
       	          8'b11111100 : gen_sep_nb_c[b*3:b*3+2] = 3'd6; 
       	          8'b11111110 : gen_sep_nb_c[b*3:b*3+2] = 3'd7; 
       	          8'b11111111 : gen_sep_nb_c[b*3:b*3+2] = 3'd0; 
       	          default     : gen_sep_nb_c[b*3:b*3+2] = 3'd0; 
              endcase
     end //gen_sep_nb
     endgenerate



     // logic to extend sep0_lane0 when tlast coincides with TXDV
     always @(posedge USER_CLK)
         if(!CHANNEL_UP) 
            tlast_txdv_coincide_r <= `DLY 1'b0;
         else if(channel_full_c & !TXDATAVALID_IN)
            tlast_txdv_coincide_r <= `DLY 1'b1;
         else
            tlast_txdv_coincide_r <= `DLY 1'b0;

     always @(posedge USER_CLK)
     begin
       if (sep0_lane0_r  & !pdu_ok_c)
         sep0_lane0_r <= `DLY 1'b1;
       else
         sep0_lane0_r <= `DLY sep0_lane0;
     end

     assign sep0_lane0 = (CHANNEL_UP & next_full_data_c & !tlast_txdv_coincide_r & s_axi_tx_tready);

     assign sep_lane0  = (CHANNEL_UP & next_partial_data_c & (s_axi_tx_tkeep > 8'h00) & (s_axi_tx_tkeep < 8'hFE) & s_axi_tx_tready); 

     assign gen_sep_c  =  (((s_axi_tx_tready & sep_lane0) | (pdu_ok_c & !s_axi_tx_tready & sep0_lane0_r)));
 
     assign gen_sep7_c = (next_partial_data_c & s_axi_tx_tready & (s_axi_tx_tkeep == 8'hFE));
 
 
     always @(posedge USER_CLK)
     begin
         if(datavalid_in_r & datavalid_in_r2 & !(|GEN_CC))
             gen_sep_r   <= `DLY 1'b0;
         else if(!datavalid_in_r)
             gen_sep_r   <= `DLY gen_sep_c;
     end
 
     always @(posedge USER_CLK)
     begin
             gen_sep_nb_r <= `DLY gen_sep_nb_c;
     end
 
     always @(posedge USER_CLK)
     begin
         if(!datavalid_in_r2)
             gen_sep7_r   <= `DLY gen_sep7_r;
         else
             gen_sep7_r   <= `DLY gen_sep7_c;
     end
 
 
     // Assign gen_sep_nb to SEP_NB port
     always @(posedge USER_CLK)
     begin
         if(!datavalid_in_r)
             SEP_NB   <=  `DLY    gen_sep_nb_r;
         else
             SEP_NB   <=  `DLY    gen_sep_nb_c;
     end      
 
 
     // The flops for the GEN_CC signal are replicated for timing and instantiated to allow us
     // to set their value reliably on powerup.
     FDR gen_cc_flop
     (
         .D(do_cc_r),
         .C(USER_CLK),
         .R(~CHANNEL_UP),
         .Q(GEN_CC)
     );
 
 
 
  
 
 
 
 endmodule
 
