#include "Ht.h"
#include "PersFunc.h"

void CPersFunc::f1(const uint8_t &i, uint8_t &o)
{
	o = i + o;
}

void CPersFunc::f3(const uint8_t &i, uint8_t &o)
{
	o = GR_htReset ? 0 : (i + o);
}

struct s2 {
	ht_uint5 m_f0;
	uint8_t m_f1[4];
};

uint8_t f2(uint8_t a=1, uint8_t b=2) {
	return a + b;
}

void CPersFunc::f4(s4 &m) {
	if (T1_bool)
		m.m_u8++;
}

void CPersFunc::PersFunc()
{
    if (PR1_htValid) {
	switch (PR1_htInst) {
	case TEST: {
		{
			uint8_t o = 6;
			uint8_t i = 7;

			f3(i, o);

			if (o != 13) {
			HtAssert(0, (uint32_t)o);
			P1_err += 1;
			}
		}

		{
			uint8_t a=3;
			uint8_t b=4;

			uint8_t c1 = f2(a, b);
			uint8_t c2 = f2(a);
			uint8_t c3 = f2();

			if (c1 != 7 || c2 != 5 || c3 != 3) {
				HtAssert(0, (uint32_t)((c3 << 16) | (c2 << 8) | c1));
				P1_err += 1;
			}
		}

		{
			int d=1;
			s1 a;
			a.m_f1[0] = 1;
			a.m_f1[0] += 1;
			a.m_f1[0] ++;
			a.m_f1[(ht_uint2)d] = 0;
			a.m_f1[(ht_uint2)d] += 1;
			a.m_f1[d&3] ++;

			f1(a.m_f1[0], a.m_f1[(ht_uint2)d]);

			if (a.m_f1[0] != 3) {
				HtAssert(0, (uint32_t)a.m_f1[0]);
				P1_err += 1;
			}
			if (a.m_f1[(ht_uint2)d] != 5) {
				HtAssert(0, (uint32_t)a.m_f1[(ht_uint2)d]);
				P1_err += 1;
			}
		}

		{
			int d=1;
			uint8_t a[2] = { 0 };
			a[0] = 1;
			a[0] += 1;
			a[0] ++;
			a[(ht_uint1)d] = 0;
			a[(ht_uint1)d] += 1;
			a[d&1] ++;

			f1(a[0], a[(ht_uint1)d]);

			if (a[0] != 3) {
				HtAssert(0, (uint32_t)a[0]);
				P1_err += 1;
			}
			if (a[(ht_uint1)d] != 5) {
				HtAssert(0, (uint32_t)a[(ht_uint1)d]);
				P1_err += 1;
			}
		}

		{
			ht_uint2 d=2;
			ht_uint2 e = 1;
			s2 a[4];
			a[0].m_f1[0] = 1;
			a[1].m_f1[0] = 1;
			a[2].m_f1[0] = 1;
			a[3].m_f1[0] = 1;
			a[d].m_f1[0] += 1;
			a[2].m_f1[0] ++;
			a[e].m_f1[(ht_uint2)d] = 0;
			a[1].m_f1[(ht_uint2)d] += 1;
			a[e].m_f1[d&3] ++;

			f1(a[d].m_f1[0], a[e].m_f1[d]);

			if (a[d].m_f1[0] != 3) {
				HtAssert(0, (uint32_t)a[d].m_f1[0]);
				P1_err += 1;
			}
			if (a[e].m_f1[d] != 5) {
				HtAssert(0, (uint32_t)a[e].m_f1[d]);
				P1_err += 1;
			}
		}

		{
			T1_bool = true;
			P1_s4.m_u8 = 1;

			f4(P1_s4);

			if (P1_s4.m_u8 != 2) {
				HtAssert(0, (uint32_t)P1_s4.m_u8);
				P1_err += 1;
			}
		}

		if (P1_err)
			P1_cnt = 128;

		HtContinue(RTN);
	}
	break;
	case RTN: {
		if (SendReturnBusy_htmain()) {
			HtRetry();
			break;
		}

		// let HtAssert propagate
		if (P1_cnt) {
			P1_cnt -= 1;
			HtContinue(RTN);
		}

		SendReturn_htmain(P1_err);
	}
	break;
	default:
		assert(0);
	}
    }
}
