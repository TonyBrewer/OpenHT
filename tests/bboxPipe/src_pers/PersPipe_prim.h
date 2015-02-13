#ifndef PERSPIPE_PRIM_H
#define PERSPIPE_PRIM_H


// state for bbox clocked primitive
ht_state struct bbox_prim_state {
	bool		r_vm[25];
	uint64_t	r_vt[25];
};

// would be nice if reset was auto connected like clk...
ht_prim ht_clk("intfClk1x", "intfClk2x") void bbox_wrap(bool &vm, uint64_t &vt, bool const &reset, uint64_t const &scalar, ht_uint12 const &arc0, uint64_t const &va, uint64_t const &vb, bbox_prim_state &s);

#endif
