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

void CPersInc::fv(ht_uint4 & c)
{
	c = 0x8;
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

ht_uint4 fr(ht_uint4 const & v)
{
	if ((v & 0x8) != 0)
		return 0;

	if ((v & 0x4) == 0) {
	} else
		return 1;

	ht_uint4 v2 = v-1;
	switch (v) {
	case 3:
		v2 += 1;
		break;
	default:
		v2 += 1;
	}

	if ((v & 0x2) != 0) {
		if ((v & 0x1) != 0)
			return 2;
	} else {
		if ((v & 0x1) != 0) {
		} else
			return 3;
	}

	return 4;
}

struct CStruct2 {
	ht_uint4 m_u4[3];
	bool	m_b;
};

#define SCORE_WIDTH 2
typedef ht_uint4 Score_t;

struct ScoreStruct {
	ht_uint4 m_score[SCORE_WIDTH];
};

static void IncArray(ScoreStruct & score, sc_uint<1> & idx)
{
	score.m_score[idx++] = 2;
}

void CPersInc::PersInc()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case INC_INIT:
		{
			//{
			//	struct A {
			//		uint8_t x;
			//		uint16_t y;
			//		ht_uint4 z;
			//	};

			//	A a[] = { { 1, 2, 3 }, { 2, 3, 4 }, { 3, 4, 5 }, { 4, 5, 6 } };
			//	if (a[0].x != 1 || a[1].y != 3 || a[3].z != 6)
			//		HtAssert(0, 0);
			//}

			{ // test for bug 2049
				int a[4];
				//if (true)
					for (int i = 0; i < 4; i += 1)
						if ((i & 1) == 0)
							a[i] = i + 1;
						else
							a[i] = i + 2;
				//else
				//	a[0] = 0;

				for (int i = 0; i < 4; i += 1)
					if (a[i] != i + 1 + (i & 1))
						HtAssert(0, 0);
			}

			{		// test for bug 1891
				bool b = true;
				uint32_t c = ~b;
				ht_uint4 d = ~fb();
				ht_uint4 e = fr(~b ? d : d);

				if (c != 0xfffffffe || d != 0xf || e != 0)
					HtAssert(0, 0);
			}

			{		// test for bug 1895
				int a = 3;
				int b = a | 0;
				int c[3] = {-1, -1, -1};

				if (a == b) {
					for (int i = 0; i < 3; i += 1)
						c[i] = i;
				}
				if (c[0] != 0 || c[1] != 1 || c[2] != 2)
					HtAssert(0, 0);
			}

			{
				bool b = false;
				bool a = b != true;
				ht_uint6 c = 0x23;
				b |= a && fu4() == 5;
				c |= a && c(5,2) == 5;
			}

			{
				char t0 = '\1';
				char t2 = '\12';
				char t3 = '\123';
				char t4 = '\xfe';
				char t5 = '\x0';
				uint64_t t1 = 18446744073709551615ULL;
				if (t0 != 1 || t1 != 0xffffffffffffffffULL || t2 != 0xa || t3 != 83 || t4 != -2 || t5 != 0)
					HtAssert(0, 0);
			}

			{
				if (c2 != 2 || c3() != 3 || s1(4) != 4 || s2(3,4) != 7)
					HtAssert(0, 0);
			}

			{
				ht_uint4 v8 = 8;
				ht_uint4 r8 = fr(v8);

				ht_uint4 v4 = 4;
				ht_uint4 r4 = fr(v4);

				ht_uint4 v3 = 3;
				ht_uint4 r3 = fr(v3);

				ht_uint4 v2 = 2;
				ht_uint4 r2 = fr(v2);

				ht_uint4 v1 = 1;
				ht_uint4 r1 = fr(v1);

				ht_uint4 v0 = 0;
				ht_uint4 r0 = fr(v0);

				if (r8 != 0 || r4 != 1 || r2 != 4 || r3 != 2 || r1 != 4 || r0 != 3)
					HtAssert(0, 0);
			}

			{
				ht_uint3 a=3, b=2, r=a+b;

				if (r != 5)
					HtAssert(0, 0);
			}

			{
				ht_uint3 a=3, r, b=2;

				r = a + b;

				if (r != 5)
					HtAssert(0, 0);
			}

			{
				int a = 6;
				bool c = (bool)a;
				bool d = !!a;

				if (!c || !d)
					HtAssert(0, 0);
			}

			P_loopCnt = 0;

			{	// test for bug 1576 - missing cast with double cast
				struct {
					ht_int10 m_d[2];
				} a;
				a.m_d[0] = 0xff;
				a.m_d[1] = 0xff;
				ht_uint1 i = PR_htValid;
				ht_uint10 c = 0xff;
				int16_t r = (int16_t)c + (int16_t)(ht_uint6)a.m_d[i];

				if (r != 0xff + 0x3f)
					HtAssert(0, 0);
			}

			{               // test for bug 1456 - constant integer propogation
				sc_uint<8> task = 0;
				for (int i = 0; i < 4; i += 1) {
					int j = i * 2;
					int k = j + 2 - 1;
					task(k, j) = i;
				}
				if (task != 0xe4)
					HtAssert(0, 0);
			}

			{               // test for bug 1461 - loop unrolling with out of range indexing
				int a[3] = { 1, 2, 3 };
				int b = 0;
				for (int i = 0; i < 3; i += 1) {
					if (i > 0)
						b += a[i - 1];

					switch (i) {
					case 0: b += a[i + 1]; break;
					case 1: b += a[i + 1]; break;
					default: break;
					}
				}

				if (b != 8)
					HtAssert(0, 0);
			}

			{               // test for bug 1487
				ScoreStruct ss;
				ss.m_score[1] = 0;
				sc_uint<1> i = 1;

				IncArray(ss, i);

				if (ss.m_score[1] != 2)
					HtAssert(0, 0);
			}

			//uint18_t tScore;

			//for (int i = 0; i< 9; i++){
			//	int j = i*SCORE_WIDTH;
			//	int k = j+SCORE_WIDTH-1;
			//	int l = 4-i;
			//	if (l<0) l = i-4;
			//	tScore(k,j) = (Score_t)l;
			//}

			ht_uint4 c_13;
			fv(c_13);
			c_13 += 1;

			fb();

			bool b_12 = true;
			ScoreStruct c_12;
			c_12.m_score[0] = 0;
			Score_t d_12 = 0;

			Score_t rsltScore = b_12 ? c_12.m_score[(ht_uint1)d_12] : (Score_t)7;
			rsltScore += 1;

			// multiply by a constant
			ht_uint6 b_10 = 3;
			ht_uint8 a_10 = b_10 * 4;
			ht_uint8 c_10 = b_10 * 4u;
			ht_uint9 d_10 = b_10 * 5;
			ht_uint9 e_10 = b_10 * 5u;
			d_10 += a_10 + c_10 + e_10;

			ht_int6 b_11 = -3;
			ht_int8 a_11 = b_11 * 4;
			a_11 = b_11 * 4u;
			ht_int9 d_11 = b_11 * 5;
			d_11 = b_11 * 5u;
			ht_int8 f_11 = b_11 * -4;
			ht_int9 g_11 = b_11 * -5;
			g_11 += a_11 + d_11 + f_11;

			// bug_1439
			ht_uint5 a_1439 = 0x3;
			int8_t b_1439 = 0x4;
			bool r_1439 = b_1439 < (ht_int6)a_1439;
			r_1439 = r_1439;

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
				MemAddr_t memRdAddr = SR_arrayAddr + ((P_loopCnt * 2) << 3);

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
			MemAddr_t memWrAddr = SR_arrayAddr + ((P_loopCnt * 2 + 1) << 3);

			// Issue write memory request
			WriteMem(memWrAddr, memWrData);

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
