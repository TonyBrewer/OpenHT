#include "Ht.h"
using namespace Ht;
#include <thread>

struct status_t {
  uint8_t lane_up;
  uint8_t chan_up;
  uint8_t corr_alm;
  uint8_t fatal_alm;
  uint32_t unused_0;
};

union msg_t {
  status_t status;
  uint64_t data;
};

volatile bool done[2];
volatile uint8_t fatal_alm[2];
volatile uint8_t corr_alm[2];
volatile uint8_t chan_up[2];
volatile uint8_t lane_up[2];

void thread_handler(CHtAuUnit *pAuUnit, int unitId, int *errCnt);
void status_handler(CHtAuUnit *pAuUnit0, CHtAuUnit *pAuUnit1);
void usage(char *);

int main(int argc, char **argv)
{
	uint64_t len;

	// check command line args
	if (argc == 1) {
		len = 100;  // default len
	} else if (argc == 2) {
		len = strtoull(argv[1], 0, 0);
		if (len <= 0) {
			usage(argv[0]);
			return 0;
		}
	} else {
		usage(argv[0]);
		return 0;
	}

	printf("Running with len = %llu\n", (long long)len);
	fflush(stdout);

	CHtHif *pHtHif = new CHtHif();
	int unitCnt = pHtHif->GetUnitCnt();
	printf("#AUs = %d\n", unitCnt);

	CHtAuUnit ** pAuUnits = new CHtAuUnit * [unitCnt];
	for (int unit = 0; unit < unitCnt; unit++)
		pAuUnits[unit] = new CHtAuUnit(pHtHif);

	// Send arguments to all units using messages
	pHtHif->SendAllHostMsg(LEN, (uint64_t)len);
	printf("DBG - MSG SENT\n");
	fflush(stdout);

	// Start Threads
	std::thread unitThreads[32];
	int errCnt[32];
	for (int i = 0; i < unitCnt; i++) {
		done[i] = false;
		fatal_alm[i] = 0;
		corr_alm[i] = 0;
		chan_up[i] = 0;
		lane_up[i] = 0;
		errCnt[i] = 0;
		unitThreads[i] = std::thread(thread_handler, pAuUnits[i], i, &errCnt[i]);
	}
	std::thread statusThread(status_handler, pAuUnits[0], pAuUnits[1]);

	// Artificially wait until some time has passed
	usleep(10000);

	// Wait for threads
	for (int i = 0; i < unitCnt; i++) {
		unitThreads[i].join();
	}
	statusThread.join();

	int totalErrCnt = 0;
	for (int i = 0; i < unitCnt; i++) {
		totalErrCnt += errCnt[i];
	}


	if (totalErrCnt)
		printf("\nFAILED (%d issues)\n", totalErrCnt);
	else
		printf("\nPASSED\n");

	delete pHtHif;

	return totalErrCnt;
}

void thread_handler(CHtAuUnit *pAuUnit, int unitId, int *errCnt) {

	// Send calls to unit
	pAuUnit->SendCall_htmain();
	printf("DBG[%d] - CALL SENT\n", unitId);
	fflush(stdout);

	// Wait for return
	uint64_t error[4];
	while (!pAuUnit->RecvReturn_htmain(error[0], error[1], error[2], error[3])) {
		uint8_t msgType;
		msg_t msgData;
		if (pAuUnit->RecvHostMsg(msgType, msgData.data)) {
			fatal_alm[unitId] = msgData.status.fatal_alm;
			corr_alm[unitId] = msgData.status.corr_alm;
			chan_up[unitId] = msgData.status.chan_up;
			lane_up[unitId] = msgData.status.lane_up;
		}
		usleep(1000);
	}

	for (int i = 0; i < 4; i++) {
		if (error[i] != 0) {
			printf("ERROR[%d]: Found %ld mismatches on lane %d!\n", unitId, error[i], i);
			errCnt += error[i];
		}
	}

	done[unitId] = true;
}

void status_handler(CHtAuUnit *pAuUnit0, CHtAuUnit *pAuUnit1) {
	while (!(done[0] & done[1])) {
		printf("\33[2K\r");
		printf("Status: [0]: Fatal: 0x%02X, Corr: 0x%02X, ChanUp: 0x%02X, LaneUp: 0x%02X  [1]: Fatal: 0x%02X, Corr: 0x%02X, ChanUp: 0x%02X, LaneUp: 0x%02X",
		       fatal_alm[0],
		       corr_alm[0],
		       chan_up[0],
		       lane_up[0],
		       fatal_alm[1],
		       corr_alm[1],
		       chan_up[1],
		       lane_up[1]);
		fflush(stdout);
		usleep(1000);
	}
}

// Print usage message and exit with error.
void
usage(char *p)
{
	printf("usage: %s [count (default 100)] \n", p);
	exit(1);
}
