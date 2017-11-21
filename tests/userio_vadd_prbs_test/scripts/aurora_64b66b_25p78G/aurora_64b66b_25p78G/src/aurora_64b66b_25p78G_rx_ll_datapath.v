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
 //
 //  RX_LL_DATAPATH
 //
 //
 //  Description: the RX_LL_DATAPATH module takes regular data in Aurora format
 //               and transforms it to LocalLink formatted data
 //
 //              
 //
 
`timescale 1 ps / 1 ps
 
(* DowngradeIPIdentifiedWarnings="yes" *)
 module aurora_64b66b_25p78G_RX_LL_DATAPATH
 (
 
     //Aurora Lane Interface
     RX_PE_DATA,
     RX_PE_DATA_V,
 
     RX_SEP,
     RX_SEP7,
     RX_SEP_NB,
 
     RX_CC,           
     RXDATAVALID_TO_LL,           
     RX_IDLE,           
 
 
     // Global logic interface
     CHANNEL_UP,
 
     //AXI4-Stream Interface
     m_axi_rx_tdata,
     m_axi_rx_tvalid,
     m_axi_rx_tkeep,
     m_axi_rx_tlast,
     
     //System Interface
     USER_CLK,
     RESET
 );
 
 `define DLY #1
  parameter            STRB_WIDTH         = 8; // STROBE bus width
  parameter            REM_WIDTH          = 3; // REM bus width
 
 //***********************************Port Declarations*******************************
     
    
     //Aurora Lane Interface
       input     [0:63]     RX_PE_DATA;  
       input                RX_PE_DATA_V; 
       input                RX_SEP; 
       input                RX_SEP7; 
       input     [0:2]      RX_SEP_NB;         
        
     //AXI4-Stream Interface
       output [0:63]     m_axi_rx_tdata; 
       output            m_axi_rx_tvalid;
       output [0:7]      m_axi_rx_tkeep; 
       output            m_axi_rx_tlast; 
     
     
       input                RX_CC;           
       input                RXDATAVALID_TO_LL;           
       input                RX_IDLE;           
 
 
     //Global Interface
       input                CHANNEL_UP; 
     
     //System Interface
       input                USER_CLK; 
       input                RESET; 
 
 
     
 //****************************Parameter Declarations**************************
 
     parameter REM_BITS  =  3;
 
 //****************************External Register Declarations**************************
     //AXI4-Stream Interface
       reg       [0:63]     m_axi_rx_tdata; 
       reg                  m_axi_rx_tvalid;
       reg       [0:7]      m_axi_rx_tkeep; 
       reg                  m_axi_rx_tlast; 
 
 //****************************Internal Register Declarations**************************
 
       reg                  rx_pdu_ok; 
       reg                  rx_pe_data_v_r;     
       reg                  rx_pe_data_v_r2;     
       reg       [73:0]     raw_data_r;        
       reg       [73:0]     raw_data_r2;
       reg                  rx_ll_dv_r1;     
       reg                  rx_ll_dv_r2;     
       reg                  rx_cc_r1;     
       reg                  rx_cc_r2;     
       reg                  rx_sep_r; 
       reg                  hold_valid;
       reg                  hold_valid_r;
       reg       [0:7]      rx_keep_dec_lane_0;         
 
 //*********************************Wire Declarations**********************************
 
       wire                 rx_sep_comb; 
       wire                 pipe1_rx_pdu_in_progress_c; 
 
       wire      [0:7]      rx_txkeep_c;
       wire      [0:7]      rx_txkeep_c_1;
       wire      [0:7]      rx_keep;         
       wire      [0:7]      pipe1_rx_keep;         
       wire      [0:7]      pipe2_rx_keep;         
       wire                 sep_detect;
       wire                 sep0_detect;
    
     // Stage 1 pipeline 
       wire      [63:0]     pipe1_rx_pe_data_r;     
       wire                 rx_pe_data_v_c;     
       wire                 pipe1_rx_sep_r;      
       wire                 pipe1_rx_sep7_r;
     // Stage 2 pipeline 
       wire      [63:0]     pipe2_rx_pe_data_r;     
       wire                 pipe2_rx_sep_r;      
       wire                 pipe2_rx_sep7_r;      
       wire                 rx_cc_c;        
       wire                 rx_sep_c; 
       wire                 rx_sep7_c; 
       wire                 rx_idle_c;        
       wire                 dv_conic_sep;        
       wire                 sep_conic_dv;        
       wire                 rx_tvalid_c;        
       wire      [73:0]     raw_data_c;        
       wire      [73:0]     raw_data_c2;    
 
 
     genvar    i;
 //*********************************Main Body of Code**********************************
 
     always @ (*)
     begin
       if (RX_SEP | RX_SEP7) begin
 
         case(RX_SEP_NB[0 : 2]) 
           3'd0     : begin
              if(!(|RX_SEP_NB[0 : 2]))
                      rx_keep_dec_lane_0 = 8'b11111111;
              else 
                      rx_keep_dec_lane_0 = 8'b00000000;
           end 
 
 
           3'd1     : rx_keep_dec_lane_0 = 8'b10000000;
           3'd2     : rx_keep_dec_lane_0 = 8'b11000000;
           3'd3     : rx_keep_dec_lane_0 = 8'b11100000;
           3'd4     : rx_keep_dec_lane_0 = 8'b11110000;
           3'd5     : rx_keep_dec_lane_0 = 8'b11111000;
           3'd6     : rx_keep_dec_lane_0 = 8'b11111100;
           3'd7     : rx_keep_dec_lane_0 = 8'b11111110;
 
           default  : rx_keep_dec_lane_0 = 8'b00000000;
         endcase
       end 
       else if (RX_PE_DATA_V) 
 
         rx_keep_dec_lane_0 = 8'b11111111;
       else 
         rx_keep_dec_lane_0 = 8'b00000000;
     end 
 

     assign rx_keep = rx_keep_dec_lane_0;

 
     assign rx_pe_data_v_c = |RX_PE_DATA_V  & CHANNEL_UP;
     assign rx_sep_c       = |RX_SEP & CHANNEL_UP;
     assign rx_sep7_c      = |RX_SEP7 & CHANNEL_UP;
     assign raw_data_c     = {RX_PE_DATA, rx_keep, rx_sep_c, rx_sep7_c};
 
     always @ (posedge USER_CLK) 
     begin
       if (pipe1_rx_pdu_in_progress_c) 
         raw_data_r <= `DLY raw_data_c;
     end


     always @ (posedge USER_CLK) 
     begin
       raw_data_r2         <= `DLY raw_data_c2;
       rx_pe_data_v_r      <= `DLY rx_pe_data_v_c;
       rx_pe_data_v_r2     <= `DLY rx_pe_data_v_r;
       rx_ll_dv_r1         <= `DLY RXDATAVALID_TO_LL;
       rx_ll_dv_r2         <= `DLY rx_ll_dv_r1;
       rx_cc_r1            <= `DLY rx_cc_c;
       rx_cc_r2            <= `DLY rx_cc_r1;
       rx_sep_r            <= `DLY rx_sep_c;
     end
 
     // Stage 1 pipeline registers data & controls from the sym_dec module
     // If the received data beat is <8bytes then the pipe1 data & control
     // is passed on to the  m_axi_rx_tdata   bus
     assign pipe1_rx_pe_data_r = raw_data_r[73:2+STRB_WIDTH];
     assign pipe1_rx_keep      = raw_data_r[2+STRB_WIDTH-1:2];
     assign pipe1_rx_sep_r     = raw_data_r[1];
     assign pipe1_rx_sep7_r    = raw_data_r[0];
 
     assign   rx_cc_c          =  RX_CC;         
     assign   rx_idle_c        =  &RX_IDLE;
     assign   raw_data_c2      =  raw_data_r;
 
 
     // Stage 2 pipeline registers data & controls from the sym_dec module
     // If the received data beat is less than 8bytes then the pipe1 data & control
     // is passed on to the  m_axi_rx_tdata  bus
     assign pipe2_rx_pe_data_r = raw_data_r2[73:2+STRB_WIDTH];
     assign pipe2_rx_keep      = raw_data_r2[2+STRB_WIDTH-1:2];
     assign pipe2_rx_sep_r     = raw_data_r2[1];
     assign pipe2_rx_sep7_r    = raw_data_r2[0];
 
 
     // rx_pdu_ok_c is set when received block is data block
     always @ (posedge USER_CLK) 
     begin
         rx_pdu_ok <= `DLY ((rx_pe_data_v_r  | (rx_pe_data_v_r & ((pipe1_rx_sep_r  & !pipe1_rx_keep[STRB_WIDTH-1]) | pipe1_rx_sep7_r)))  );
     end

     // Special condition to handle gearbox push coincide with data/sep/sep7
     assign sep_conic_dv = (hold_valid & (rx_pe_data_v_r2 & !(pipe2_rx_sep_r | pipe2_rx_sep7_r)));

     assign dv_conic_sep = hold_valid & rx_ll_dv_r1 & !rx_cc_r1  & rx_pe_data_v_r  & !(pipe2_rx_sep_r | pipe2_rx_sep7_r);

     always @(*)
     begin
       if (rx_pdu_ok & (rx_cc_r1 | !rx_ll_dv_r1  | (!rx_pe_data_v_r & !sep0_detect)))
         hold_valid = 1'b1;
       else if (hold_valid_r & (rx_cc_r2 | !rx_ll_dv_r2  | !rx_pe_data_v_r2))
         hold_valid = 1'b1;
       else 
         hold_valid = 1'b0;
     end 

     always @ (posedge USER_CLK) 
     begin
       if(!CHANNEL_UP)
         hold_valid_r <= `DLY 1'b0;
       else 
         hold_valid_r <= `DLY hold_valid;
     end

     assign rx_tvalid_c  = (rx_pdu_ok | dv_conic_sep | sep0_detect);
 

 
     assign pipe1_rx_pdu_in_progress_c = ((rx_pe_data_v_c  | ( rx_sep_c | rx_sep7_c)  ) & (!rx_cc_c)) ;
 
 
 
     always @(posedge USER_CLK)
     begin
         if (rx_tvalid_c)
           m_axi_rx_tdata  <=  `DLY pipe2_rx_pe_data_r;
     end
 
     //Register the m_axi_rx_tvalid signal
     always @(posedge USER_CLK)
     begin
         if(!CHANNEL_UP)
            m_axi_rx_tvalid   <=  `DLY 1'b0;
         else if (sep_conic_dv)
            m_axi_rx_tvalid   <=  `DLY 1'b0;
         else 
            m_axi_rx_tvalid   <=  `DLY rx_tvalid_c;
     end

     assign sep_detect  = ((pipe2_rx_sep_r & !pipe2_rx_keep[STRB_WIDTH-1]) | pipe2_rx_sep7_r );
     assign sep0_detect = (rx_sep_r & pipe1_rx_keep[STRB_WIDTH-1]);
     always @(posedge USER_CLK)
     begin
       if(!CHANNEL_UP)
         m_axi_rx_tlast  <=  `DLY 1'b0;
       else if (rx_tvalid_c)
         m_axi_rx_tlast  <=  `DLY sep_detect | sep0_detect;
       else 
         m_axi_rx_tlast  <=  `DLY 1'b0;
     end

    assign rx_txkeep_c    = pipe1_rx_keep;
    assign rx_txkeep_c_1  = pipe2_rx_keep;

     //SEP_NB is assigned to m_axi_rx_tkeep    
     always @(posedge USER_CLK)
     begin
         if (sep_detect) 
            m_axi_rx_tkeep  <=  `DLY    rx_txkeep_c_1;
         else if (sep0_detect) 
            m_axi_rx_tkeep  <=  `DLY    rx_txkeep_c;
         else 
            m_axi_rx_tkeep  <=  `DLY    {8{1'b1}};
     end


 
 endmodule
 
 
