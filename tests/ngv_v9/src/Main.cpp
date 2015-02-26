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

#	define GVAR_QW_SIZE (sizeof(CGVar) / 8)
#	define GVAR_BUF_QW_SIZE  (8 * GVAR_QW_SIZE)
#	define MEM_LINE_QW_SIZE 8

	uint64_t * pBuf = new uint64_t[CALL_CNT * (GVAR_BUF_QW_SIZE * 2) + MEM_LINE_QW_SIZE];
	pBuf = (uint64_t *)((uint64_t)(pBuf + MEM_LINE_QW_SIZE - 1) & ~0x3fULL);

	// initialize memory
	for (int gv2HtId = 0; gv2HtId < CALL_CNT; gv2HtId += 1) {
		for (int loop = 0; loop < 8; loop += 1) {

			CGVar * pGvar = (CGVar *)(pBuf + GVAR_BUF_QW_SIZE * (gv2HtId * 2 + 1) + (loop * sizeof(CGVar) / 8));

			pGvar->m_a = (char)(0x40 + loop + gv2HtId);
			pGvar->m_b[0] = (short)(0x4001 + loop + gv2HtId);
			pGvar->m_b[1] = (short)(0x4002 + loop + gv2HtId);
			pGvar->m_c[0].m_s = (char)(0x41 + loop + gv2HtId);
			pGvar->m_c[1].m_sa = (0 + gv2HtId) & 1;
			pGvar->m_c[1].m_sb = (5 + gv2HtId) & 7;
			pGvar->m_c[1].m_sc = (2 + gv2HtId) & 3;
			pGvar->m_c[2].m_s = 0 + gv2HtId;
			pGvar->m_d = 0x40000000 + loop + gv2HtId;

			for (int i = 0; i < 12; i += 1)
				pGvar->m_e[i] = 0x4000000000000000LL + loop + i + gv2HtId;
		}
	}

	int errCnt = 0;
	int callCnt = 0;
	int rtnCnt = 0;
	while (callCnt < 5 || rtnCnt < 5) {
		CGVar * pCallGvarRd = (CGVar *)(pBuf + GVAR_BUF_QW_SIZE * callCnt * 2);
		CGVar * pCallGvarWr = (CGVar *)(pBuf + GVAR_BUF_QW_SIZE * (callCnt * 2 + 1));
		if (callCnt < 5 && pUnit->SendCall_htmain(pCallGvarRd, pCallGvarWr))
			callCnt += 1;

		CGVar * pRtnGvar = (CGVar *)(pBuf + GVAR_BUF_QW_SIZE * rtnCnt * 2);
		uint16_t rtnErrCnt;
		if (rtnCnt < 5 && pUnit->RecvReturn_htmain(rtnErrCnt)) {
			errCnt += rtnErrCnt;
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
