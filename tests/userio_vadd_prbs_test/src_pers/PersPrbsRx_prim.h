#pragma once

// state for prbs clocked primitive
ht_state struct prbs_rcv_prm_state {
	ht_uint64	rnd_lower[2];
	ht_uint64	rnd_upper[2];
	ht_uint64	res_lower[1];
	ht_uint64	res_upper[1];
	bool    	err[1];
	bool    	vld[1];
};

//   function definition located in .cpp file
ht_prim ht_clk("ck") void prbs_rcv (
	bool const & rst,
	bool const & i_req,
	ht_uint64 const & i_din_lower, 
	ht_uint64 const & i_din_upper, 
	bool & o_err,
	bool & o_vld,
	prbs_rcv_prm_state &s);
