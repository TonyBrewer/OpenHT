#include "Gvar.h"

#include "Ht.h"
using namespace Ht;

#define CALL_CNT 5

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	#define GVAR_BUF_QW_SIZE  (8 * sizeof(CGVar) + 1)
#	define MEM_LINE_QW_SIZE 8

	uint64_t * pBuf = new uint64_t[CALL_CNT * GVAR_BUF_QW_SIZE + MEM_LINE_QW_SIZE];
	pBuf = (uint64_t *)((uint64_t)(pBuf + MEM_LINE_QW_SIZE - 1) & ~0x3fULL);

	int errCnt = 0;
	int callCnt = 0;
	int rtnCnt = 0;
	while (callCnt < 5 || rtnCnt < 5) {
		CGVar * pCallGvar = (CGVar *)(pBuf + GVAR_BUF_QW_SIZE * callCnt);
		if (callCnt < 5 && pUnit->SendCall_htmain(pCallGvar))
			callCnt += 1;

		CGVar * pRtnGvar = (CGVar *)(pBuf + GVAR_BUF_QW_SIZE * rtnCnt);
		if (rtnCnt < 5 && pUnit->RecvReturn_htmain()) {
			for (int i = 0; i < 8; i += 1) {
				if (pRtnGvar[i].m_a != 0x40 + i)
					errCnt += 1;
				if (pRtnGvar[i].m_b[0] != 0x4001 + i)
					errCnt += 1;
				if (pRtnGvar[i].m_b[1] != 0x4002 + i)
					errCnt += 1;
				if (pRtnGvar[i].m_c[0].m_s != 0x41 + i)
					errCnt += 1;
				if (pRtnGvar[i].m_c[1].m_sa != 0)
					errCnt += 1;
				if (pRtnGvar[i].m_c[1].m_sb != -3)
					errCnt += 1;
				if (pRtnGvar[i].m_c[1].m_sc != -2)
					errCnt += 1;
				if (pRtnGvar[i].m_c[2].m_s != 0)
					errCnt += 1;
				if (pRtnGvar[i].m_d != 0x40000000 + i)
					errCnt += 1;
				for (int j = 0; j < 12; j += 1) {
					if (pRtnGvar[i].m_e[j] != 0x4000000000000000LL + i + j)
						errCnt += 1;
				}
			}
			rtnCnt += 1;
		}

		usleep(1);
	}

	delete pUnit;
	delete pHtHif;

	if (errCnt)
		printf("FAILED: detected %d issues!\n", errCnt);
	else
		printf("PASSED\n");

	return errCnt;
}
