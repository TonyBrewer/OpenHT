#include "Ht.h"

void HtCoprocModel()
{
  uint32_t haltCnt = 0;
  uint16_t nau = 0;
  uint16_t au = 0;

  CHtModelHif *pModel = new CHtModelHif;
  CHtModelAuUnit *const *pAuUnits = pModel->AllocAllAuUnits();
  nau = pModel->GetAuUnitCnt();

  uint16_t rcvAu;
  uint32_t rcvCnt;
  
  // Loop until all AUs are done echoing data
  while (haltCnt < nau) {
    for (au = 0; au < nau; au++) {

      uint32_t wordCnt = 0;
      uint64_t recvData = 0;
      int32_t errs = 0;

      if(pAuUnits[au]->RecvCall_htmain(rcvAu, rcvCnt)) {

	printf("Model: AU %2d - Processing\n", rcvAu);

	while (wordCnt < rcvCnt) {

	  // Rerun loop if no data to read
	  if (!pAuUnits[au]->RecvHostData(1, &recvData)) {
	    continue;
	  }

	  // At this point, there is valid data in recvData
	  // Generate an expected response and compare
	  uint64_t expectedData = 0;
	  expectedData |= ((rcvAu & 0xFFFFLL)<<48);
	  expectedData |= ((wordCnt + 1) & 0xFFFFFFFFFFFFLL);

	  if (expectedData != recvData) {
	    printf("Model: WARNING - Expected Data did not match Received data!\n");
	    printf("         0x%016llx != 0x%016llx\n",
		   (unsigned long long)expectedData, (unsigned long long)recvData);
	    errs++;
	  }

	  // Send it back!
	  while (!pAuUnits[au]->SendHostData(1, &expectedData));

	  wordCnt++;

	}

	while (!pAuUnits[au]->SendReturn_htmain(errs));

      }

      haltCnt += pAuUnits[au]->RecvHostHalt();

    }
  }

  pModel->FreeAllAuUnits();
  delete pModel;

}
