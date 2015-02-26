#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	uint16_t inArray[11];
	uint16_t outArray[11];
	for (int i = 0; i < 11; i++) {
		inArray[i] = 0x6900 | i;
		outArray[i] = 0x5A5A;
	}

	Align16 inStruct[11];
	Align16 outStruct[11];
	for (int i = 0; i < 11; i += 1) {
		inStruct[i].m_i16[0] = 0x1110 | i;
		inStruct[i].m_i16[1] = 0x2220 | i;
		inStruct[i].m_i8[0] = 0x30 | i;
		inStruct[i].m_i8[1] = 0x40 | i;
		inStruct[i].m_i8[2] = 0x50 | i;
		inStruct[i].m_u16[0] = 0x6660 | i;
		inStruct[i].m_u16[1] = 0x7770 | i;
		inStruct[i].m_u16[2] = 0x8880 | i;
	}

	uint64_t inPrivArray[10];
	uint64_t outPrivArray[10];
	for (uint64_t i = 0; i < 10; i++) {
		inPrivArray[i] = 0x0f1e2d3c4b5a6900ULL | i;
		outPrivArray[i] = 0x5A5A5A5A5A5A5A5AULL;
	}

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	pUnit->SendCall_MemSub(inArray, outArray, &inStruct[3], &outStruct[3], &inPrivArray[2], &outPrivArray[2]);

	while (!pUnit->RecvReturn_MemSub())
		usleep(1);

	int err_cnt = 0;
	for (int i = 0; i < 11; i += 1) {
		if (outArray[i] != (0x6900 | i))
			err_cnt += 1;
	}

	for (int i = 3; i < 10; i += 1) {
		if (outStruct[i].m_i16[0] != (0x1110 | i)) err_cnt += 1;
		if (outStruct[i].m_i16[1] != (0x2220 | i)) err_cnt += 1;
		if (outStruct[i].m_i8[0] != (0x30 | i)) err_cnt += 1;
		if (outStruct[i].m_i8[1] != (0x40 | i)) err_cnt += 1;
		if (outStruct[i].m_i8[2] != (0x50 | i)) err_cnt += 1;
		if (outStruct[i].m_u16[0] != (0x6660 | i)) err_cnt += 1;
		if (outStruct[i].m_u16[1] != (0x7770 | i)) err_cnt += 1;
		if (outStruct[i].m_u16[2] != (0x8880 | i)) err_cnt += 1;
	}

	for (int i = 2; i < 8; i++) {
		if (outPrivArray[i] != (0x0f1e2d3c4b5a6900LL | i)) err_cnt += 1;
	}

	delete pHtHif;

	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}
