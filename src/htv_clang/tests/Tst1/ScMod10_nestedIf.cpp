#include "HtSyscTypes.h"
#include "HtIntTypes.h"

union U {
	int a;
	int b;
};

SC_MODULE(NestedIf) {

	// comment
	sc_in<bool> i_clock1x;

#define TEST1

#ifdef TEST1
#pragma htv tps "1x.x trs=0 tco=0"
	ht_int15 a, b;
#pragma htv tps "1x.x trs=0 tco=0"
	ht_int15 c;
#pragma htv tpe "1x.z, trs=0, tsu=0"
	ht_int15 r;
	U u;
	ht_int15 t;
	ht_int15 r_c, r_t;
#endif

#ifdef TEST2
#pragma htv tps "1x.x trs=0 tco=300"
	ht_int15 a, b, c, d;
#pragma htv tpe "1x.z, trs=0, tsu=0"
	ht_int15 r, r2;
#endif

	bool &rtnBool(bool &a) { return a; }
	void NestedIf1x();
	void NestedIf_src();

	SC_CTOR(NestedIf) {
		SC_METHOD(NestedIf1x);
		sensitive << i_clock1x.pos();
	}
};

void NestedIf::NestedIf1x()
{
	NestedIf_src();
}

void NestedIf::NestedIf_src()
{
	//U v, w;
	//int x = 3;
	//if (a == b) {
	//	u.a = 4;
	//	if (b == c) {
	//		v.a = u.a + 2;
	//		w.b = v.b - u.a;
	//	} else
	//		u.b = a;
	//	u.b = b + c;
	//} else
	//	r = c + v.b;
	//int y = w.a;

	//if (a == c)
		//if (b == c)
#ifdef TEST1
	if (a == b)
		t = a * b;
	else if (a == c)
		t = 0;
	else if (b == c)
		t = b;
	else
		t = c;

	r = r_t + r_c;

	r_t = t;
	r_c = c;
#endif
#ifdef TEST2
	r = (a * b + c) * b + c;
	r2 = d;
#endif
}
