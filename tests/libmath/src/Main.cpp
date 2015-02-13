#include <limits.h>

#include "Ht.h"
using namespace Ht;

struct tests_s {
	uint8_t  op;
	uint64_t op1;
	uint64_t op2;
	uint64_t rtn1;
	uint64_t rtn2;
} Tests[] = {
	{0 /*OP_UDIV*/, 33, 3, 11, 0},
	{0 /*OP_UDIV*/, 1234567, 11, 112233, 4},
	{0 /*OP_UDIV*/, 11, 20, 0, 11},
	{0 /*OP_UDIV*/, 100001, 10, 10000, 1},
	{1 /*OP_SDIV*/, 33, -3, -11, 0},
	{1 /*OP_SDIV*/, -1234567, 11, -112233, 4},
	//{1 /*OP_SDIV*/, 0, 0, 0, 0},	// DBZ
	//{1 /*OP_SDIV*/, LLONG_MIN, -1, 0, 0},	// Overfow
	{2 /*OP_UDIV*/, 33, 3, 11, 0},
	{2 /*OP_UDIV*/, 1234567, 11, 112233, 4},
	{2 /*OP_UDIV*/, 11, 20, 0, 11},
	{2 /*OP_UDIV*/, 100001, 10, 10000, 1},
	{3 /*OP_SDIV*/, 33, -3, -11, 0},
	{3 /*OP_SDIV*/, -1234567, 11, -112233, 4},
	{0xff, 0, 0, 0, 0}
};

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	int err = 0;

	for (int i=0; Tests[i].op != 0xff; i++) {
		pAuUnit->SendCall_htmain(Tests[i].op,
					 Tests[i].op1,
					 Tests[i].op2);

		uint64_t rtn1, rtn2;
		while (!pAuUnit->RecvReturn_htmain(rtn1, rtn2)) usleep(1000);

		if (Tests[i].rtn1 != rtn1 || Tests[i].rtn2 != rtn2) {
			err += 1;

			printf("Test[%d] failed... rtn1=0x%llx rtn2=0x%llx exp: 0x%llx 0x%llx\n",
				i, (long long)rtn1, (long long)rtn2,
				(long long)Tests[i].rtn1,
				(long long)Tests[i].rtn2);
		}
	}

#define RND_TEST
#if defined(RND_TEST)
	#define SEED -1

        // Setup Random number seed (for repeatability)
	struct timeval st;
	gettimeofday(&st, NULL);
	unsigned int seed = (SEED != -1) ? SEED : (int)st.tv_usec;
	printf("Seed = 0x%x\n", seed);
	srandom(seed);
	
	for (int i=0; i < 200; i++) {
		uint64_t op  = (random() & 1) << 1;
		uint64_t op1 = random() & 0xffffffff;
		uint64_t op2 = random() & 0xffff;

		pAuUnit->SendCall_htmain(op, op1, op2);

		uint64_t rtn1, rtn2;
		while (!pAuUnit->RecvReturn_htmain(rtn1, rtn2)) usleep(1000);

		uint64_t div = (op & 1) ?
			(uint64_t)((int64_t)op1/(int64_t)op2) :
			op1/op2;
		uint64_t rem = (op & 1) ?
			(uint64_t)((int64_t)op1%(int64_t)op2) :
			op1%op2;

		if (div != rtn1 || rem != rtn2) {
			err += 1;

			printf("%lld / %lld failed... rtn1=0x%llx rtn2=0x%llx exp: 0x%llx 0x%llx\n",
				(long long)op1, (long long)op2,
				(long long)rtn1, (long long)rtn2,
				(long long)div, (long long)rem);
		}
	}
#endif

	delete pHtHif;

	if (err)
		printf("FAILED (err=%d)\n", err);
	else
		printf("PASSED\n");

	return err;
}
