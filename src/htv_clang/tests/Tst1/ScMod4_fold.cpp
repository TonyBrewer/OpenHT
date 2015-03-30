#include "HtSyscTypes.h"

SC_MODULE(FoldTst) {
	void FoldTst1x();

	SC_CTOR(FoldTst) {
		SC_METHOD(FoldTst1x);
		//sensitive << i_clock1x.pos();
	}
};

enum E { e1, e2, e3=4 };

void FoldTst::FoldTst1x()
{
	char c = "abcd"[3];
	char d = "abcd"[6];

	int a = 0;
	if (e1 == e2)
		a = 1+5/3;
	else
		a = 2 == 6%4 ? 2 : -1;
}
