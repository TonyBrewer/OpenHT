#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	uint32_t i;
	ArrayIdx_t xDimLen;
	ArrayIdx_t yDimLen = 51;
	void *a1, *a2, *a3;

	CHtHif *pHtHif = new CHtHif();
	int unitCnt = pHtHif->GetUnitCnt();
	printf("#AUs = %d\n", unitCnt);

	CHtAuUnit ** pAuUnits = new CHtAuUnit * [unitCnt];
	for (int unit = 0; unit < unitCnt; unit++)
		pAuUnits[unit] = new CHtAuUnit(pHtHif);

	uint8_t msgType;
	uint64_t msgData;
	for (int unit = 0; unit < unitCnt; unit++)
		while (!pAuUnits[unit]->RecvHostMsg(msgType, msgData) || msgType != VADD_TYPE_SIZE)
			usleep(1000);

	int typeSize = (int)msgData & 0xff;
	xDimLen = (int)(msgData >> 8) & 0xff;

	printf("Running with yDimLen = %u, xDimLen = %u\n", yDimLen, xDimLen);
	printf("Initializing arrays\n");
	fflush(stdout);

	a1 = CHtHif::HostMemAlloc(xDimLen * yDimLen * typeSize);
	a2 = CHtHif::HostMemAlloc(xDimLen * yDimLen * typeSize);
	a3 = CHtHif::HostMemAlloc(xDimLen * yDimLen * typeSize);
	memset(a3, 0, xDimLen * yDimLen * typeSize);

	for (i = 0; i < (uint32_t)xDimLen * yDimLen; i++) {
		switch (typeSize) {
		case 1:
			((uint8_t *)a1)[i] = i;
			((uint8_t *)a2)[i] = 2 * i;
			break;
		case 2:
			((uint16_t *)a1)[i] = i;
			((uint16_t *)a2)[i] = 2 * i;
			break;
		case 4:
			((uint32_t *)a1)[i] = i;
			((uint32_t *)a2)[i] = 2 * i;
			break;
		case 8:
			((uint64_t *)a1)[i] = i;
			((uint64_t *)a2)[i] = 2 * i;
			break;
		default:
			assert(0);
		}
	}

	// Coprocessor memory arrays
	void *cp_a1 = pHtHif->MemAlloc(xDimLen * yDimLen * typeSize);
	void *cp_a2 = pHtHif->MemAlloc(xDimLen * yDimLen * typeSize);
	void *cp_a3 = pHtHif->MemAlloc(xDimLen * yDimLen * typeSize);

	if (!cp_a1 || !cp_a2 || !cp_a3) {
		fprintf(stderr, "ht_cp_malloc() failed.\n");
		exit(-1);
	}

	pHtHif->MemCpy(cp_a1, a1, xDimLen * yDimLen * typeSize);
	pHtHif->MemCpy(cp_a2, a2, xDimLen * yDimLen * typeSize);
	pHtHif->MemSet(cp_a3, 0, xDimLen * yDimLen * typeSize);

	// Send arguments to all units using messages
	pHtHif->SendAllHostMsg(OP1_ADDR, (uint64_t)cp_a1);
	pHtHif->SendAllHostMsg(OP2_ADDR, (uint64_t)cp_a2);
	pHtHif->SendAllHostMsg(RES_ADDR, (uint64_t)cp_a3);

	// Send calls to units
	ArrayIdx_t unitYDimOff = 0;
	ArrayIdx_t unitYDimLen = (yDimLen + unitCnt - 1) / unitCnt;

	printf("Starting personality\n");

	for (int unit = 0; unit < unitCnt; unit++) {
		uint32_t unitAddrOff = unitYDimOff * xDimLen * typeSize;
		pAuUnits[unit]->SendCall_htmain(unitAddrOff, unitYDimLen, xDimLen);
		unitYDimOff += unitYDimLen;
		unitYDimLen = min(unitYDimLen, (ArrayIdx_t)(yDimLen - unitYDimOff));
	}

	// Wait for returns
	long long actualSum = 0;
	uint32_t unitSum;
	for (int unit = 0; unit < unitCnt; unit++) {
		while (!pAuUnits[unit]->RecvReturn_htmain(unitSum))
			usleep(1000);
		actualSum += unitSum;
	}

	printf("Checking results\n");

	pHtHif->MemCpy(a3, cp_a3, xDimLen * yDimLen * typeSize);

	// check results
	int errorCnt = 0;
	long long expectSum = 0;
	for (uint32_t y = 0; y < yDimLen; y++) {
		for (uint32_t x = 0; x < xDimLen; x++) {
			uint32_t i = y * xDimLen + x;
			long long exp, act1, act2, act3;
			switch (typeSize) {
			case 1:
				act1 = ((uint8_t*)a1)[i];
				act2 = ((uint8_t*)a2)[i];
				act3 = ((uint8_t*)a3)[i];
				exp = (uint8_t)(act1 + act2);
				break;
			case 2:
				act1 = ((uint16_t*)a1)[i];
				act2 = ((uint16_t*)a2)[i];
				act3 = ((uint16_t*)a3)[i];
				exp = (uint16_t)(act1 + act2);
				break;
			case 4:
				act1 = ((uint32_t*)a1)[i];
				act2 = ((uint32_t*)a2)[i];
				act3 = ((uint32_t*)a3)[i];
				exp = (uint32_t)(act1 + act2);
				break;
			case 8:
				act1 = ((uint64_t*)a1)[i];
				act2 = ((uint64_t*)a2)[i];
				act3 = ((uint64_t*)a3)[i];
				exp = (uint64_t)(act1 + act2);
				break;
			default:
				assert(0);
			}

			if (act3 != exp) {
				printf("a3[%u][%u] is 0x%llx, should be 0x%llx <= 0x%llx + 0x%llx\n",
					   y, x, act3, exp, act1, act2);
				errorCnt += 1;
			}
			expectSum += exp;
		}
	}
	if (actualSum != expectSum) {
		printf("act_sum %llu != exp_sum %llu\n", actualSum, expectSum);
		errorCnt += 1;
	}

	if (errorCnt)
		printf("FAILED: detected %d issues!\n", errorCnt);
	else
		printf("PASSED\n");

	// free memory
	CHtHif::HostMemFree(a1);
	CHtHif::HostMemFree(a2);
	CHtHif::HostMemFree(a3);
	pHtHif->MemFree(cp_a1);
	pHtHif->MemFree(cp_a2);
	pHtHif->MemFree(cp_a3);

	delete pHtHif;

	return errorCnt;
}
