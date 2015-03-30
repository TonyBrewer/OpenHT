#include "HtSyscTypes.h"

SC_MODULE(RefTst) {
	void RefTst1x();

	struct A {
		A() {}
		A(A &ax) { m_a = ax.m_b; m_b = ax.m_a; }
		int m_a:3;
		int m_b:4;
	} m_A;

	struct B {
		int m_c:5;
		int m_d:6;
		A m_e;
		A m_f;
	} m_B, m_C;

	A & refA(A & a1, A & a2) {
		bool b[4] = { true, false, false, true };
		return b[1] ? a1 : a2;
	}

	SC_CTOR(RefTst) {
		SC_METHOD(RefTst1x);
		//sensitive << i_clock1x.pos();
	}
};



void RefTst::RefTst1x()
{
	A & a = m_A;
	A & b = m_B.m_e;
	B & c = m_C;
	A & d = c.m_f;
	A & e = true ? d : b;

	int x = 2 - 1;
	bool bo[2][3];
	bool (&br)[3] = bo[++x];
	x = 2;
	if (false)
		int bi = br[x++];

	A f = a;
	A g = b;
	B h = c;
	A i = d;
	A j = e;
	j = e;

	e.m_b = 3;

	A k = refA(e, j);

	refA(e, j).m_a = 2;
}
