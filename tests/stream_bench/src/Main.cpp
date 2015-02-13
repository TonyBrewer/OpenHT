#include "Ht.h"
using namespace Ht;

#include "time.h"
timespec tv_diff(timespec start, timespec end) {
	timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

#if defined(HT_MODEL)
uint64_t dataLength = 100000;
#endif
#if defined(HT_SYSC)
uint64_t dataLength = 10000;
#endif
#if defined(HT_VSIM)
uint64_t dataLength = 50;
#endif
#if defined(HT_APP)
uint64_t dataLength = 50000;
#endif

// Main Program
int main(int argc, char **argv) {

	// Init coprocessor - Alloc 1 AU
	CHtHif *pHtHif = new CHtHif();
	CHtAuUnit *pAuUnit = new CHtAuUnit(pHtHif);

	// Make initial call
	pAuUnit->SendCall_htmain();


	// Populate data array with information to send to coprocessor
	uint64_t *sendData;
	sendData = (uint64_t*)calloc(dataLength, sizeof(uint64_t));
	for (uint32_t i = 0; i < dataLength; i++) {
		sendData[i] = i;
	}
	uint64_t numSent = 0;
	uint64_t sentSum = 0;
	uint64_t numRcvd = 0;
	uint64_t rcvdSum = 0;

	// Print Starting Time
	timespec startTime;
	clock_gettime(CLOCK_REALTIME, &startTime);

	// Send / Recv data
	bool markerSent = false;
	while (numRcvd < dataLength) {
		uint64_t temp;
		if (pAuUnit->RecvHostData(1, &temp)) {
			rcvdSum += temp;
			numRcvd++;
		}

		if (numSent < dataLength && pAuUnit->SendHostData(1, &sendData[numSent])) {
			sentSum += sendData[numSent];
			numSent++;
		}

		if (numSent == dataLength && !markerSent) {
			pAuUnit->SendHostDataMarker();
			pAuUnit->FlushHostData();
			markerSent = true;
		}
	}

	// Return from coproccessor
	uint32_t byteCnt;
	while (!pAuUnit->RecvReturn_htmain(byteCnt)) {
		usleep(1000);
	}

	// Print Ending Time
	timespec endTime;
	clock_gettime(CLOCK_REALTIME, &endTime);

	timespec diff;
	diff = tv_diff(startTime, endTime);

	double sec = (double)diff.tv_sec + ((double)diff.tv_nsec / 1.0e9);

	printf("Elapsed time = %f sec\n", sec);
	printf("Bytes transferred/received = %d\n", byteCnt);

	double throughput = (double)byteCnt / sec;
	throughput /= 1000000.0;
	printf("Throughput = %f MB/s\n", throughput);


	if (byteCnt != 8*dataLength) {
	  printf("FAILED: ByteCnt Mismatch: Expected: %lld, Returned: %lld\n", (long long int)8*dataLength, (long long int)byteCnt);
	} else if (sentSum != rcvdSum) {
		printf("FAILED: Sum Mismatch: Sent: %lld, Received: %lld\n", (long long int)sentSum, (long long int)rcvdSum);
	} else {
		printf("PASSED\n");
	}

	// Free resources
	free(sendData);

	delete pAuUnit;
	delete pHtHif;

	return 0;
}
