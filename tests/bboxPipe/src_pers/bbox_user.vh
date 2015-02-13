///////////////////////////////////////////////////////////////////////////////
// These will control how the black box framework will behave, 
localparam BBUSER_AMS_SIZE     = 64'd0; // No AMS elements
localparam BBUSER_FPG_FP_CNT   = 8'h4;   // 4 FP's per FPG
localparam BBUSER_CUSTOM_VREC0 = 12'h0;  // no custom vrec (since bit 0 of VRES is 0)
localparam BBUSER_VRES         = 8'b0000_0110; // support 16 or 32 elements/VR
localparam BBUSER_ARC0MSB      = 10;     // 4 bits of user ARC0
localparam BBUSER_ARC0_BITS_VALID = 11'h781;
localparam BBUSER_ARC1MSB      = 0;      // unused
localparam BBUSER_ARC1_BITS_VALID = 1'h0;
localparam BBUSER_ARC2MSB      = 0;      // unused
localparam BBUSER_ARC2_BITS_VALID = 1'h0;
localparam BBUSER_ARC3MSB      = 0;      // unused
localparam BBUSER_ARC3_BITS_VALID = 1'h0;
localparam BBUSER_PIPELEN      = 24;     // length of pipe
localparam BBUSER_MAXITER      = 0;      // maximum number of cycles a bb can iterate in an operation

// please review XILINX requirements for pll's if you wish to use this mode
localparam BBUSER_BBCLK_ASYNC  = 0; // set this to 1 if you want to use bb_clk1x and bb_clk2x (async)
localparam BBUSER_BBCLK_MULT   = 0; // Fvco = this * input frequency (166Mhz for sim, but 150Mhz for phys) 
localparam BBUSER_BBCLK_DIV    = 0; // bb_clk2x freq = Fvco / this
//example:bbclk2x=150Mhz phys, BBUSER_BBCLK_ASYNC=1;BBUSER_BBCLK_MULT=6;BBUSER_BBCLK_DIV=3;

///////////////////////////////////////////////////////////////////////////////
// This will pull in the predefined values for ARC0 which are
//localparam ARC0_SINGLE    = 0;
//localparam ARC0_LOAD0     = 1;
localparam ARC0_START0    = 2;
//localparam ARC0_STORE0    = 3;
//localparam ARC0_LOAD1     = 4;
localparam ARC0_START1    = 5;
//localparam ARC0_STORE1    = 6;
//`include "bbox_arc0.vh"

//////////////////////////////////////////////////////////////////////////////
// optional user defined locations in ARC's
// these are parsed by bBoxParser so they can be used in the C model and GDB
// They must follow the naming convention of BBARC#_<YourFoo> = <BIT_lsb>; // optional comment
// the ARC#_ prefix is reserved for ARC*_ field locations and may not be used for anything else in this file
localparam ARC0_AMux         = 7;// 1 - 0 to A addr input
localparam ARC0_BMux         = 8;   // 00 - inB to B addr input
                                    // 01 - Scalar to B addr input
                                    // 10 - 0 to B addr input
                                    // 11 - 0 to B addr input
localparam ARC0_BBop         = 10;  //  0 - centrifuge
                                    //  1 - pop count

///////////////////////////////////////////////////////////////////////////////
// optional user defined values for ARC field's
// these are parsed by bBoxParser so they can be used in the C model and in GDB
// They must follow the naming convention of Arc#_<> = value 
// the Arc#_ prefix is reserved for Arc*_ field values and may not be used for anything else in this file
localparam Arc0_AMux_A              = 1'b0;
localparam Arc0_AMux_Zero           = 1'b1;
localparam Arc0_BMux_B              = 2'b00;
localparam Arc0_BMux_S              = 2'b01;
localparam Arc0_BMux_Zero           = 2'b10;
localparam Arc0_BMux_Ill            = 2'b11;
localparam Arc0_BBop_CFG            = 1'b0;
localparam Arc0_BBop_POP            = 1'b1;

///////////////////////////////////////////////////////////////////////////////
// this is a define added here to be used in bbox_box_top.v
// it isn't used anywhere else
localparam BBARC0_idle = 11'h280; // force adder inputs to zero, no write enable

///////////////////////////////////////////////////////////////////////////////
// user defined ARC values for mov to AMS/ mov from AMS
// these are required, they should be set to zero if no AMS is present
localparam BBARC0_MOV_TO_AMS   = 12'h0;
localparam BBARC0_MOV_FROM_AMS = 12'h0;
localparam BBARC1_MOV_TO_AMS   = 1'h0;
localparam BBARC1_MOV_FROM_AMS = 1'h0;
localparam BBARC2_MOV_TO_AMS   = 1'h0;
localparam BBARC2_MOV_FROM_AMS = 1'h0;
localparam BBARC3_MOV_TO_AMS   = 1'h0;
localparam BBARC3_MOV_FROM_AMS = 1'h0;

///////////////////////////////////////////////////////////////////////////////
// which ARITH instructions are valid for this bbox personalities
// each type of ARITH this will be used by the personality should be set to 1'b1
// instructions which write only VM
localparam ARITHarc_VM_valid    = 1'b0;
localparam ARITHimm_VM_valid    = 1'b0;
localparam ARITHarc_SVM_valid   = 1'b0;
localparam ARITHarc_VSVM_valid  = 1'b0;
localparam ARITHarc_VVVM_valid  = 1'b0;
localparam ARITHimm_VVVM_valid  = 1'b0;
// instructions which write only Vt with 0 or 1 inputs
localparam ARITHarc_V_valid     = 1'b1;
localparam ARITHimm_V_valid     = 1'b1;
localparam ARITHarc_VV_valid    = 1'b1;
localparam ARITHimm_VV_valid    = 1'b1;
localparam ARITHarc_SV_valid    = 1'b1;
// instructions which write only Vt with 2 inputs
localparam ARITHarc_VSV_valid   = 1'b1;
localparam ARITHarc_VVV_valid   = 1'b1;
localparam ARITHimm_VVV_valid   = 1'b1;
// instructions which write only Vt with 3 inputs
localparam ARITHarc_VSVV_valid  = 1'b0;
// instructions which write Vm and Vt with 2 inputs
localparam ARITHarc_VSVMV_valid = 1'b0;
localparam ARITHarc_VVVMV_valid = 1'b0;
localparam ARITHimm_VVVMV_valid = 1'b0;
// instructions with no dest (use to write to AMS or just check for a hit/error signal)
localparam ARITHarc_N_valid     = 1'b0;
localparam ARITHimm_N_valid     = 1'b0;
localparam ARITHarc_N_S_valid   = 1'b0;
localparam ARITHarc_N_V_valid   = 1'b1;
localparam ARITHimm_N_V_valid   = 1'b1;
localparam ARITHarc_N_VV_valid  = 1'b1;
localparam ARITHimm_N_VV_valid  = 1'b1;
localparam ARITHarc_N_VS_valid  = 1'b1;
localparam ARITHarc_N_VSV_valid = 1'b0;

