#include "Ht.h"
#include "PersRow.h"

void CPersRow::PersRow() {
  if (PR_htValid) {
    switch (PR_htInst) {

    case ROW_ENTRY: {
      if (SendCallBusy_sub()) {
	HtRetry();
	break;
      }
      
      // Generate a seperate thread for each matrix element in a single row
      if (P_eleIdx < SR_mcCol) {
	SendCallFork_sub(ROW_JOIN, P_rowIdx, P_eleIdx, 0);
	HtContinue(ROW_ENTRY);
	P_eleIdx += 1;
      } else {
	RecvReturnPause_sub(ROW_RTN);
      }
    }
      break;

    case ROW_JOIN: {
      RecvReturnJoin_sub();
    }
      break;

    case ROW_RTN: {
      if (SendReturnBusy_row()) {
	HtRetry();
	break;
      }
      
      // Finished calculating matrix row
      SendReturn_row();
    }
      break;

    default:
      assert(0);

    }
  }
}
