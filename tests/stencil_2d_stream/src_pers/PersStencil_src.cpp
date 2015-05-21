#include "Ht.h"
#include "PersStencil.h"

#define BUSY_RETRY(b) {if (b) {HtRetry(); break;}}

void
CPersStencil::PersStencil()
{
	if (PR1_htValid) {
		switch (PR1_htInst) {
		case STENCIL_ENTER: {
			S_bStart = true;
			S_rdAddr = PR1_rdAddr;
			S_wrAddr = PR1_wrAddr + (2 * (PR1_cols + STENCIL_RADIUS*2) + STENCIL_RADIUS) * sizeof(uint32_t);
			S_rdRowIdx = 0;
			S_wrRowIdx = STENCIL_RADIUS;
			S_cols = PR1_cols;
			S_rows = PR1_rows;
			S_coef0 = 0x80;
			S_coef1 = 0x18;
			S_coef2 = 0x08;

			StencilBufferInit_5x5r2(PR1_rows, PR1_cols);

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
	if (SR_bStart && SR_rdRowIdx < SR_rows + STENCIL_RADIUS*2 && !ReadStreamBusy()) {
		ReadStreamOpen(SR_rdAddr, SR_cols + STENCIL_RADIUS*2);
		S_rdAddr += (SR_cols + STENCIL_RADIUS*2) * sizeof(uint32_t);
		S_rdRowIdx += 1;
	}

	// start write stream per row
	if (SR_bStart && SR_wrRowIdx < SR_rows + STENCIL_RADIUS && !WriteStreamBusy()) {
		WriteStreamOpen(SR_wrAddr, SR_cols);
		S_wrAddr += (SR_cols + STENCIL_RADIUS*2) * sizeof(uint32_t);
		S_wrRowIdx += 1;
	}

	CStencilBufferIn_5x5r2 stIn;
	stIn.m_bValid = ReadStreamReady() && WriteStreamReady();
	stIn.m_data = stIn.m_bValid ? ReadStream() : 0;
	
	CStencilBufferOut_5x5r2 stOut;

	StencilBuffer_5x5r2(stIn, stOut);

	// compute stencil
	if (stOut.m_bValid) {
		uint32_t rslt = (uint32_t)
			((stOut.m_data[0][2] * SR_coef2 + stOut.m_data[1][2] * SR_coef1 +
			stOut.m_data[2][0] * SR_coef2 + stOut.m_data[2][1] * SR_coef1 +
			stOut.m_data[2][2] * SR_coef0 +
			stOut.m_data[2][3] * SR_coef1 + stOut.m_data[2][4] * SR_coef2 +
			stOut.m_data[3][2] * SR_coef1 + stOut.m_data[4][2] * SR_coef2) >> 8);

		WriteStream(rslt);
	}
}

void CPersStencil::StencilBufferInit_5x5r2(uint32_t const &ySize, uint32_t const &xSize)
{
	S_yIdx = 0;
	S_xIdx = 0;
	S_ySize = ySize;
	S_xSize = xSize;
}

void CPersStencil::StencilBuffer_5x5r2(CStencilBufferIn_5x5r2 const & stIn, CStencilBufferOut_5x5r2 & stOut)
{
	// This code is at the point of reading the stream
	if (stIn.m_bValid) {
		if (SR_yIdx == 0) {
			// first row
			S_r3_que.push(stIn.m_data);
			S_r2_que.push(0);
			S_r1_que.push(0);
			S_r0_que.push(0);

		} else if (SR_yIdx == SR_ySize + STENCIL_RADIUS*2 - 1) {
			// last row
			S_r3_que.pop();
			S_r2_que.pop();
			S_r1_que.pop();
			S_r0_que.pop();

		} else {
			S_r3_que.push(stIn.m_data);
			S_r2_que.push(S_r3_que.front());
			S_r1_que.push(S_r2_que.front());
			S_r0_que.push(S_r1_que.front());

			S_r3_que.pop();
			S_r2_que.pop();
			S_r1_que.pop();
			S_r0_que.pop();
		}

		S_xIdx += 1;
		if (S_xIdx == SR_xSize + STENCIL_RADIUS*2) {
			S_xIdx = 0;
			S_yIdx += 1;
		}

		bool stValid = SR_yIdx >= STENCIL_RADIUS*2 && SR_xIdx >= STENCIL_RADIUS*2;

		uint32_t stData[5];
		stData[0] = S_r0_que.front();
		stData[1] = S_r1_que.front();
		stData[2] = S_r2_que.front();
		stData[3] = S_r3_que.front();
		stData[4] = stIn.m_data;

		for (int c = 0; c < 1 + STENCIL_RADIUS*2; c += 1) {
			for (int r = 0; r < 1 + STENCIL_RADIUS*2; r += 1) {
				if (c == 4)
					S_stOut.m_data[r][c] = stData[r];
				else
					S_stOut.m_data[r][c] = SR_stOut.m_data[r][c+1];
			}
		}

		T1_stOut = S_stOut;
		T1_stOut.m_bValid = stValid;
	}

	stOut = TR3_stOut; 
}
