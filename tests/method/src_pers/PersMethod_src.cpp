#include "Ht.h"
#include "PersMethod.h"
#include "PersMethod_src.h"

Sx CPersMethod::funcSx()
{
	return S_sx;
}

void CPersMethod::PersMethod()
{
    if (PR_htValid) {
        switch (PR_htInst) {
        case TEST:
            {
                P_err = 0;
				P_cnt = 0;

				{
					S_sx.m_b = 0x11;

					ht_uint5 r = funcSx().m_b;

					if (r != 0x11) {
						HtAssert(0, (uint32_t)r);
						P_err |= 0x20000;
					}
				}

				{
					B b;
					b.m_b[3].m_a[2] = 0x9;
					b.m_b[3].m_xA = 5;

					ht_uint4 r = b.GetBt2(3).m_a[2];
					ht_uint3 r2 = b.GetBt2(3).m_xA;

					if (r != 0x9 || r2 != 5) {
						HtAssert(0, (uint32_t)r);
						P_err |= 0x20000;
					}
				}

				//{	// parser does not support
				//	G c[2];

				//	c[0].m_b[1].m_c[2] = 5;
				//	c[1].m_b[0].m_c[2] = 4;

				//	bool b1 = true;
				//	bool b2 = b1 && !b1;

				//	ht_uint5 rslt = (b2 ? c[0].m_b[1] : c[1].m_b[0]).m_c[2];

				//	if (rslt != 0x5) {
				//		HtAssert(0, (uint32_t)rslt);
				//		P_err |= 0x10000;
				//	}
				//}

				{ // Test 1.a

					A aa;
					aa.m_a[2] = 0xa;
					ht_uint4 a = aa.GetAt(2);

					if (a != 0xa) {
						HtAssert(0, (uint32_t)a);
						P_err |= 0x8000;
					}

					B bb;
					bb.m_b[2].m_a[1] = 0xa;
					ht_uint4 b = bb.GetBt2(2).GetAt(1);

					if (b != 0xa) {
						HtAssert(0, (uint32_t)b);
						P_err |= 0x4000;
					}

					C cc;
					cc.m_c[5].m_b[3].m_a[1] = 0xa;
					ht_uint4 c = cc.GetCt(5,3,1);

					if (c != 0xa) {
						HtAssert(0, (uint32_t)c);
						P_err |= 0x2000;
					}
				}

				{ // Test 1.r

					A aa;
					aa.m_a[2] = 0xa;
					ht_uint4 a = aa.GetAr(2);

					if (a != 0xa) {
						HtAssert(0, (uint32_t)a);
						P_err |= 0x1000;
					}

					B bb;
					bb.m_b[2].m_a[1] = 0xa;
					ht_uint4 b1 = bb.GetBr2(2).GetAt(1);

					if (b1 != 0xa) {
						HtAssert(0, (uint32_t)b1);
						P_err |= 0x800;
					}

					ht_uint4 b2 = bb.GetBt2(2).GetAr(1);

					if (b2 != 0xa) {
						HtAssert(0, (uint32_t)b2);
						P_err |= 0x400;
					}

					ht_uint4 b3 = bb.GetBr2(2).GetAr(1);

					if (b3 != 0xa) {
						HtAssert(0, (uint32_t)b3);
						P_err |= 0x200;
					}

					C cc;
					cc.m_c[5].m_b[3].m_a[1] = 0xa;
					ht_uint4 c = cc.GetCr(5,3,1);

					if (c != 0xa) {
						HtAssert(0, (uint32_t)c);
						P_err |= 0x100;
					}
				}

				{ // Test 1.b
                    struct Z {
                        ht_uint1 m_z;
                        ht_uint1 GetZ() { return m_z; }
                    } zz;

                    zz.m_z = 1;
                    if (zz.GetZ() != 1) {
                        HtAssert(0, (uint32_t)zz.GetZ());
                        P_err |= 0x80;
                    }
                }

                { // Test 2
                    ht_uint4 a1 = 4;
                    Inc(a1);
                    if (a1 != 5) {
                        HtAssert(0, (uint32_t)a1);
                        P_err |= 0x40;
                    }
                }

                { // Test 3
                    D d;
                    d.m_a = 5;
                    d.m_b = 0x123;
                    uint8_t c = 0x9c;
                    ht_uint10 rslt = d.Sum(1, c);
                    if (rslt != 0x1c5) {
                        HtAssert(0, (uint32_t)rslt);
                        P_err |= 0x20;
                    }
                }

                { // Test 4
                    F c[2];
                    c[0].m_b[1].m_c[2] = 5;
                    c[1].m_b[2].m_c[3] = 6;
                    c[0].m_b[3].m_c[1] = 3;
                    c[1].m_b[2].m_c[0] = 1;

					ht_uint5 rslt = c[0].GetBt(1).GetCt(2) +
                        c[1].GetBt(2).m_c[3] +
                        c[0].m_b[3].GetCt(1) +
                        c[1].m_b[2].m_c[0];

                    if (rslt != 0xf) {
                        HtAssert(0, (uint32_t)rslt);
                        P_err |= 0x10;
                    }
                }

                { // Test 5
                    F c[2];
                    c[0].m_b[1].m_c[2] = 5;
                    c[1].m_b[2].m_c[3] = 6;
                    c[0].m_b[3].m_c[1] = 3;
                    c[1].m_b[2].m_c[0] = 1;

                    ht_uint1 ci0 = 0;
                    ht_uint2 bi3 = 3;
                    ht_uint2 ai3 = 3;

                    ht_uint5 rslt = c[ci0].GetBt(1).GetCt(2) +
                        c[1].GetBt(2).m_c[ai3] +
                        c[ci0].m_b[bi3].GetCt(1) +
                        c[1].m_b[2].m_c[0];

                    if (rslt != 0xf) {
                        HtAssert(0, (uint32_t)rslt);
                        P_err |= 8;
                    }
                }

				{
					G c[2];

					c[0].m_b[1].m_c[2] = 5;
					c[1].m_b[0].m_c[3] = 2;

					ht_uint1 ci0 = 0;
					ht_uint2 bi1 = 0; // GetBr adds one to index
					ht_uint2 ai2 = 2;

					ht_uint5 rslt = c[ci0].GetBr(bi1).GetCt(ai2);

					if (rslt != 0x5) {
						HtAssert(0, (uint32_t)rslt);
						P_err |= 4;
					}
				}

				{
					H c[2];

                    c[0].m_b[1].m_c[2] = 5;
                    c[1].m_b[0].m_c[3] = 2;

                    ht_uint1 ci0 = 0;
                    ht_uint2 bi1 = 1;
                    ht_uint2 ai2 = 2;

                    ht_uint5 rslt = c[ci0].GetBt(bi1).GetCt(ai2);

                    if (rslt != 0x5) {
                        HtAssert(0, (uint32_t)rslt);
                        P_err |= 2;
                    }
                }

				{
					C cc;
					cc.m_c[5].m_b[3].m_a[1] = 0xa;
					cc.m_ib[3] = 2;
					ht_uint4 c = cc.GetCt2(5,3,1);

					if (c != 0xa) {
						HtAssert(0, (uint32_t)c);
						P_err |= 1;
					}
				}

                if (P_err)
                    P_cnt = 128;

                HtContinue(RTN);
            }
            break;
        case RTN:
            {
                if (SendReturnBusy_htmain()) {
                    HtRetry();
                    break;
                }

                // let HtAssert propagate
                if (P_cnt) {
                    P_cnt -= 1;
                    HtContinue(RTN);
                }

                SendReturn_htmain(P_err);
            }
            break;
        default:
            assert(0);
        }
    }
}
