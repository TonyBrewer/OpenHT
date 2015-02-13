#include "Ht.h"
#include "PersCopy.h"

// Copy regDimen1 to regDimen2 transposing var dimensions

void CPersCopy::PersCopy()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case ENTRY:
			{
				if (SendReturnBusy_copy()) {
					HtRetry();
					break;
				}

				switch (P_tst) {
				case 13:
					for (int i = 0; i < 2; i += 1) {
						for (int j = 0; j < 8; j += 1) {
							GW_regDimen2_data(j, i, GR_regDimen1_data(i, j));
						}
					}
					SendReturn_copy();
					break;
				case 14:
					for (int i = 0; i < 2; i += 1) {
						for (int j = 0; j < 8; j += 1) {
							GW_regFldDimen2_data(j, i, GR_regFldDimen1_data(i, j));
						}
					}
					SendReturn_copy();
					break;
				case 15:
					for (int i = 0; i < 2; i += 1) {
						for (int j = 0; j < 8; j += 1) {
							GW_varFld2_data(j, i, GR_varFld1_data(i, j));
						}
					}
					SendReturn_copy();
					break;
				case 16:
					P_varAddr1 = 0;
					HtContinue(T16C1);
					break;
				case 17:
					P_fldAddr1 = 0;
					HtContinue(T17C1);
					break;
				default:
					assert(0);
				}
			}
			break;
		case T16C1:
			{
				GW_varAddr2_data(0, 0, GR_varAddr1_data(0));

				P_varAddr1 = 1;

				HtContinue(T16C2);
			}
			break;
		case T16C2:
			{
				GW_varAddr2_data(0, 1, GR_varAddr1_data(0));

				P_varAddr1 = 0;

				HtContinue(T16C3);
			}
			break;
		case T16C3:
			{
				GW_varAddr2_data(1, 0, GR_varAddr1_data(1));

				P_varAddr1 = 1;

				HtContinue(T16C4);
			}
			break;
		case T16C4:
			{
				if (SendReturnBusy_copy()) {
					HtRetry();
					break;
				}

				GW_varAddr2_data(1, 1, GR_varAddr1_data(1));

				SendReturn_copy();
			}
			break;
		case T17C1:
			{
				GW_fldAddr2_data(0, 0, GR_fldAddr1_data(0));

				P_fldAddr1 = 1;

				HtContinue(T17C2);
			}
			break;
		case T17C2:
			{
				GW_fldAddr2_data(0, 1, GR_fldAddr1_data(0));

				P_fldAddr1 = 0;

				HtContinue(T17C3);
			}
			break;
		case T17C3:
			{
				GW_fldAddr2_data(1, 0, GR_fldAddr1_data(1));

				P_fldAddr1 = 1;

				HtContinue(T17C4);
			}
			break;
		case T17C4:
			{
				if (SendReturnBusy_copy()) {
					HtRetry();
					break;
				}

				GW_fldAddr2_data(1, 1, GR_fldAddr1_data(1));

				SendReturn_copy();
			}
			break;
		default:
			assert(0);
		}
	}
}
