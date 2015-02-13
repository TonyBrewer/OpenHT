#include "Ht.h"
#include "PersGv.h"

void CPersGv::PersGv()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case GV_ENTRY:
		{
			CStruct_ghi ghi;
			ghi.m_g = 4;
			ghi.m_h = 6;
			ghi.m_i = 8;

			GW_gvar.write_addr(PR_htId & 7);

			GW_gvar.m_abc = 34;
			GW_gvar.m_def.m_x = (ht_uint5)(23 + PR_htId);
			GW_gvar.m_ghi = ghi;

			P_gvAddr = PR_htId & 7;

			HtContinue(GV_I1);
		}
		break;
		case GV_I1:
		{
			if (GR_gvar.m_abc != 34 || GR_gvar.m_def.m_x != 23 + PR_htId || GR_gvar.m_ghi.m_g != 4 || GR_gvar.m_ghi.m_h != 6 || GR_gvar.m_ghi.m_i != 8)
				HtAssert(0, 0);

			GW_gvar.write_addr(PR_htId & 7);
			GW_gvar.m_def.m_x = (ht_uint5)(3 + PR_htId);

			HtContinue(GV_I2);
		}
		break;
		case GV_I2:
		{
			if (GF_gvar.m_abc != 34 || GF_gvar.m_def.m_x != 23 + PR_htId || GF_gvar.m_ghi.m_g != 4 || GF_gvar.m_ghi.m_h != 6 || GF_gvar.m_ghi.m_i != 8)
				HtAssert(0, 0);

			if (GR_gvar.m_abc != 34 || GR_gvar.m_def.m_x != 3 + PR_htId || GR_gvar.m_ghi.m_g != 4 || GR_gvar.m_ghi.m_h != 6 || GR_gvar.m_ghi.m_i != 8)
				HtAssert(0, 0);

			GW_gvar.write_addr(PR_htId & 7);
			GW_gvar.m_def.m_x.AtomicInc();

			HtContinue(GV_RETURN);
		}
		break;
		case GV_RETURN:
		{
			if (SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			if (GF_gvar.m_abc != 34 || GF_gvar.m_def.m_x != 3 + PR_htId || GF_gvar.m_ghi.m_g != 4 || GF_gvar.m_ghi.m_h != 6 || GF_gvar.m_ghi.m_i != 8)
				HtAssert(0, 0);

			if (GR_gvar.m_abc != 34 || GR_gvar.m_def.m_x != 4 + PR_htId || GR_gvar.m_ghi.m_g != 4 || GR_gvar.m_ghi.m_h != 6 || GR_gvar.m_ghi.m_i != 8)
				HtAssert(0, 0);

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}
}
