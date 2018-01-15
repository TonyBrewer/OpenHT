/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
module dispatch #(parameter
    FREQ = 0,
    PART = 0
)(
input		clk,
`ifdef CNY_PLATFORM_TYPE2
input		clkhx,
`endif
input		reset,

input		disp_inst_vld,
input  [4:0]	disp_inst,
input  [17:0]	disp_aeg_idx,
input		disp_aeg_rd,
input		disp_aeg_wr,
input  [63:0]	disp_aeg_wr_data,

output [17:0]	disp_aeg_cnt,
output [15:0]	disp_exception,
output [63:0]	disp_rtn_data,
output		disp_rtn_data_vld,
output		disp_idle,
output		disp_stall,


// personality control
input	[3:0]	num_ae,
input	[7:0]	num_units,
output [15:0]	ctlQueWidth,
output [47:0]	ctlQueBase,

output		start,
output		reset_top,
input		busy
);

    //
    // AEG[0..NA-1] Registers
    //
    localparam NA = 5;
    localparam AB = f_enc_bits(NA);

    wire r_reset;
`ifdef CNY_PLATFORM_TYPE2
    HtResetFlop1x r_reset_ (
			   .clkhx(clkhx),
			   .clk1x(clk),
			   .r_reset(r_reset),
			   .i_reset(reset)
			   );
`else
    HtResetFlop r_reset_ (
			 .clk(clk),
			 .r_reset(r_reset),
			 .i_reset(reset)
			 );
`endif

    assign disp_aeg_cnt = NA;

    wire [63:0] w_aeg[NA-1:0];
    wire c_st_idle;

    genvar g;
    generate for (g=0; g<NA; g=g+1) begin : g0
	reg [63:0] c_aeg, r_aeg;

	always @* begin
	    c_aeg = r_aeg;
	    case (g)
		// report exceptions to app
		1: c_aeg = r_aeg | disp_exception;
		// coded funny for random init
		2: if (disp_idle || !disp_idle) c_aeg = PART;
		3: if (disp_idle || !disp_idle) begin
		    c_aeg[36 +: 28] = 'b0;	// Reserved
		    c_aeg[32 +:  4] = num_ae;
		    c_aeg[16 +: 16] = FREQ;
		    c_aeg[ 8 +:  8] = 8'h00;	// ABI {4'major, 4'minor}
		    c_aeg[ 0 +:  8] = num_units;
		end
		// polling interface
		4:  begin
		    c_aeg = 'b0;
		    c_aeg[0] = c_st_idle;
		end
	    endcase
	end

	wire c_aeg_we = disp_aeg_wr && disp_aeg_idx[AB-1:0] == g;

	always @(posedge clk) begin
	    if (c_aeg_we)
		r_aeg <= disp_aeg_wr_data;
	    else
		r_aeg <= c_aeg;
	end
	assign w_aeg[g] = r_aeg;
    end endgenerate

    reg		r_rtn_val, r_err_unimpl, r_err_aegidx;
    reg [63:0]	r_rtn_data;

    wire c_val_aegidx = disp_aeg_idx < NA;
    wire c_is_caep0 = disp_inst_vld && disp_inst == 'd0;
    wire c_is_caep1 = disp_inst_vld && disp_inst == 'd1;
    wire c_bad_caep = disp_inst_vld && disp_inst > 'd1;

    always @(posedge clk) begin
	r_rtn_val    <= disp_aeg_rd;
	r_rtn_data   <= c_val_aegidx ?
			w_aeg[disp_aeg_idx[AB-1:0]] : {2{32'hdeadbeef}};
	r_err_aegidx <= (disp_aeg_wr || disp_aeg_rd) && !c_val_aegidx;
	r_err_unimpl <= c_bad_caep;
    end
    assign disp_rtn_data_vld = r_rtn_val;
    assign disp_rtn_data     = r_rtn_data;
    assign disp_exception    = {14'b0, r_err_aegidx, r_err_unimpl};

    // Dispatch information
    assign ctlQueWidth = w_aeg[0][48 +: 16];
    assign ctlQueBase  = w_aeg[0][0  +: 48];


    //
    // Execute state machine
    //
    localparam	IDLE = 0,
		RESET = 1,
		START = 2,
		BUSY = 3;

    reg		r_st_idle, r_cnt_eq0;
    reg		c_start, r_start, c_caep1, r_caep1;
    reg [1:0]	c_state, r_state,
		c_cnt, r_cnt;

    wire c_kickit = c_is_caep0 || c_is_caep1;

    always @* begin
	// default(s)
	c_state = r_state;
	c_cnt  = ~|r_cnt ? r_cnt : r_cnt - 'b1;
	c_start = 'b0;
	c_caep1 = r_caep1;

	case (r_state)
	    IDLE: begin
		if (c_kickit) begin
		    c_state = RESET;
		    c_cnt = 'd3;
		end
		c_caep1 = c_is_caep1;
	    end
	    RESET: begin
		if (r_cnt_eq0) begin
		    c_state = START;
		    c_cnt = 'd3;
		    c_start = 'b1;
		end
	    end
	    START: begin
		if (r_cnt_eq0) begin
		    c_state = BUSY;
		    c_cnt = 'd3;
		end
	    end
	    BUSY: begin
		if (r_cnt_eq0) begin
		    c_state = IDLE;
		end
		if (busy) begin
		    c_cnt = 'd3;
		end
	    end
	    default: c_state = 'bx;
	endcase
    end

    assign c_st_idle = c_state == IDLE;

    always @(posedge clk) begin
	if (r_reset) begin
	    r_state  <= IDLE;
	end else begin
	    r_state  <= c_state;
	end
	r_st_idle <= c_st_idle;
	r_start   <= c_start;
	r_caep1   <= c_caep1;

	r_cnt_eq0 <= c_cnt == 'd0;
	r_cnt     <= c_cnt;
    end
    assign disp_idle  = r_st_idle;
    assign disp_stall = !r_caep1 && (!r_st_idle || c_kickit);
    assign start = r_start;
`ifdef CNY_PLATFORM_TYPE2
    reg		r_st_idle_hx;
    always @(posedge clkhx) begin
	r_st_idle_hx <= r_st_idle;
    end
    assign reset_top = r_st_idle_hx;
`else
    assign reset_top = r_st_idle;
`endif

    `include "functions.vh"
endmodule
