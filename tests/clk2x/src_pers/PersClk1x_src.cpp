#include "Ht.h"
#include "PersClk1x.h"

void CPersClk1x::PersClk1x()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case CLK1X_ENTRY:
			{
#define BW 2
#define D2O(d) (BW+(d)+1)
#define I 4
#define J (BW*2+3)
				ht_uint3 S_workSi[I][J];
				ht_uint3 S_workSj[I][J];
				for (int i = 0; i < I; i += 1)
					for (int j = 0; j < J; j += 1) {
						S_workSi[i][j] = 0;
						S_workSj[i][j] = 0;
					}
				ht_uint3 SR_lastWorkSi = 0;
				ht_uint3 SR_lastWorkSj = 0;

				for (int i=0; i<4; i++) {
					for (int x=-BW; x<BW+1; x++) {
						S_workSi[i][(ht_uint7)D2O(x)] = S_workSi[i][(ht_uint7)D2O(x+1)];
						S_workSj[i][(ht_uint7)D2O(-x)] = S_workSj[i][(ht_uint7)D2O(-x-1)];
					}
					S_workSi[i][(ht_uint7)D2O(BW)] = SR_lastWorkSi;
					S_workSj[i][(ht_uint7)D2O(-BW)] = SR_lastWorkSj;
				}

				ht_uint3 sum = 0;
				for (int i = 0; i < I; i += 1)
					for (int j = 0; j < J; j += 1) {
						sum += S_workSi[i][j];
						sum += S_workSj[i][j];
					}

				HtContinue(CLK1X_RTN);
			}
			break;
		case CLK1X_RTN:
			{
				if (SendReturnBusy_clk1x()) {
					HtRetry();
					break;
				}

				SendReturn_clk1x();
			}
			break;
		default:
			assert(0);
		}
	}
}
