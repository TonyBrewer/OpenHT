#include "Ht.h"
#include "PersStencil.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

void
CPersStencil::PersStencil()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case STENCIL_ENTER: {
			// Split offset calucation from source stencil to destination location over
			// two cytles for timing. OFF = ((Y_ORIGIN * (PR_cols + X_SIZE-1) + X_ORIGIN) * sizeof(StType_t);
			//
			uint32_t offset = Y_ORIGIN * (PR_cols + X_SIZE-1);

			S_rdAddr = PR_rdAddr;
			S_wrAddr = offset;
			S_rdRowIdx = 0;
			S_wrRowIdx = Y_ORIGIN;
			S_cols = PR_cols;
			S_rows = PR_rows;
			S_coef = PR_coef;

			HtContinue(STENCIL_START);
		}
		break;
		case STENCIL_START: {
			S_bStart = true;

			S_wrAddr = ((uint32_t)S_wrAddr + X_ORIGIN) * sizeof(StType_t) + PR_wrAddr;

			StencilBufferInit_5x5r2((ht_uint11)PR_cols, (ht_uint11)PR_rows);

			HtContinue(STENCIL_WAIT);
		}
		break;
		case STENCIL_WAIT: {
			if (S_wrRowIdx == S_rows)
				WriteStreamPause(STENCIL_RETURN);
			else
				HtContinue(STENCIL_WAIT);
		}
		break;
		case STENCIL_RETURN: {
			BUSY_RETRY(SendReturnBusy_htmain());

			SendReturn_htmain();
		}
		break;
		default:
			assert(0);
		}
	}

	// start read stream per row
	if (SR_bStart && SR_rdRowIdx < SR_rows + Y_SIZE-1 && !ReadStreamBusy()) {
		ReadStreamOpen(SR_rdAddr, SR_cols + X_SIZE-1);
		S_rdAddr += (SR_cols + X_SIZE-1) * sizeof(StType_t);
		S_rdRowIdx += 1;
	}

	// start write stream per row
	if (SR_bStart && SR_wrRowIdx < SR_rows + Y_ORIGIN && !WriteStreamBusy()) {
		WriteStreamOpen(SR_wrAddr, SR_cols);
		S_wrAddr += (SR_cols + X_SIZE-1) * sizeof(StType_t);
		S_wrRowIdx += 1;
	}

	CStencilBufferIn_5x5r2 stIn;
	stIn.m_bValid = ReadStreamReady() && WriteStreamReady();
	stIn.m_data = stIn.m_bValid ? ReadStream() : 0;

	CStencilBufferOut_5x5r2 stOut;

	StencilBuffer_5x5r2(stIn, stOut);

	//
	// compute stencil
	//
	T1_bValid = stOut.m_bValid;

	for (uint32_t x = 0; x < X_SIZE; x += 1)
		for (uint32_t y = 0; y < Y_SIZE; y += 1)
			T1_mult[y][x] = (StType_t)(stOut.m_data[y][x] * SR_coef.m_coef[y][x]);

	for (uint32_t x = 0; x < X_SIZE; x += 1) {
		T2_ysum[x] = 0;
		for (uint32_t y = 0; y < Y_SIZE; y += 1)
			T2_ysum[x] += T2_mult[y][x];
	}

	T3_rslt = 0;
	for (uint32_t x = 0; x < X_SIZE; x += 1)
		T3_rslt += T3_ysum[x];

	if (T3_bValid)
		WriteStream(T3_rslt);
}
