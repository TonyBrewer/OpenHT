#include "Ht.h"
#include "PersCalc.h"

#define BUSY_RETRY(b) { if (b) { HtRetry(); break; } }

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

void
CPersCalc::PersCalc()
{
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case CALC_ENTER: {
			switch (PR1_operation) {
			case (0): {
				HtContinue(CALC_OPN1);
				S_opnVld = 1;
				S_opn = (ht_uint2)PR1_operation;
				break;
			}
			case (1): {
				HtContinue(CALC_OPN2);
				S_opnVld = 1;
				S_opn = (ht_uint2)PR1_operation;
				break;
			}
			case (2): {
				HtContinue(CALC_OPN3);
				S_opnVld = 1;
				S_opn = (ht_uint2)PR1_operation;
				break;
			}
			case (3): {
				HtContinue(CALC_OPN4);
				S_opnVld = 1;
				S_opn = (ht_uint2)PR1_operation;
				break;
			}
			default:
				assert(0);
			}
			break;
		}
		case CALC_OPN1: {
			BUSY_RETRY(ReadStreamBusy_RdA());
			BUSY_RETRY(WriteStreamBusy_WrC());

			if (!PR1_vecLen) {
				HtContinue(CALC_RETURN);
				break;
			}

			MemAddr_t addrA = SR_op1Addr + PR1_offset * sizeof(uint64_t);
			MemAddr_t addrC = SR_op3Addr + PR1_offset * sizeof(uint64_t);

			ReadStreamOpen_RdA(addrA, PR1_vecLen);
			WriteStreamOpen_WrC(addrC, PR1_vecLen);

			WriteStreamPause_WrC(CALC_RETURN);
			break;
		}
		case CALC_OPN2: {
			BUSY_RETRY(ReadStreamBusy_RdC());
			BUSY_RETRY(WriteStreamBusy_WrB());

			if (!PR1_vecLen) {
				HtContinue(CALC_RETURN);
				break;
			}

			MemAddr_t addrC = SR_op3Addr + PR1_offset * sizeof(uint64_t);
			MemAddr_t addrB = SR_op2Addr + PR1_offset * sizeof(uint64_t);

			ReadStreamOpen_RdC(addrC, PR1_vecLen);
			WriteStreamOpen_WrB(addrB, PR1_vecLen);

			WriteStreamPause_WrB(CALC_RETURN);
			break;
		}
		case CALC_OPN3: {
			BUSY_RETRY(ReadStreamBusy_RdA());
			BUSY_RETRY(ReadStreamBusy_RdB());
			BUSY_RETRY(WriteStreamBusy_WrC());

			if (!PR1_vecLen) {
				HtContinue(CALC_RETURN);
				break;
			}

			MemAddr_t addrA = SR_op1Addr + PR1_offset * sizeof(uint64_t);
			MemAddr_t addrB = SR_op2Addr + PR1_offset * sizeof(uint64_t);
			MemAddr_t addrC = SR_op3Addr + PR1_offset * sizeof(uint64_t);

			ReadStreamOpen_RdA(addrA, PR1_vecLen);
			ReadStreamOpen_RdB(addrB, PR1_vecLen);
			WriteStreamOpen_WrC(addrC, PR1_vecLen);

			WriteStreamPause_WrC(CALC_RETURN);
			break;
		}
		case CALC_OPN4: {
			BUSY_RETRY(ReadStreamBusy_RdB());
			BUSY_RETRY(ReadStreamBusy_RdC());
			BUSY_RETRY(WriteStreamBusy_WrA());

			if (!PR1_vecLen) {
				HtContinue(CALC_RETURN);
				break;
			}

			MemAddr_t addrB = SR_op2Addr + PR1_offset * sizeof(uint64_t);
			MemAddr_t addrC = SR_op3Addr + PR1_offset * sizeof(uint64_t);
			MemAddr_t addrA = SR_op1Addr + PR1_offset * sizeof(uint64_t);

			ReadStreamOpen_RdB(addrB, PR1_vecLen);
			ReadStreamOpen_RdC(addrC, PR1_vecLen);
			WriteStreamOpen_WrA(addrA, PR1_vecLen);

			WriteStreamPause_WrA(CALC_RETURN);
			break;
		}
		case CALC_RETURN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			S_opnVld = 0;
			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}

	// Temporary variables to use as input to the primitive
	// (these are not saved between cycles)
	uint64_t i_a = 0;
	uint64_t i_b = 0;
	bool i_vld = false;

	// Operation 0
	if (SR_opnVld == 1 &&
	    SR_opn == 0 &&
	    ReadStreamReady_RdA() &&
	    WriteStreamReady_WrC()) {
		uint64_t a = ReadStream_RdA();
		WriteStream_WrC(a);
	}

	// Operation 1
	if (SR_opnVld == 1 &&
	    SR_opn == 1 &&
	    ReadStreamReady_RdC() &&
	    S_resCnt < 490) {
		i_a = (uint64_t)ReadStream_RdC();
		i_b = (uint64_t)SR_scalar;
		i_vld = true;
	}
	if (SR_opnVld == 1 &&
	    SR_opn == 1 &&
	    S_res.empty() == false &&
	    WriteStreamReady_WrB()) {
		uint64_t a = S_res.front();
		WriteStream_WrB(a);
		S_res.pop();
		S_resCnt = S_resCnt - 1;
	}

	// Operation 2
	if (SR_opnVld == 1 &&
	    SR_opn == 2 &&
	    ReadStreamReady_RdA() &&
	    ReadStreamReady_RdB() &&
	    WriteStreamReady_WrC()) {
		uint64_t a = ReadStream_RdA();
		uint64_t b = ReadStream_RdB();
		WriteStream_WrC(a + b);
	}

	// Operation 3
	if (SR_opnVld == 1 &&
	    SR_opn == 3 &&
	    ReadStreamReady_RdC() &&
	    S_resCnt < 490) {
		i_a = (uint64_t)ReadStream_RdC();
		i_b = (uint64_t)SR_scalar;
		i_vld = true;
	}
	if (SR_opnVld == 1 &&
	    SR_opn == 3 &&
	    S_res.empty() == false &&
	    ReadStreamReady_RdB() &&
	    WriteStreamReady_WrA()) {
		uint64_t a = (uint64_t)S_res.front();
		uint64_t b = (uint64_t)ReadStream_RdB();
		WriteStream_WrA(a + b);
		S_res.pop();
		S_resCnt = S_resCnt - 1;
	}


	// Temporary variables to use as outputs to the primitive
	// (these are not saved between cycles)
	uint64_t o_res;
	bool o_vld;

	// Instantiate clocked primitive
	mult_wrap(i_a, i_b, i_vld, o_res, o_vld, mult_prm_state1);

	// Check for valid outputs from the primitive
	if (o_vld) {

		// Store Result into shared ram to be written to memory later
		S_res.push(o_res);
		S_resCnt = S_resCnt + 1;
	}
}
