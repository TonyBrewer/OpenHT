#include "Ht.h"
using namespace Ht;

// Defines
#ifndef SEED
#  define SEED -1
#endif

#ifdef HT_VSIM
#  define MAX_WORDS 50
#else
#ifdef HT_SYSC
#  define MAX_WORDS 1000
#else
#ifdef HT_MODEL
#  define MAX_WORDS 1000000
#endif
#endif
#endif


// State Struct to keep track of information per AU
typedef struct state {
	bool	   done;      // Flag - AU completed all tasks
	bool	   doneCall;  // Flag - AU completed call, do send/recv

	uint32_t  numWords; // Number of 64-bit data words to send
	uint32_t  numSent;  // Count of 64-bit words already sent

	uint64_t lastRecv;  // Last 64-bit word received from this AU
} state_t;


// Main Program
int main(int argc, char **argv) {

	uint32_t errCnt = 0;
	uint16_t numAuActive = 0;
	uint16_t nau = 0;
	uint16_t au = 0;

	CHtHif *pHtHif = new CHtHif();
	nau = pHtHif->GetUnitCnt();
	CHtAuUnit ** pAuUnits = new CHtAuUnit * [nau];

	for (int unitId = 0; unitId < nau; unitId += 1)
		pAuUnits[unitId] = new CHtAuUnit(pHtHif);

	printf("\nStarting streaming example with %d AUs\n", nau);

	// Print Starting Time
	time_t curTime = time(NULL);
	printf("Start time = %s", ctime(&curTime));


	// Setup Random number seed (for repeatability)
	struct timeval st;
	gettimeofday(&st, NULL);
	unsigned int seed = (SEED != -1) ? SEED : (int)st.tv_usec;
	printf("Seed = 0x%x\n\n", seed);
	srand(seed);


	// Initialize State structure for each available AU
	state_t *auState = (state_t *)calloc(nau, sizeof(state_t));
	for (au = 0; au < nau; au++) {
		numAuActive++;
		auState[au].done = false;
		auState[au].doneCall = false;
		auState[au].numWords = (rand() % (MAX_WORDS)) + 1;
		auState[au].numSent = 0;
		auState[au].lastRecv = ((au & 0xFFFFLL)<<48);
	}


	// Do Call/Send/Receive/Return...
	while(numAuActive) {

		for (au = 0; au < nau; au++) {

			uint64_t data;

			// AU complete, skip to next AU
			if (auState[au].done == true) {
				continue;
			}


			// Call
			if (auState[au].doneCall == false) {

				// Initial call to coprocessor, sends AU number and word count for basic checking
				if (pAuUnits[au]->SendCall_htmain(au, auState[au].numWords)) {
					printf("Host:  AU %2d - Call\n  Word Count %d\n", au, auState[au].numWords);
					auState[au].doneCall = true;
				}
			}


			// Send
			if ((auState[au].doneCall == true) && (auState[au].numSent != auState[au].numWords)) {

				while (auState[au].numSent < auState[au].numWords) {

					// Create test send data (0xYYYYZZZZZZZZZZZZ, where Y = au number, Z = sent word count)
					data = 0LL;
					data |= ((au & 0xFFFFLL)<<48);
					data |= ((auState[au].numSent + 1) & 0xFFFFFFFFFFFFLL);

					if (pAuUnits[au]->SendHostData(1, &data)) {
						//printf("Host:  AU %2d - Sent\n  Data 0x%016llx\n", au, (unsigned long long)data);
						auState[au].numSent++;

						// Flush on last word sent
						if (auState[au].numSent == auState[au].numWords) {
							pAuUnits[au]->FlushHostData();
						}

					} else {
						break;
					}

				}

			}


			// Receive
			while (pAuUnits[au]->RecvHostData(1, &data)) {
				//printf("Host:  AU %2d - Received\n  Data 0x%016llx\n", au, (unsigned long long)data);
				if (data != auState[au].lastRecv+1) {
					printf("Host:  WARNING - Data returned out of order!\n");
					printf("         0x%016llx != 0x%016llx + 1\n",
						(unsigned long long)data, (unsigned long long)auState[au].lastRecv);
					errCnt++;
				}

				auState[au].lastRecv = data;
			}


			// Return
			uint32_t errs;
			if (pAuUnits[au]->RecvReturn_htmain(errs)) {
				printf("Host:  AU %2d - Return\n", au);

				errCnt += errs;

				numAuActive--;
				auState[au].done = true;
			}

		} // End for (au...)

	} // End while (numAuActive)


	// Print Ending Time
	curTime = time(NULL);
	printf("\nEnd time = %s\n", ctime(&curTime));

	// Free resources
	free(auState);

	for (int unitId = 0; unitId < nau; unitId += 1)
		delete pAuUnits[unitId];

	delete pHtHif;

	// Print Error Information
	if(errCnt == 0) {
		printf("PASSED\n\n");
	} else {
		printf("FAILED - saw %d errors\n\n", errCnt);
	}

	return errCnt;
}
