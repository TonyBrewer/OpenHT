#include "Ht.h"
#include "PersBigInt.h"

void CPersBigInt::PersBigInt()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case BI_INIT:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			sc_bigint<66> b66 = 0x50;
			sc_bigint<62> b62 = b66 >> 4;
			sc_biguint<4> b4 = 9;
			sc_biguint<4> b4b = ~b4;
			sc_bigint<68> b68 = -b62 + b4b;
			if (++b68 != 2) {
				P_err = true;
			}

			sc_biguint<5> b5 = 5;
			sc_biguint<5> c5 = 6 + b5;
			ht_uint5 h5 = 5;
			if (c5 != h5 + 6) {
				P_err = true;
			}

			bool bl = c5[2];
			if (bl != false) {
				P_err = true;
			}

			SendReturn_htmain(P_err);
		}
		break;
		default:
			assert(0);
		}
	}
}
