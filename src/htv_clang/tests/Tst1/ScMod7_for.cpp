#include "HtSyscTypes.h"

SC_MODULE(For) {
	void For1x();

	SC_CTOR(For) {
		SC_METHOD(For1x);
		//sensitive << i_clock1x.pos();
	}
};

void For::For1x()
{
	int a;
	for (int i = 0; i < 3; i += 1)
		a = i;
}
