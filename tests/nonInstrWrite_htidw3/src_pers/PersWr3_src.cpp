#include "Ht.h"
#include "PersWr3.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersWr3::PersWr3()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case WR3_ENTRY: {
			if (SR_counter == COUNT) {
				HtContinue(WR3_RTN);
			} else {
				S_counter = SR_counter + 1;
				GW_gv_3w_1r_var_reg = 0x9ABC;
				GW_gv_3w_2r_var_reg = 0x9ABC;
				GW_gv_3w_1r_var_dst.write_addr(4);
				GW_gv_3w_1r_var_dst = 0x9ABC;
				GW_gv_3w_2r_var_dst.write_addr(4);
				GW_gv_3w_2r_var_dst = 0x9ABC;
				GW_gv_3w_1r_var_blk.write_addr(4);
				GW_gv_3w_1r_var_blk = 0x9ABC;
				GW_gv_3w_2r_var_blk.write_addr(4);
				GW_gv_3w_2r_var_blk = 0x9ABC;

				GW_gv_3w_1r_str_a_reg.v1 = 0x9ABC;
				GW_gv_3w_2r_str_a_reg.v1 = 0x9ABC;
				GW_gv_3w_1r_str_a_dst.write_addr(4);
				GW_gv_3w_1r_str_a_dst.v1 = 0x9ABC;
				GW_gv_3w_2r_str_a_dst.write_addr(4);
				GW_gv_3w_2r_str_a_dst.v1 = 0x9ABC;
				GW_gv_3w_1r_str_a_blk.write_addr(4);
				GW_gv_3w_1r_str_a_blk.v1 = 0x9ABC;
				GW_gv_3w_2r_str_a_blk.write_addr(4);
				GW_gv_3w_2r_str_a_blk.v1 = 0x9ABC;

				GW_gv_3w_1r_str_b_reg.v1 = 0x9ABC;
				GW_gv_3w_2r_str_b_reg.v1 = 0x9ABC;
				GW_gv_3w_1r_str_b_dst.write_addr(4);
				GW_gv_3w_1r_str_b_dst.v1 = 0x9ABC;
				GW_gv_3w_2r_str_b_dst.write_addr(4);
				GW_gv_3w_2r_str_b_dst.v1 = 0x9ABC;
				GW_gv_3w_1r_str_b_blk.write_addr(4);
				GW_gv_3w_1r_str_b_blk.v1 = 0x9ABC;
				GW_gv_3w_2r_str_b_blk.write_addr(4);
				GW_gv_3w_2r_str_b_blk.v1 = 0x9ABC;

				GW_gv_3w_1r_unn_a_reg.v2 = 0x9ABC;
				GW_gv_3w_2r_unn_a_reg.v2 = 0x9ABC;
				GW_gv_3w_1r_unn_a_dst.write_addr(4);
				GW_gv_3w_1r_unn_a_dst.v2 = 0x9ABC;
				GW_gv_3w_2r_unn_a_dst.write_addr(4);
				GW_gv_3w_2r_unn_a_dst.v2 = 0x9ABC;
				GW_gv_3w_1r_unn_a_blk.write_addr(4);
				GW_gv_3w_1r_unn_a_blk.v2 = 0x9ABC;
				GW_gv_3w_2r_unn_a_blk.write_addr(4);
				GW_gv_3w_2r_unn_a_blk.v2 = 0x9ABC;

				GW_gv_3w_1r_unn_b_reg.v2 = 0x9ABC;
				GW_gv_3w_2r_unn_b_reg.v2 = 0x9ABC;
				GW_gv_3w_1r_unn_b_dst.write_addr(4);
				GW_gv_3w_1r_unn_b_dst.v2 = 0x9ABC;
				GW_gv_3w_2r_unn_b_dst.write_addr(4);
				GW_gv_3w_2r_unn_b_dst.v2 = 0x9ABC;
				GW_gv_3w_1r_unn_b_blk.write_addr(4);
				GW_gv_3w_1r_unn_b_blk.v2 = 0x9ABC;
				GW_gv_3w_2r_unn_b_blk.write_addr(4);
				GW_gv_3w_2r_unn_b_blk.v2 = 0x9ABC;
				HtContinue(WR3_ENTRY);
			}
		}
		break;
		case WR3_RTN: {
			BUSY_RETRY(SendReturnBusy_wr3());

			SendReturn_wr3();
		}
		break;
		default:
			assert(0);
		}
	}
}
