#include <string.h>

#include "Ht.h"
using namespace Ht;

#define THREAD_CNT 32

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	int callCnt = 0;
	int rtnCnt = 0;
	int err = 0;
	while (callCnt < THREAD_CNT || rtnCnt < THREAD_CNT) {
		if (callCnt < THREAD_CNT && pAuUnit->SendCall_htmain())
			callCnt += 1;

		int16_t cnt;
		if (pAuUnit->RecvReturn_htmain(cnt)) {
			if (cnt != THREAD_CNT)
				err += 1;
			rtnCnt += 1;
		}
	}

	delete pHtHif;

	if (err > 0)
		printf("FAILED\n");
	else
		printf("PASSED\n");

	return err;
}
