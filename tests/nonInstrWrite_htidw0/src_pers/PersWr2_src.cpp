#include "Ht.h"
#include "PersWr2.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

void
CPersWr2::PersWr2()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case WR2_ENTRY: {
			if (SR_counter == COUNT) {
				HtContinue(WR2_RTN);
			} else {
				S_counter = SR_counter + 1;
				GW_gv_2w_1r_var_reg = 0x5678;
				GW_gv_2w_2r_var_reg = 0x5678;
				GW_gv_3w_1r_var_reg = 0x5678;
				GW_gv_3w_2r_var_reg = 0x5678;
				GW_gv_2w_1r_var_dst.write_addr(4);
				GW_gv_2w_1r_var_dst = 0x5678;
				GW_gv_2w_2r_var_dst.write_addr(4);
				GW_gv_2w_2r_var_dst = 0x5678;
				GW_gv_3w_1r_var_dst.write_addr(4);
				GW_gv_3w_1r_var_dst = 0x5678;
				GW_gv_3w_2r_var_dst.write_addr(4);
				GW_gv_3w_2r_var_dst = 0x5678;
				GW_gv_2w_1r_var_blk.write_addr(4);
				GW_gv_2w_1r_var_blk = 0x5678;
				GW_gv_2w_2r_var_blk.write_addr(4);
				GW_gv_2w_2r_var_blk = 0x5678;
				GW_gv_3w_1r_var_blk.write_addr(4);
				GW_gv_3w_1r_var_blk = 0x5678;
				GW_gv_3w_2r_var_blk.write_addr(4);
				GW_gv_3w_2r_var_blk = 0x5678;

				GW_gv_2w_1r_str_a_reg.v1 = 0x5678;
				GW_gv_2w_2r_str_a_reg.v1 = 0x5678;
				GW_gv_3w_1r_str_a_reg.v1 = 0x5678;
				GW_gv_3w_2r_str_a_reg.v1 = 0x5678;
				GW_gv_2w_1r_str_a_dst.write_addr(4);
				GW_gv_2w_1r_str_a_dst.v1 = 0x5678;
				GW_gv_2w_2r_str_a_dst.write_addr(4);
				GW_gv_2w_2r_str_a_dst.v1 = 0x5678;
				GW_gv_3w_1r_str_a_dst.write_addr(4);
				GW_gv_3w_1r_str_a_dst.v1 = 0x5678;
				GW_gv_3w_2r_str_a_dst.write_addr(4);
				GW_gv_3w_2r_str_a_dst.v1 = 0x5678;
				GW_gv_2w_1r_str_a_blk.write_addr(4);
				GW_gv_2w_1r_str_a_blk.v1 = 0x5678;
				GW_gv_2w_2r_str_a_blk.write_addr(4);
				GW_gv_2w_2r_str_a_blk.v1 = 0x5678;
				GW_gv_3w_1r_str_a_blk.write_addr(4);
				GW_gv_3w_1r_str_a_blk.v1 = 0x5678;
				GW_gv_3w_2r_str_a_blk.write_addr(4);
				GW_gv_3w_2r_str_a_blk.v1 = 0x5678;

				GW_gv_2w_1r_str_b_reg.v1 = 0x5678;
				GW_gv_2w_2r_str_b_reg.v1 = 0x5678;
				GW_gv_3w_1r_str_b_reg.v1 = 0x5678;
				GW_gv_3w_2r_str_b_reg.v1 = 0x5678;
				GW_gv_2w_1r_str_b_dst.write_addr(4);
				GW_gv_2w_1r_str_b_dst.v1 = 0x5678;
				GW_gv_2w_2r_str_b_dst.write_addr(4);
				GW_gv_2w_2r_str_b_dst.v1 = 0x5678;
				GW_gv_3w_1r_str_b_dst.write_addr(4);
				GW_gv_3w_1r_str_b_dst.v1 = 0x5678;
				GW_gv_3w_2r_str_b_dst.write_addr(4);
				GW_gv_3w_2r_str_b_dst.v1 = 0x5678;
				GW_gv_2w_1r_str_b_blk.write_addr(4);
				GW_gv_2w_1r_str_b_blk.v1 = 0x5678;
				GW_gv_2w_2r_str_b_blk.write_addr(4);
				GW_gv_2w_2r_str_b_blk.v1 = 0x5678;
				GW_gv_3w_1r_str_b_blk.write_addr(4);
				GW_gv_3w_1r_str_b_blk.v1 = 0x5678;
				GW_gv_3w_2r_str_b_blk.write_addr(4);
				GW_gv_3w_2r_str_b_blk.v1 = 0x5678;

				GW_gv_2w_1r_unn_a_reg.v2 = 0x5678;
				GW_gv_2w_2r_unn_a_reg.v2 = 0x5678;
				GW_gv_3w_1r_unn_a_reg.v2 = 0x5678;
				GW_gv_3w_2r_unn_a_reg.v2 = 0x5678;
				GW_gv_2w_1r_unn_a_dst.write_addr(4);
				GW_gv_2w_1r_unn_a_dst.v2 = 0x5678;
				GW_gv_2w_2r_unn_a_dst.write_addr(4);
				GW_gv_2w_2r_unn_a_dst.v2 = 0x5678;
				GW_gv_3w_1r_unn_a_dst.write_addr(4);
				GW_gv_3w_1r_unn_a_dst.v2 = 0x5678;
				GW_gv_3w_2r_unn_a_dst.write_addr(4);
				GW_gv_3w_2r_unn_a_dst.v2 = 0x5678;
				GW_gv_2w_1r_unn_a_blk.write_addr(4);
				GW_gv_2w_1r_unn_a_blk.v2 = 0x5678;
				GW_gv_2w_2r_unn_a_blk.write_addr(4);
				GW_gv_2w_2r_unn_a_blk.v2 = 0x5678;
				GW_gv_3w_1r_unn_a_blk.write_addr(4);
				GW_gv_3w_1r_unn_a_blk.v2 = 0x5678;
				GW_gv_3w_2r_unn_a_blk.write_addr(4);
				GW_gv_3w_2r_unn_a_blk.v2 = 0x5678;

				GW_gv_2w_1r_unn_b_reg.v2 = 0x5678;
				GW_gv_2w_2r_unn_b_reg.v2 = 0x5678;
				GW_gv_3w_1r_unn_b_reg.v2 = 0x5678;
				GW_gv_3w_2r_unn_b_reg.v2 = 0x5678;
				GW_gv_2w_1r_unn_b_dst.write_addr(4);
				GW_gv_2w_1r_unn_b_dst.v2 = 0x5678;
				GW_gv_2w_2r_unn_b_dst.write_addr(4);
				GW_gv_2w_2r_unn_b_dst.v2 = 0x5678;
				GW_gv_3w_1r_unn_b_dst.write_addr(4);
				GW_gv_3w_1r_unn_b_dst.v2 = 0x5678;
				GW_gv_3w_2r_unn_b_dst.write_addr(4);
				GW_gv_3w_2r_unn_b_dst.v2 = 0x5678;
				GW_gv_2w_1r_unn_b_blk.write_addr(4);
				GW_gv_2w_1r_unn_b_blk.v2 = 0x5678;
				GW_gv_2w_2r_unn_b_blk.write_addr(4);
				GW_gv_2w_2r_unn_b_blk.v2 = 0x5678;
				GW_gv_3w_1r_unn_b_blk.write_addr(4);
				GW_gv_3w_1r_unn_b_blk.v2 = 0x5678;
				GW_gv_3w_2r_unn_b_blk.write_addr(4);
				GW_gv_3w_2r_unn_b_blk.v2 = 0x5678;
				HtContinue(WR2_ENTRY);
			}
		}
		break;
		case WR2_RTN: {
			BUSY_RETRY(SendReturnBusy_wr2());

			SendReturn_wr2();
		}
		break;
		default:
			assert(0);
		}
	}
}
