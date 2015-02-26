#include "Ht.h"
#include "PersMult.h"

// Include file with primitives
#include "PersMult_prim.h"

void CPersMult::PersMult() {

  S_op1Mem.read_addr(PR1_htId);
  S_op2Mem.read_addr(PR1_htId);

  // Private variables, always reflect 32-bit copy of op1 and op2
  P1_op1 = (uint32_t)S_op1Mem.read_mem();
  P1_op2 = (uint32_t)S_op2Mem.read_mem();

  // use clocked primitive
  multiplier(P1_op1, P1_op2, T6_res, mult_prm_state1);

  // Work to be done on Cycle 6
  if (PR6_htValid) {
    switch (PR6_htInst) {

    case MULT_LD1: {
      if (ReadMemBusy()) {
	HtRetry();
	break;
      }
      
      // Memory read request - Op1
      MemAddr_t memRdAddr = (ht_uint48)(SR_maBase + (PR6_rowIdx << 3) + ((SR_mcRow*PR6_calcIdx) << 3));
      ReadMem_op1Mem(memRdAddr, PR6_htId, 1);
      HtContinue(MULT_LD2);
    }
      break;

    case MULT_LD2: {
      if (ReadMemBusy()) {
	HtRetry();
	break;
      }
      
      // Memory read request - Op2
      MemAddr_t memRdAddr = (ht_uint48)(SR_mbBase + (PR6_eleIdx << 3) + ((SR_mcCol*PR6_calcIdx) << 3));
      ReadMem_op2Mem(memRdAddr, PR6_htId, 1);
      ReadMemPause(MULT_RTN);
    }
      break;

    case MULT_RTN: {
      if (SendReturnBusy_mult()) {
	HtRetry();
	break;
      }
      
      // Return finished product back to PersSub
      SendReturn_mult(T6_res);
    }
      break;
    default:
      assert(0);
    }
  }
}
