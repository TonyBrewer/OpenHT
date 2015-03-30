#include "HtSyscTypes.h"

//-D_HTV -IC:/Users/TBrewer/Documents/llvm-project/my_tests/ht_lib -IC:/Users/TBrewer/Documents/llvm-project/my_tests/Clk2x C:/Users/TBrewer/Documents/llvm-project/my_tests/Tst1/ScMod1.cpp

SC_MODULE(PersHta) {

	class X {
	public:
		X() { a = 1; }
		X(int a_) { a = a_; }

	private:
		int a;
	};

	class V {
	public:

		int b;
	};

	X xx;

	void PersHta1x();
	void PersHta_src();

	SC_CTOR(PersHta) {
		SC_METHOD(PersHta1x);
		//sensitive << i_clock1x.pos();
	}
};

void func(int a[2], int (&b)[3])
{
	int c = b[1];
	int d = a[4];
	int i = 1;
	int f[6];
	f[i++] = b[2] + a[3];
}

void PersHta::PersHta1x()
{
	X x;
	x = X();

	V v;
	v = V();

	X y = X();
	X z = X(3);

	z = X(3);

	PersHta_src();
}

void PersHta::PersHta_src()
{
	X q[5];
	X s[4] = { X(1), X(4) };
	X t[4][5] = { X(0), X(), { X(2) }, { X(5) } };
	X u[4][3] = { { X(), X() }, { X(1) }, { X(2) } };
	t[2][4] = X(6);

	{
		int x = 3;
		int t = 2;
		t = 1;
	}

	int g[2];
	int h[4][3];

	int (&j)[3] = h[1];

	func(g, h[1]);
}
