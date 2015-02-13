#include "Ht.h"
using namespace Ht;

#ifdef HT_VSIM
# define QW_CNT 16
#else
# define QW_CNT 256
#endif
union TestData {
	uint8_t		m_uint8[8];
	uint16_t	m_uint16[4];
	uint32_t	m_uint32[2];
	uint64_t	m_uint64;
};

static TestData actual[QW_CNT], expected[QW_CNT];

int main(int argc, char **argv)
{
	for (int i = 0; i < QW_CNT; i++) {
		actual[i].m_uint64 = 0x5A5A5A5A5A5A5A5A;
		expected[i].m_uint64 = 0x5A5A5A5A5A5A5A5A;
	}

	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pUnit = new CHtAuUnit(pHtHif);

	printf("#AUs = %d\n", pHtHif->GetUnitCnt());
	fflush(stdout);

	uint64_t randomData = 0x0123456789ABCDEF;
	uint64_t randomMod = 13;

	int callCnt = 0;
	int returnCnt = 0;

	for (int i = 0; i < QW_CNT; i += 1) {
		bool bQuadWord = i % 5 == 0;

		if (bQuadWord) {
			callCnt += 1;
			while (pUnit->SendCall_WrSub( (uint64_t) &actual[i].m_uint64, randomData, 3) == false);

			expected[i].m_uint64 = randomData;
		} else {
			uint64_t addr32 = (uint64_t) &actual[i].m_uint32[(i>>2) & 1];
			uint64_t addr16 = (uint64_t) &actual[i].m_uint16[((i>>1) & 3) ^ 2];
			uint64_t addr8 = (uint64_t) &actual[i].m_uint8[(i & 7) ^ 6];

			callCnt += 3;
			while (pUnit->SendCall_WrSub( addr32, randomData, 2) == false);
			while (pUnit->SendCall_WrSub( addr16, randomData >> 32, 1) == false);
			while (pUnit->SendCall_WrSub( addr8, randomData >> 48, 0) == false);

			expected[i].m_uint32[(i>>2) & 1] = (uint32_t)randomData;
			expected[i].m_uint16[((i>>1) & 3) ^ 2] = (uint16_t)(randomData >> 32);
			expected[i].m_uint8[(i & 7) ^ 6] = (uint8_t)(randomData >> 48);
		}

		randomData += randomMod;

		bool boolVar;
		while (pUnit->RecvReturn_WrSub(boolVar))
			returnCnt += 1;
	}

	bool boolVar;
	while (returnCnt < callCnt) {
		if (pUnit->RecvReturn_WrSub(boolVar))
			returnCnt += 1;
	}

	delete pHtHif;

	// check results
	int err_cnt = 0;
	for (int i = 0; i < QW_CNT; i++) {
		if (actual[i].m_uint64 != expected[i].m_uint64) {
			printf("expected[%d].m_uint64 = 0x%llx\n", i, (long long)expected[i].m_uint64);
			printf("  actual[%d].m_uint64 = 0x%llx\n", i, (long long)actual[i].m_uint64);
			err_cnt++;
		}
	}

	if (err_cnt)
		printf("FAILED: detected %d issues!\n", err_cnt);
	else
		printf("PASSED\n");

	return err_cnt;
}
