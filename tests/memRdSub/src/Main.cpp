#include "Ht.h"
using namespace Ht;

#define QW_CNT 256
union TestData {
	uint8_t		m_uint8[8];
	uint16_t	m_uint16[4];
	uint32_t	m_uint32[2];
	uint64_t	m_uint64;
	int8_t		m_int8[8];
	int16_t		m_int16[4];
	int32_t		m_int32[2];
	int64_t		m_int64;
};

static TestData data[QW_CNT];

int main(int argc, char **argv)
{
	for (int i = 0; i < QW_CNT; i++) {
		data[i].m_uint64 = 0x5A5A5A5A5A5A5A5A;
	}

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	uint64_t randomData = 0x0123456789ABCDEF;
	uint64_t randomMod = 13;

	int callCnt = 0;
	int returnCnt = 0;
	int err_cnt = 0;

	for (int i = 0; i < QW_CNT; i += 1) {
		bool bQuadWord = i % 5 == 0;

		if (bQuadWord) {
			data[i].m_uint64 = randomData;

			callCnt += 1;
			if (i & 0x40)
				while (pUnit->SendCall_RdSub( (uint64_t) &data[i].m_uint64, data[i].m_uint64, 3) == false);
			else
				while (pUnit->SendCall_RdSub( (uint64_t) &data[i].m_int64, data[i].m_uint64, 7) == false);
		} else {
			uint64_t addr32 = (uint64_t) &data[i].m_uint32[(i>>2) & 1];
			uint64_t addr16 = (uint64_t) &data[i].m_uint16[((i>>1) & 3) ^ 2];
			uint64_t addr8 = (uint64_t) &data[i].m_uint8[(i & 7) ^ 6];

			data[i].m_uint32[(i>>2) & 1] = (uint32_t)randomData;
			data[i].m_uint16[((i>>1) & 3) ^ 2] = (uint16_t)(randomData >> 32);
			data[i].m_uint8[(i & 7) ^ 6] = (uint8_t)(randomData >> 48);

			callCnt += 3;
			if (i & 0x40) {
				while (pUnit->SendCall_RdSub( addr32, data[i].m_uint32[(i>>2) & 1], 2) == false);
				while (pUnit->SendCall_RdSub( addr16, data[i].m_uint16[((i>>1) & 3) ^ 2], 1) == false);
				while (pUnit->SendCall_RdSub( addr8, data[i].m_uint8[(i & 7) ^ 6], 0) == false);
			} else {
				while (pUnit->SendCall_RdSub( addr32, data[i].m_int32[(i>>2) & 1], 6) == false);
				while (pUnit->SendCall_RdSub( addr16, data[i].m_int16[((i>>1) & 3) ^ 2], 5) == false);
				while (pUnit->SendCall_RdSub( addr8, data[i].m_int8[(i & 7) ^ 6], 4) == false);
			}
		}

		randomData *= randomMod;

		uint64_t actual;
		uint64_t expected;
		while (pUnit->RecvReturn_RdSub(actual, expected)) {
			if (actual != expected) {
				printf("expected = 0x%llx\n", (long long)expected);
				printf("  actual = 0x%llx\n", (long long)actual);
				err_cnt++;
			}
			returnCnt += 1;
		}
	}

	uint64_t actual;
	uint64_t expected;
	while (returnCnt < callCnt) {
		if (pUnit->RecvReturn_RdSub(actual, expected)) {
			if (actual != expected) {
				printf("expected = 0x%llx\n", (long long)expected);
				printf("  actual = 0x%llx\n", (long long)actual);
				err_cnt++;
			}
			returnCnt += 1;
		}
	}

	delete pHtHif;

	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}
