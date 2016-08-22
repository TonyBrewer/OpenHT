#include "Ht.h"
using namespace Ht;

#ifdef HT_VSIM
#define NMSGS 10
#else
#ifdef HT_SYSC
#define NMSGS 100
#else
#define NMSGS 10000
#endif
#endif

uint64_t delta_usec(uint64_t *usr_stamp);

int main(int argc, char **argv)
{
	uint64_t stop;

	printf("Coherence Microbenchmark\n");
	printf("  Running with NMSGS = %llu\n", (long long)NMSGS);
	fflush(stdout);

	// Get Coprocessor / Unit handles
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit * pAuUnit = new CHtAuUnit(pHtHif);

	fflush(stdout);


	// Initialize Data
	uint64_t msgs[NMSGS];
	uint64_t cp_msgs[NMSGS];
	for (int i = 0; i < NMSGS; i++) {
		msgs[i] = i;
		cp_msgs[i] = -1;
	}


	// Send Messages
	delta_usec(NULL);
	for (int i = 0; i < NMSGS; i++) {
		pAuUnit->SendHostMsg(TO_CP_MSG, (uint64_t)msgs[i]);
		uint8_t msgType;
		while (!pAuUnit->RecvHostMsg(msgType, cp_msgs[i]) || msgType != TO_HOST_MSG)
			usleep(1000);
	}
	stop = delta_usec(NULL);
	printf("Completed...\n");

	// Print Stats
	printf("\n");
	printf("Avg time per round-trip message (usecs)\n");
	printf("%11.4f\n",
	       (double)(stop/NMSGS));
	printf("\n");

	// check results
	printf("\n");
	int errCnt = 0;
	for (int i = 0; i < NMSGS; i++) {
#ifdef HT_VSIM
		printf("  MSG %d\n", i);
#endif
		if (msgs[i] != cp_msgs[i]) {
			fprintf(stderr, "ERROR - cp_msgs[%d] = %ld (expected %ld)\n", i, cp_msgs[i], msgs[i]);
			errCnt++;
		}
	}
	printf("\n");
	if (errCnt) {
		fprintf(stderr, "FAILED (errCnt = %d)\n", errCnt);
	} else {
		printf("PASSED\n");
	} 
	printf("\n");

	delete pAuUnit;
	delete pHtHif;

	return 0;
}
