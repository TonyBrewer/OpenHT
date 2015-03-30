#include "HtSyscTypes.h"
#include "HtIntTypes.h"

SC_MODULE(PrimTst) {

	// comment
#pragma htv tps "1x"
	ht_int15 a, b, c;
#pragma htv tpe "1x"
	ht_int15 r;

	void PrimTst1x();
	void PrimTst_src();

	SC_CTOR(PrimTst) {
		SC_METHOD(PrimTst1x);
		//sensitive << i_clock1x.pos();
	}
};

void PrimTst::PrimTst1x()
{
	PrimTst_src();
}

void PrimTst::PrimTst_src()
{
	r = a * b + c;
	//r = ht_int15::fma(a, b, c);
}
