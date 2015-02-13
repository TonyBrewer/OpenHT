#pragma once

#include "PersCommon.h"
#include "Pers_structs.h"

// state for add clocked primitive
ht_state struct addCState {
	uint32_t	res;
};

// 32-bit add, clocked primitive
//   function definition located in .cpp file
ht_prim ht_clk("ck") void addC (
	uint32_t const & i_a, 
	uint32_t const & i_b,
	uint32_t & o_res,
	addCState &s);

// 32-bit add, propagational primitive
//   function definition located in .cpp file
ht_prim void addP (
	uint32_t const & i_a, 
	uint32_t const & i_b,
	uint32_t & o_res);

// 32-bit add, propagational primitive using structures for input and output
//   function definition located in .cpp file
ht_prim void addS (
	AddIn const & i_in, 
	AddOut & o_res);
