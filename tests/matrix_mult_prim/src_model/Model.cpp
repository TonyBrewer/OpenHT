#include "Ht.h"
using namespace Ht;

void Ht::HtCoprocModel(Ht::CHtHifLibBase * pHtHifLibBase) {
  CHtModelHif *pModel = new CHtModelHif(pHtHifLibBase);
  int nau = pModel->GetUnitCnt();

  CHtModelAuUnit ** pAuUnits = new CHtModelAuUnit *;
  for (int i = 0; i < nau; i += 1)
    pAuUnits[i] = new CHtModelAuUnit(pModel);
  
  unsigned int i, j;

  int debug = 0;
  
  uint8_t msgType;
  uint64_t msgData;
  uint32_t rowIdx, stride;
  uint64_t *op1Base = NULL, *op2Base = NULL, *resBase = NULL;
  uint64_t *op1Addr = NULL, *op2Addr = NULL, *resAddr = NULL;
  uint64_t mcRow = 0, mcCol = 0, comRowCol = 0;
  uint64_t op1, op2, sum;

  printf("nau = %d\n", nau);

  int haltCnt = 0;
  while (haltCnt < nau) {
    for (int au = 0; au < nau; au += 1) {
      if (pAuUnits[au]->RecvHostMsg(msgType, msgData)) {
	switch (msgType) {
	case MA_BASE: op1Base = (uint64_t *)msgData; break;
	case MB_BASE: op2Base = (uint64_t *)msgData; break;
	case MC_BASE: resBase = (uint64_t *)msgData; break;
	case MC_ROW: mcRow = msgData; break;
	case MC_COL: mcCol = msgData; break;
	case COMMON: comRowCol = msgData; break;
	default: assert(0);
	}
      }
      if (pAuUnits[au]->RecvCall_htmain(rowIdx, stride)) {
	      
	// Loop over entire dataset
	while (rowIdx < mcRow) {

	  op1Addr = op1Base + rowIdx;         // Get pos of Row in Matrix A
	  op2Addr = op2Base;                  // Get pos of Col in Matrix B
	  resAddr = resBase + (rowIdx*mcCol); // Get pos of Row in Matrix C

	  if (debug)
	    printf("Working Row: %d | mcRow = %lld\n", rowIdx, (unsigned long long)mcRow);
		
	  // Loop over each element in the row
	  for (i = 0; i < mcCol; i++) {

	    if (debug)
	      printf("Column: %d\n",i);
		  
	    // Loop over each calculation per row entry
	    sum = 0;
	    for (j = 0; j < comRowCol; j++) {

	      if (debug)
		printf("Addr: %lld : %lld   ", (unsigned long long)op1Addr, (unsigned long long)op2Addr);

	      op1 = *op1Addr;
	      op2 = *op2Addr;
	      sum += op1*op2;

	      if (debug)
		printf("Data: %lld : %lld\n", (unsigned long long)op1, (unsigned long long)op2);

	      op1Addr += mcRow;
	      op2Addr += mcCol;
	      

	    }

	    // Store Matrix C element
	    *resAddr = sum;
		  
	    // Increment to next element in same row in Matrix A
	    // Increment to next column in Matrix B
	    // Increment to next element in same row in Matrix C
	    op1Addr = op1Base + rowIdx;
	    op2Addr = op2Base + (i+1);
	    resAddr++;
		  
	  }
		
	  // Go to next available row
	  rowIdx += stride;
		
	}
	      
	while (!pAuUnits[au]->SendReturn_htmain()) ;
      }
      haltCnt += pAuUnits[au]->RecvHostHalt();
    }
  }
  delete pModel;
}
