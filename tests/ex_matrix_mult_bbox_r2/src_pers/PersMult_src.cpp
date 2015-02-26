#include "Ht.h"
#include "PersMult.h"

// Include file with primitives
#include "PersMult_prim.h"

void CPersMult::PersMult() {

  // Always set S_op1Mem, S_op2Mem to the proper address (every clock cycle)
  S_op1Mem.read_addr(PR1_htId);
  S_op2Mem.read_addr(PR1_htId);

  // Work to be done on Cycle 1
  if (PR1_htValid) {
    switch (PR1_htInst) {

    case MULT_LD1: {
      if (ReadMemBusy()) {
	HtRetry();
	break;
      }
      
      // Memory read request - Op1
      MemAddr_t memRdAddr = (ht_uint48)(SR_maBase + (PR1_rowIdx << 3) + ((SR_mcRow*PR1_calcIdx) << 3));
      ReadMem_op1Mem(memRdAddr, PR1_htId, 1);
      HtContinue(MULT_LD2);
    }
      break;

    case MULT_LD2: {
      if (ReadMemBusy()) {
	HtRetry();
	break;
      }
      
      // Memory read request - Op2
      MemAddr_t memRdAddr = (ht_uint48)(SR_mbBase + (PR1_eleIdx << 3) + ((SR_mcCol*PR1_calcIdx) << 3));
      ReadMem_op2Mem(memRdAddr, PR1_htId, 1);
      ReadMemPause(MULT_CALC);
    }
      break;

    case MULT_CALC: {

      // Setup private variables, always reflect 32-bit copy of op1 and op2
      // Used as inputs to the black box multiplier
      P1_op1 = (uint32_t)S_op1Mem.read_mem();
      P1_op2 = (uint32_t)S_op2Mem.read_mem();
      HtContinue(MULT_RTN);
    }
      break;

    case MULT_RTN: {
      if (SendReturnBusy_mult()) {
	HtRetry();
	break;
      }
      
      // Return finished product back to PersSub
      SendReturn_mult(PR1_result);
    }
      break;

    default:
      assert(0);
    }
  }


  // Instantiate primitive - outputs valid 5 cycles later on Stage 6
  multiplier(P1_op1, P1_op2, T6_res, mult_prm_state1);


  // Work to be done on Cycle 6
  if (PR6_htValid) {
    switch (PR6_htInst) {

    case MULT_LD1: {
      // Do Nothing
    }
      break;

    case MULT_LD2: {
      // Do Nothing
    }
      break;

    case MULT_CALC: {
      // Save result into a private variable so it can be reread when reentering the pipline
      P6_result = T6_res;
    }
      break;

    case MULT_RTN: {
      // Do Nothing
    }
      break;

    default:
      assert(0);
    }
  }

}
