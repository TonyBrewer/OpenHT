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
			S_multRdy = false;
			S_addRdy = false;
			S_muladdRdy = false;
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
	uint64_t i1_a = 0;
	uint64_t i1_b = 0;
	bool i1_vld = false;
	uint64_t i2_a = 0;
	uint64_t i2_b = 0;
	bool i2_vld = false;
	uint64_t i3_a = 0;
	uint64_t i3_b = 0;
	uint64_t i3_c = 0;
	bool i3_vld = false;

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
	    S_resCnt < 490 &&
	    SR_multRdy) {
		i1_a = (uint64_t)ReadStream_RdC();
		i1_b = (uint64_t)PR1_scalar;
		i1_vld = true;
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
	    S_resCnt < 490 &&
	    SR_addRdy) {
		i2_a = (uint64_t)ReadStream_RdA();
		i2_b = (uint64_t)ReadStream_RdB();
		i2_vld = true;
	}
	if (SR_opnVld == 1 &&
	    SR_opn == 2 &&
	    S_res.empty() == false &&
	    WriteStreamReady_WrC()) {
		uint64_t c = S_res.front();
		WriteStream_WrC(c);
		S_res.pop();
		S_resCnt = S_resCnt - 1;
	}

	// Operation 3
	if (SR_opnVld == 1 &&
	    SR_opn == 3 &&
	    ReadStreamReady_RdB() &&
	    ReadStreamReady_RdC() &&
	    S_resCnt < 490 &&
	    SR_muladdRdy) {
		i3_a = (uint64_t)ReadStream_RdB();
		i3_b = (uint64_t)ReadStream_RdC();
		i3_c = (uint64_t)PR1_scalar;
		i3_vld = true;
	}
	if (SR_opnVld == 1 &&
	    SR_opn == 3 &&
	    S_res.empty() == false &&
	    WriteStreamReady_WrA()) {
		uint64_t a = (uint64_t)S_res.front();
		WriteStream_WrA(a);
		S_res.pop();
		S_resCnt = S_resCnt - 1;
	}


	// Temporary variables to use as outputs to the primitive
	// (these are not saved between cycles)
	uint64_t o1_res;
	bool o1_rdy;
	bool o1_vld;
	uint64_t o2_res;
	bool o2_rdy;
	bool o2_vld;
	uint64_t o3_res;
	bool o3_rdy;
	bool o3_vld;

	// Instantiate clocked primitive
	mult_wrap(GR_htReset, i1_a, i1_b, i1_vld, o1_rdy, o1_res, o1_vld, mult_prm_state1);
	S_multRdy = o1_rdy;

	// Check for valid outputs from the primitive
	if (o1_vld) {

		// Store Result into shared ram to be written to memory later
		S_res.push(o1_res);
		S_resCnt = S_resCnt + 1;
	}

	// Instantiate clocked primitive
	add_wrap(GR_htReset, i2_a, i2_b, i2_vld, o2_rdy, o2_res, o2_vld, add_prm_state1);
	S_addRdy = o2_rdy;

	// Check for valid outputs from the primitive
	if (o2_vld) {

		// Store Result into shared ram to be written to memory later
		S_res.push(o2_res);
		S_resCnt = S_resCnt + 1;
	}

	// Instantiate clocked primitive
	muladd_wrap(GR_htReset, i3_a, i3_b, i3_c, i3_vld, o3_rdy, o3_res, o3_vld, muladd_prm_state1);
	S_muladdRdy = o3_rdy;

	// Check for valid outputs from the primitive
	if (o3_vld) {

		// Store Result into shared ram to be written to memory later
		S_res.push(o3_res);
		S_resCnt = S_resCnt + 1;
	}
}
