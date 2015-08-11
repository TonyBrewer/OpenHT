#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	for (int testIdx = 0; testIdx < 2; testIdx += 1) {
		if (testIdx == 0) {

			uint64_t rdData[256]; // Max 32 Cycles Per Test

			for (int i = 0; i < 256; i += 1)
				rdData[i] = 123 + i;

			for (int t = 0; t < 8; t += 1) {
				while (!pUnit->SendCall_main(0, t, rdData + t))
					usleep(1);
			}

			for (int t = 0; t < 8; t += 1) {
				while (!pUnit->RecvReturn_main())
					usleep(1);
			}
		} else {
			uint64_t wrData[8][10000];

			for (int t = 0; t < 8; t += 1) {
				while (!pUnit->SendCall_main(1, t, &wrData[t][0] + t))
					usleep(1);
			}

			for (int t = 0; t < 8; t += 1) {
				while (!pUnit->RecvReturn_main())
					usleep(1);
			}

			for (int t = 0; t < 8; t += 1) {
				for (int i = 0; i < 9216; i += 1) {
					if (wrData[t][i + t] != (((uint64_t)(&wrData[t][i + t]) >> 3) & 0x7f))
						assert(0);
				}
			}
		}
	}

	delete pUnit;
	delete pHtHif;

	printf("PASSED\n");

	return 0;
}
