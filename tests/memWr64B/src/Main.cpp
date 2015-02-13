#include "Ht.h"
using namespace Ht;

#define TEST_MODES 9

// set FIRST and LAST to run a subset of the tests
#define TEST_IDX_FIRST 0x0
//#define TEST_IDX_LAST (TEST_IDX_FIRST+1)
#define TEST_IDX_LAST testCnt

int main(int argc, char **argv)
{
	struct CTest {
		int m_offset;
		int m_qwCnt;
	} tests[64];

	int testCnt = 0;
	for (int i = 0; i < 8; i += 1) {
		for (int j = i+1; j <= 8; j += 1) {
			tests[testCnt].m_offset = testCnt*8+i;
			tests[testCnt].m_qwCnt = j-i;
			testCnt += 1;
		}
	}

	uint64_t * pHostRdArr;
	ht_posix_memalign((void **)&pHostRdArr, 64, testCnt*64);

	uint64_t * pHostWrArr;
	ht_posix_memalign((void **)&pHostWrArr, 64, testCnt*64);

	CHtHif *pHtHif = new CHtHif();

	uint64_t * pCoprocRdArr = (uint64_t*)pHtHif->MemAllocAlign(64, testCnt * 64);
	uint64_t * pCoprocWrArr = (uint64_t*)pHtHif->MemAllocAlign(64, testCnt * 64);

	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	pHtHif->SendAllHostMsg(HOST_RD_ADDR, (uint64_t)pHostRdArr);
	pHtHif->SendAllHostMsg(HOST_WR_ADDR, (uint64_t)pHostWrArr);

	pHtHif->SendAllHostMsg(COPROC_RD_ADDR, (uint64_t)pCoprocRdArr);
	pHtHif->SendAllHostMsg(COPROC_WR_ADDR, (uint64_t)pCoprocWrArr);

	int errorCnt = 0;
	for (int testMode = 0; testMode < TEST_MODES; testMode += 1) {
		if (testMode == 5)	// mode 5 writes to shared queue and only works on hc-W
			testMode += 1;

		bool bCoproc = testMode < 1 || testMode == 8;

		uint64_t * pRdArr = bCoproc ? pCoprocRdArr : pHostRdArr;
		uint64_t * pWrArr = bCoproc ? pCoprocWrArr : pHostWrArr;

		for (int i = 0; i < testCnt*8; i++) {
			pRdArr[i] = i | 0xdeadbeef00000000LL;
			pWrArr[i] = 0x0a0a0a0a0b0b0b0bLL;
		}

		// start all tests for test mode
		int testIdx = TEST_IDX_FIRST;
		int rtnCnt = TEST_IDX_FIRST;
		while (rtnCnt < TEST_IDX_LAST) {
			if (testIdx < TEST_IDX_LAST && pUnit->SendCall_htmain(testMode, tests[testIdx].m_offset, tests[testIdx].m_qwCnt)) {
				testIdx += 1;
				continue;
			}

			if (pUnit->RecvReturn_htmain()) {
				rtnCnt += 1;
				continue;
			}

			usleep(1);
		}

		//while (rtnCnt < testCnt) {
		//	while (!pUnit->SendCall_htmain(testMode, tests[testIdx].m_offset, tests[testIdx].m_qwCnt))
		//		usleep(1);

		//	while (!pUnit->RecvReturn_htmain())
		//		usleep(1);

		//	testIdx += 1;
		//	rtnCnt += 1;
		//}

		// check results
		for (int testIdx = TEST_IDX_FIRST; testIdx < TEST_IDX_LAST; testIdx += 1) {
			for (int i = 0; i < 8; i += 1) {
				if (testIdx*8+i < tests[testIdx].m_offset || testIdx*8+i >= tests[testIdx].m_offset + tests[testIdx].m_qwCnt) {
					if (pWrArr[testIdx*8+i] != 0x0a0a0a0a0b0b0b0bLL) {
						errorCnt += 1;
						printf("ERROR: *0x%016llx => 0x%016llx != 0x0a0a0a0a0b0b0b0b\n",
							(long long)&pWrArr[testIdx*8+i],
							(long long)pWrArr[testIdx*8+i]);
						fflush(stdout);
					}
				} else {
					if (pWrArr[testIdx*8+i] != pRdArr[testIdx*8+i]) {
						errorCnt += 1;
						printf("ERROR: *0x%016llx => 0x%016llx != *0x%016llx => 0x%016llx\n",
							(long long)&pWrArr[testIdx*8+i],
							(long long)pWrArr[testIdx*8+i],
							(long long)&pRdArr[testIdx*8+i],
							(long long)pRdArr[testIdx*8+i]);
						fflush(stdout);
					}
				}
			}
		}

		printf(" After test %d, errorCnt = %d\n", testMode, errorCnt);
		fflush(stdout);
	}

	delete pHtHif;

	if (errorCnt)
		printf("FAILED: detected %d errors!\n", errorCnt);
	else
		printf("PASSED\n");

	return errorCnt;
}
