#include "Ht.h"
#include "PersBug.h"

void
CPersBug::PersBug()
{
	if (PR_htValid) {
		switch (PR_htInstr) {

		// Main entry point from Host (htmain call)
		case BUG_ENTRY:
		{
			S_bStart1 = true;

			HtContinue(BUG_RTN);
		}
		break;
		case BUG_RTN:
		{
			if (!SR_bStart4 || SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			if (S_data1.b == 0x1234567812345678LL || S_data2.b != 0x1234567812345678LL)
				HtAssert(0, 0);

			if (S_data3 != 0x89ab)
				HtAssert(0, 0);

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}

	if (SR_bStart1) {
		S_bStart1 = false;

		X x;
		x.a = 0x12345678;
		x.b = 0x1234567812345678LL;
		x.c = 0x123;

		S_bram1.write_addr(1);
		S_bram1.write_mem(x);

		S_bram1.read_addr(1);

		S_bram2.write_addr(1);
		S_bram2.write_mem(x);

		S_bram3.write_addr(1);
		S_bram3.write_mem(0, 0xcdef);
		S_bram3.write_mem(1, 0x89ab);
		S_bram3.write_mem(2, 0x4567);
		S_bram3.write_mem(3, 0x0123);

		S_bStart2 = true;
	}

	if (SR_bStart2) {
		S_bStart2 = false;

		S_data1 = S_bram1.read_mem();

		S_bram2.read_addr(1);
		S_bram3.read_addr(1, 1);

		S_bStart3 = true;
	}

	if (SR_bStart3) {
		S_bStart3 = false;

		S_data2 = S_bram2.read_mem();
		S_data3 = S_bram3.read_mem();

		S_bStart4 = true;
	}
}
