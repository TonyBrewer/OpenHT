#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>

#include "Ht.h"
#include "HtBcmLib.h"
#include "sha2.h"

void BcmReportNonce(void *, CHtBcmRslt *);
void BcmFreeTask(CHtBcmTask *);

bool g_bReportNonce = false;
int g_BcmFreeTaskCnt = 0;

#ifdef HT_APP
int init_nonce = 0x00000000;
int last_nonce = 0xffffffff;
int loopCnt = 1;
#else
int init_nonce = 0x42a14690;
int last_nonce = 0x42a1469f;
int loopCnt = 1;
#endif

// this is the block header, it is 80 bytes long (steal this code)
typedef struct block_header {
	uint8_t version[4];
	uint8_t prev_block[32];
	uint8_t merkle_root[32];
	uint8_t timestamp[4];
	uint8_t bits[4];
	uint8_t nonce[4];
} block_header;

void BcmHex2bin(unsigned char *pDst, char const *pSrc)
{
	int idx = 0;

	while (*pSrc != '\0') {
		uint8_t hex = (*pSrc >= '0' && *pSrc <= '9') ? (*pSrc - '0')
			      : (tolower(*pSrc) - 'a' + 10);
		if ((idx++ & 1) == 0)
			*pDst = hex << 4;
		else
			*pDst++ |= hex;

		pSrc += 1;
	}
}

#define BcmByteSwap16(value)  \
	((((value) & 0xff) << 8) | ((value) >> 8))

#define BcmByteSwap32(value)    \
	(((uint32_t)BcmByteSwap16((uint16_t)((value) & 0xffff)) << 16) | \
	 (uint32_t)BcmByteSwap16((uint16_t)((value) >> 16)))

static inline void BcmEndianFlip(uint8_t *pDst8, uint8_t const *pSrc8, int32_t size8)
{
	assert((size8 & 0x3) == 0);
	uint32_t *pDst32 = (uint32_t *)pDst8;
	const uint32_t *pSrc32 = (uint32_t const *)pSrc8;
	int i;

	for (i = 0; i < (size8 / 4); i++)
		pDst32[i] = BcmByteSwap32(pSrc32[i]);
}

static void BcmCalcMidState(uint8_t *pMidState, uint8_t const *pData8)
{
	unsigned char data[64];
	sha2_context ctx;

	BcmEndianFlip(data, pData8, 64);
	sha2_starts(&ctx);
	sha2_update(&ctx, data, 64);
	// Quick fix... endianness of midState should not be changed
	//BcmEndianFlip(pMidState, (uint8_t *)ctx.state, 32);
	memcpy(pMidState, ctx.state, 32);
}

//static void RegenHash(void * block)
//{
//	unsigned char swap[80];
//	unsigned char hash1[32];
//	unsigned char hash2[32];
//
//	BcmEndianFlip(swap, (uint8_t *)block, 80);
//	sha2(swap, 80, hash1);
//	sha2(hash1, 32, hash2);
//}

void BcmInitTarget(uint32_t *pTarget, uint32_t bits)
{
	memset(pTarget, 0, 32);

	int exp = bits >> 24;

	*(((uint8_t *)pTarget) + exp - 1) = (bits >> 16) & 0xff;
	*(((uint8_t *)pTarget) + exp - 2) = (bits >> 8) & 0xff;
	*(((uint8_t *)pTarget) + exp - 3) = (bits >> 0) & 0xff;

	for (int i = exp - 4; i >= 0; i -= 1)
		*(((uint8_t *)pTarget) + i) = 0xff;
}

CHtBcmTask **g_pBcmTask = NULL;
bool g_bError = true;


void usage()
{
	printf("Usage:\n");
	printf("  ./app [loopCnt:1]\n");
	printf("  loopCnt - Number of times to loop work through each unit\n\n");
	exit(1);
}


int main(int argc, char **argv)
{
	// Check arguments
	if (argc < 2) {
		// Default operation
	} else if (argc == 2) {
		// Check arg
		unsigned long arg = strtol(argv[1], NULL, 0);
		if (arg == 0)
			usage();
		else
			loopCnt = arg;
	}
	// Error
	else {
		usage();
	}


	printf("\nHtBcm Test\n\n");

	int nau = HtBcmPrepare(&BcmReportNonce, &BcmFreeTask);

	printf("  Running with %d Units\n", nau);
	printf("  Looping %d Times\n", loopCnt);

	// Alloc g_pBcmTask
	g_pBcmTask = (CHtBcmTask **)calloc(loopCnt * nau, sizeof(CHtBcmTask));

	// For each loop
	for (int j = 0; j < loopCnt; j += 1) {
		printf("\nLoop %d\n", j + 1);
		g_bReportNonce = false;
		g_BcmFreeTaskCnt = 0;

		// Create Task for each Unit
		for (int i = 0; i < nau; i++) {
			int cnt = i + j * nau + 1;
			int total = loopCnt * nau;
			printf("HtBcm Hash Instance %d of %d created\n", cnt, total);

			block_header header;

			BcmHex2bin(header.version, "01000000");
			BcmHex2bin(header.prev_block, "81cd02ab7e569e8bcd9317e2fe99f2de44d49ab2b8851ba4a308000000000000");
			BcmHex2bin(header.merkle_root, "e320b6c2fffc8d750423db8b1eb942ae710e951ed797f7affc8892b0f1fc122b");
			BcmHex2bin(header.timestamp, "c7f5d74d");
			BcmHex2bin(header.bits, "f2b9441a");
			BcmHex2bin(header.nonce, "42a14695");

			// put header in big endian format
			BcmEndianFlip((uint8_t *)&header, (uint8_t *)&header, 80);

			//RegenHash(&header);

			posix_memalign((void **)&g_pBcmTask[cnt - 1], 64, sizeof(CHtBcmTask) * 1);

			BcmCalcMidState(g_pBcmTask[cnt - 1]->m_midState, (uint8_t *)&header);

			memcpy(g_pBcmTask[cnt - 1]->m_data, ((uint8_t *)&header) + 64, 12);

			uint32_t target[8];
			uint32_t bits = BcmByteSwap32(*(uint32_t *)header.bits);
			BcmInitTarget(target, bits);

			memcpy(g_pBcmTask[cnt - 1]->m_target, target + 5, 12);

			g_pBcmTask[cnt - 1]->m_initNonce = init_nonce;
			g_pBcmTask[cnt - 1]->m_lastNonce = last_nonce;

			HtBcmAddNewTask(g_pBcmTask[cnt - 1]);
		}

		// Force ScanHash call until all units have returned
		while (!g_bReportNonce || (g_BcmFreeTaskCnt < nau))
			HtBcmScanHash((void *)1);

		for (int i = 0; i < nau; i++) {
			int cnt = i + j * nau + 1;
			free(g_pBcmTask[cnt - 1]);
		}
	}

	// All loops complete, finish up
	printf("%s\n", g_bError ? "FAILED" : "PASSED");

	free(g_pBcmTask);

	HtBcmShutdown();
}

void BcmReportNonce(void *pThread, CHtBcmRslt *pBcmRslt)
{
	g_bReportNonce = true;

	g_bError = memcmp(pBcmRslt, g_pBcmTask[0], 44) != 0 || pBcmRslt->m_nonce != 0x42a14695;

	if (g_bError) {
		printf("Report Nonce miscompare:\n");
		printf("         Actual      Expected\n");
		for (int i = 0; i < 11; i += 1) {
			printf("[%2d]   0x%08x   0x%08x   %s\n",
			       i,
			       ((uint32_t *)pBcmRslt)[i],
			       ((uint32_t *)g_pBcmTask[0])[i],
			       ((uint32_t *)pBcmRslt)[i] == ((uint32_t *)g_pBcmTask[0])[i] ? "" : "x"
			       );
		}
		printf("[11]   0x%08x   0x%08x   %s\n", pBcmRslt->m_nonce, 0x42a14695,
		       pBcmRslt->m_nonce == 0x42a14695 ? "" : "x");
		printf("\n");
	}
}

void BcmFreeTask(CHtBcmTask *)
{
	g_BcmFreeTaskCnt++;
}
