#include "Ht.h"
#include "PersTest.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersTest::PersTest()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST: {
			switch (P_op) {
			case OP_UDIV:
				SendCall_idiv(TEST_RTN,
					      0,
					      (idiv_n_t)P_op1,
					      (idiv_d_t)P_op2);
				break;
			case OP_SDIV:
				SendCall_idiv(TEST_RTN,
					      1,
					      (idiv_n_t)P_op1,
					      (idiv_d_t)P_op2);
				break;
			case OP_UDIVR:
				SendCall_idivr(TEST_RTN,
					       0,
					       (idivr_n_t)P_op1,
					       (idivr_d_t)P_op2);
				break;
			case OP_SDIVR:
				SendCall_idivr(TEST_RTN,
					       1,
					       (idivr_n_t)P_op1,
					       (idivr_d_t)P_op2);
				break;
			default:
				assert(0);
			}
		}
		break;
		case TEST_RTN: {
			BUSY_RETRY(SendReturnBusy_htmain());
			switch (P_op) {
			case OP_UDIV:
				SendReturn_htmain((uint64_t)P_q,
						  (uint64_t)P_r);
				break;
			case OP_SDIV:
				SendReturn_htmain((int64_t)(idiv_sn_t)P_q,
						  (int64_t)P_r);
				break;
			case OP_UDIVR:
				SendReturn_htmain((uint64_t)P_qr,
						  (uint64_t)P_rr);
				break;
			case OP_SDIVR:
				SendReturn_htmain((int64_t)(idivr_sn_t)P_qr,
						  (int64_t)P_rr);
				break;
			default:
				assert(0);
			}
		}
		break;
		default:
			assert(0);
		}
	}
}
