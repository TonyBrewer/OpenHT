#include "HtSyscTypes.h"

SC_MODULE(Break) {
	void Break1x();

	SC_CTOR(Break) {
		SC_METHOD(Break1x);
		//sensitive << i_clock1x.pos();
	}
};

void Break::Break1x()
{
	int a = 2;
	switch(a) {
	case 0:
		a = 3;
		{
			if (a==1)
				break;
			else
				a = 2;
		}
		if (a==0)
			a = 9;
		a = 4;
		if (a == 6) {
			if (a == 7)
				a = 8;
			else {
				break;
			}
		}
		a = 5;
		break;
	}
}
