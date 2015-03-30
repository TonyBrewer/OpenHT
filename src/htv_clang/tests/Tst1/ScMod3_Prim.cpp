#include "HtSyscTypes.h"

SC_MODULE(PrimTst) {
	void PrimTst1x();

	SC_CTOR(PrimTst) {
		SC_METHOD(PrimTst1x);
		//sensitive << i_clock1x.pos();
	}
};

// ht_prim_timing(r=a+2, r=b+3)
ht_prim ht_clk("ck1", "ck2") void HtPrim(int const & a, int const & b, int & r);

enum E { e1, e2, e3=4 };

void PrimTst::PrimTst1x()
{
	int aa[4][3] = { { 1, 2 }, 3 };
	int a = 3;
	int b = 5;
	int r;

	HtPrim(a, b, r);

	E e = e2;
	if (e == e3)
		a += 1;
}
