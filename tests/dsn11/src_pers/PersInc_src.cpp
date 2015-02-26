#include "Ht.h"
#include "PersInc.h"

ht_uint4 CPersInc::fu4()
{
	return 4;
}
bool CPersInc::fb()
{
	ht_uint4 u4 = 4;
	bool b;

	switch (u4 + fu4()) {
	case 1: b = true; break;
	default: b = false; break;
	}
	return b;
}

ht_uint4 CPersInc::fp1(ht_uint4 const & a, ht_uint4 const & b)
{
	return a + b;
}

CStruct CPersInc::fs()
{
	CStruct s;

	s = 0;
	return s;
}

ht_uint4 CPersInc::fg(ht_uint4 const & a, ht_uint4 & b, ht_uint4 & c)
{
	b = a + b;
	c = a + S_pu4;
	return a - S_pu4;
}

ht_uint4 CPersInc::fl(ht_uint4 const & a, ht_uint4 & b, ht_uint4 & c)
{
	b = a + b;
	c = a;
	return a;
}

struct CStruct2 {
	ht_uint4 m_u4[3];
	bool	m_b;
};

void CPersInc::PersInc()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case INC_INIT:
		{
			P_loopCnt = 0;

			// test signed expressions
			sc_int<5> a = 5;
			sc_int<5> b = -3;
			sc_int<5> c = a + b;
			S_pu4 = (ht_uint4)c;
			c = c++;

			// test function calls in expressions
			CStruct2 st;

			st.m_b = true;
			st.m_u4[(ht_uint2)st.m_b] = st.m_b ? st.m_u4[2] : (ht_uint4)(fu4() + st.m_u4[(ht_uint2)(P_loopCnt & 1)]);
			st.m_u4[(ht_uint2)st.m_b] = 5;

			ht_uint4 u4;
			S_pu4 = fg(fu4() + 5, S_pu4, u4);

			S_pu4 = fl(fu4() + 5, S_pu4, u4);

			++S_pu4;
			S_pu4 = S_pu4++ + u4;

			S_pu4 = fs().m_u3 << 1;

			fp1(S_pu4, fu4() + 4);

			S_pb = S_pb || fb();

			S_pb = S_pb || fb() || S_pb;

			S_pu4 = fu4() + 4;
			S_pu4 = S_pu4 + fu4() + 4 - fu4();

			ht_uint4 arr[2][4];
			for (int i = 0; i < 4; i += 1) {
				arr[0][i] = 0;
				arr[1][i] = 0;
			}
			arr[fb() ? 0u : 1u][fb() << 1] = arr[fu4() & 1][fu4() & 3] + fu4() + 4 - fu4();

			// test logicals
			bool b1 = true;
			bool b2 = true;
			bool b3 = true;

			S_pb = S_pb || fb() || b1 && b2 && b3 || fb();

			// test conditional expression
			S_pu4 = S_pb ? fu4() : (ht_uint4)8;

			S_pu4 = fb() ? fu4() : (ht_uint4)8;

			S_pu4 += 1;

			S_pu4 = fu4() + (fb() && b1 ? S_pu4 : (ht_uint4)(fu4() + 4ul));

			S_pb = b2 && (fb() ? b1 : b3);

			S_pb = fb() || (fb() && b3 && S_pu4++ < 4 || --S_pu4 > 5 ? b2 : b1 && fb()) && fb() && b2;

			HtContinue(INC_READ);
		}
		break;
		case INC_READ:
		{
			if (ReadMemBusy() || SendReturnBusy_htmain()) {
				HtRetry();
				break;
			}

			// Check if end of loop
			if (P_loopCnt == P_elemCnt) {
				// Return to host interface
				SendReturn_htmain(P_loopCnt);
			} else {
				// Calculate memory read address
				MemAddr_t memRdAddr = SR_arrayAddr + (P_loopCnt << 3);

				// Issue read request to memory
				ReadMem_arrayMem(memRdAddr, PR_htId);

				// Set address for reading memory response data
				P_arrayMemRdPtr = PR_htId;

				ReadMemPause(INC_WRITE);
			}
		}
		break;
		case INC_WRITE:
		{
			if (WriteMemBusy()) {
				HtRetry();
				break;
			}

			// Increment memory data
			uint64_t memWrData = GR_arrayMem.data + 1;

			// Calculate memory write address
			MemAddr_t memWrAddr = SR_arrayAddr + (P_loopCnt << 3);

			// Issue write memory request
			WriteMem(memWrAddr, memWrData);

			memWrData = GR_arrayMem2.data;

			// Increment loop count
			P_loopCnt = P_loopCnt + 1;

			WriteMemPause(INC_READ);
		}
		break;
		default:
			assert(0);
		}
	}
}
