#include "HtSyscTypes.h"

SC_MODULE(FoldTst) {
	void FoldTst1x();

	SC_CTOR(FoldTst) {
		SC_METHOD(FoldTst1x);
		//sensitive << i_clock1x.pos();
	}
};

class X {
private:
	int a;
public:
	X() { a = 1; }
	X(int aa) { a = aa; }
};

void FoldTst::FoldTst1x()
{
	int a = 2;
	X d[3] = { X(), 3, X(a) };
	X c = X();
	X b;
	b = X();
	X & e = b;
	X f = e;
}
