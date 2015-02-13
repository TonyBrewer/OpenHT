#include "Ht.h"
using namespace Ht;

void Ht::HtCoprocModel(Ht::CHtHifLibBase * pHtHifLibBase)
{
  CHtModelHif *pModel = new CHtModelHif(pHtHifLibBase);
  CHtModelAuUnit *pAuUnit = new CHtModelAuUnit(pModel);

  bool done = false;
  bool markerRcvd = false;
  uint32_t rcvByteCnt = 0;
  // Loop until all AUs are done echoing data
  while (!done) {
    if(pAuUnit->RecvCall_htmain()) {
      while (!markerRcvd) {
	uint64_t temp;
	if (pAuUnit->RecvHostData(1, &temp)) {
	  rcvByteCnt += 8;
	  while(!pAuUnit->SendHostData(1, &temp));
	}

	if (pAuUnit->RecvHostDataMarker()) {
	  markerRcvd = true;
	  while(!pAuUnit->SendReturn_htmain(rcvByteCnt));
	}
      }
    }

    done = pAuUnit->RecvHostHalt();
  }

  delete pAuUnit;
  delete pModel;

}
