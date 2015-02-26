#include "Ht.h"
#include "PersMath.h"

enum Eabc { a = 0, b = 2, c = 1 };

void CPersMath::PersMath()
{
	if (PR_htValid) {
		switch (PR_htInst) {
		case TEST:
		{
			P_errMask = 0;
			P_cnt = 0;

			{
				ht_uint6 sh = 33;
				uint64_t a, b;
				a = 0xffffffffffffffffull;
				b = a;
				
				a = ((uint64_t)(a & ~(1ul << sh)));
				b = ((uint64_t)(b & ~(1ull << sh)));
				
				if (a != b) {
#ifndef _WIN32
					HtAssert(0, 0);
					P_errMask = 0x20000000000ull;
#endif
				}
			}

			{
				ht_uint32 a = 0;
				ht_uint16 b = 0;

				a = ~b[15];

				if (a != 1) {
					HtAssert(0, a);
					P_errMask = 0x10000000000ull;
				}
			}

			{
				ht_int32 a = 0;
				ht_int16 b = 0;

				a = ~b[15];

				if (a != 1) {
					HtAssert(0, a);
					P_errMask = 0x8000000000ull;
				}
			}

			{
				ht_uint32 a = 0;
				ht_uint16 b = 0;

				a = ~b(15, 15);

				if (a != 0xffffffff) {
					HtAssert(0, a);
					P_errMask = 0x4000000000ull;
				}
			}

			{
				ht_int32 a = 0;
				ht_int16 b = 0;

				a = ~b(15, 15);

				if (a != -1) {
					HtAssert(0, a);
					P_errMask = 0x2000000000ull;
				}
			}

			{
				ht_uint32 a = 0;
				ht_uint1 b = 0;

				a = ~b;

				if (a != 0xffffffff) {
					HtAssert(0, a);
					P_errMask = 0x1000000000ull;
				}
			}

			{
				ht_uint32 a = 0;
				bool b = false;

				a = ~b;

				if (a != 0xffffffff) {
					HtAssert(0, a);
					P_errMask = 0x800000000ull;
				}
			}

			{
				ht_uint32 a = 0;
				bool b = true;

				a = ~b;

				if (a != 0xfffffffe) {
					HtAssert(0, a);
					P_errMask = 0x400000000ull;
				}
			}

			{
				ht_int32 a = 0;
				ht_int2 b = 0;

				a = ~b;

				if (a != -1) {
					HtAssert(0, a);
					P_errMask = 0x200000000ull;
				}
			}

			{
				uint64_t a = (uint64_t)(int64_t)-1;
				if (a != 0xffffffffffffffffull) {
					HtAssert(0, 0);
					P_errMask = 0x100000000ull;
				}
			}
			
			{
				Eabc abc = b;
				if (abc != b) {
					HtAssert(0,0);
					P_errMask = 0x80000000;
				}

				Eabc abc2;
				switch (abc) {
				case a:
					abc2 = a;
					break;
				case b:
					abc2 = b;
					break;
				case c:
					abc2 = c;
					break;
				default:
					HtAssert(0,0);
				}

				if (abc2 != b) {
					HtAssert(0,0);
					P_errMask = 0x40000000;
				}
			}

			{
				enum Edef { d = 5, e = 3, f = 7 };

				Edef def = e;
				if (def != e) {
					HtAssert(0,0);
					P_errMask = 0x20000000;
				}

				Edef def2;
				switch (def) {
				case d:
					def2 = d;
					break;
				case e:
					def2 = e;
					break;
				case f:
					def2 = f;
					break;
				default:
					HtAssert(0,0);
				}

				if (def2 != e) {
					HtAssert(0,0);
					P_errMask = 0x10000000;
				}
			}

			{               // builtin reduction function
				sc_uint<10> a = 0x3ff;
				sc_uint<1> b = 0;
				b = a.and_reduce();

				if (b != 1) {
					HtAssert(0, 0);
					P_errMask = 0x8000000;
				}
			}

			{
				sc_uint<3> v0 = (sc_uint<3>)641032;
				uint8_t v1 = (uint8_t)289326;

				sc_uint<11> r0 = (sc_uint<11>)( ( v0 - v1++ ) / 559470 );

				if (r0 != 1094) {
					HtAssert(0, 0);
					P_errMask = 0x4000000;
				}
			}

			{
				sc_uint<22> v0 = (sc_uint<22>)99;
				uint8_t v1 = (uint8_t)9999998;

				bool r0 = (bool)( v0 == (uint8_t)v1++ );

				if (r0 != false) {
					HtAssert(0, 0);
					P_errMask = 0x2000000;
				}
			}

			{
				sc_int<2> v0 = (sc_int<2>)2;

				if (v0 != -2) {
					HtAssert(0, 0);
					P_errMask = 0x1000000;
				}
			}

			{
				sc_int<32> v0 = 1;

				bool r0 = (bool)( ~10 < ( false ? (sc_int<32>)1 : (sc_int<32>)v0 ) );

				if (r0 != true) {
					HtAssert(0, 0);
					P_errMask = 0x800000;
				}
			}

			{
				ht_int32 v1 = 1;
				ht_int32 r = (ht_int32) (~1 >> /*(sc_uint<5>)*/v1);

				if (r != -1) {
					HtAssert(0, 0);
					P_errMask = 0x400000;
				}
			}

			{
				ht_int32 v0 = 0x7fffffff;
				ht_int32 v1 = 1+64;
				ht_int32 r = (ht_int32) (v0 >> v1);

				if (r != 0x3fffffff) {
					HtAssert(0, 0);
					P_errMask = 0x200000;
				}
			}

			{
				int t = (int) (1 << 31);
				ht_int32 r = (ht_int32) (1 << 31);

				if (t != (int)0x80000000 || r != (int)0x80000000) {
					HtAssert(0, 0);
					P_errMask = 0x100000;
				}
			}

			{
				int v0 = 4;
				int32_t t = (int) (v0 > 3);

				if (t != 1) {
					HtAssert(0, 0);
					P_errMask = 0x80000;
				}
			}

			sc_int<32> r0;
			sc_int<32> r1;
			{
				sc_int<32> v0 = 1;

				r0 = (sc_int<32>)++v0;
				r1 = (sc_int<32>)v0;

				if (r0 != 2 || r1 != 2) {
					HtAssert(0, 0);
					P_errMask = 0x40000;
				}
			}
			{
				sc_int<32> r2;
				sc_int<32> r3;
				sc_int<32> v0 = 1;

				r2 = (sc_int<32>)--v0;
				r3 = (sc_int<32>)v0;

				if (r2 != 0 || r3 != 0) {
					HtAssert(0, 0);
					P_errMask = 0x20000;
				}
			}

			struct s1 {
				ht_uint16	m_o;

				void		Set(ht_uint16 o)
				{
					m_o = o;
				}
				ht_uint16	Get()
				{
					return m_o;
				}

				ht_uint10	Func(ht_uint4 in)
				{
					m_o = (m_o << 4) | in;
					return in;
				}
			};

			{               // test order of add operator
				s1 s;
				s.Set(0);

				ht_uint10 r = s.Func(1) + s.Func(2) + s.Func(3);

				if (r != 6 || s.Get() != 0x123) {
					HtAssert(0, 0);
					P_errMask = 0x10000;
				}
			}

			{               // test order of multiply operator
				s1 s;
				s.Set(0);

				ht_uint10 r = s.Func(1) * s.Func(2) * s.Func(3);

				if (r != 6 || s.Get() != 0x123) {
					HtAssert(0, 0);
					P_errMask = 0x8000;
				}
			}

			{               // test order of ? operator
				s1 s;
				s.Set(0);
				bool bSel = true;

				ht_uint10 r = (bSel ? s.Func(1) : s.Func(2)) + (!bSel ? s.Func(3) : s.Func(4));

				if (r != 5 || s.Get() != 0x14) {
					HtAssert(0, 0);
					P_errMask = 0x4000;
				}
			}

			{
				// test bug 1258
				struct { uint32_t a : 1; } a;
				struct { sc_uint<1> b; } b;
				a.a = 1;
				b.b = 1;

				ht_uint1 rslt = a.a ^ b.b;
				if (rslt != 0) {
					HtAssert(0, 0);
					P_errMask = 0x2000;
				}
			}

#ifdef GCC44_AND_ABOVE_ASSERT
			{
				struct {
					uint64_t b : 48;
				} s1;

				struct {
					uint32_t a : 16;
				} s2;

				s1.b = 0x5fffe8250028ull;
				s2.a = 4;
				ht_uint48 rslt = s1.b + (ht_uint48)(s2.a - 8);

				if (rslt != 0x6000e8250024ull) {
					HtAssert(0, (uint32_t)(rslt >> 32));
					P_errMask = 0x1000;
				}
			}
#endif

			{
				struct s1 {
					ht_uint48 b;
				};

				uint16_t a = 4;
				s1 s;
				s.b = 0x5fffe8250028ull;
				ht_uint48 rslt = s.b + (ht_uint48)(a - 8);

				if (rslt != 0x5fffe8250024ull) {
					HtAssert(0, (uint32_t)(rslt >> 32));
					P_errMask = 0x800;
				}
			}

			{
				ht_uint31 a = 0x7fffffff;
				ht_uint31 b = 1;
				bool rslt = (a + b) == 0x80000000u;
				if (!rslt) {
					HtAssert(0, 0);
					P_errMask = 0x400;
				}
			}

			{
				ht_int5 a = -3;
				ht_uint5 b = 2;
				uint8_t rslt = (uint8_t)(a + b);
				if (rslt != 0xff) {
					HtAssert(0, (uint32_t)rslt);
					P_errMask = 0x200;
				}
			}

			{
				ht_int40 a = 4;
				ht_uint5 b = 31;
				bool rslt = a < (ht_int6)b;
				if (!rslt) {
					HtAssert(0, (uint32_t)rslt);
					P_errMask = 0x100;
				}
			}

			{
				ht_uint6 a = 8;
				ht_uint6 rslt = a + (PR_htValid ? 1 : -1);
				// coded tricksy for x propagation in verilog
				if (rslt == 9) {
					;
				} else {
					HtAssert(0, (uint32_t)rslt);
					P_errMask = 0x80;
				}
			}

			{
				uint8_t a = 3;
				uint8_t b[4];
				b[0] = 0; b[1] = 1; b[2] = 2; b[3] = 3;
				uint8_t rslt = b[(ht_uint2)a++];
				if (a == 4 && rslt == 3) {
					;
				} else {
					HtAssert(0, (uint32_t)((a << 16) | rslt));
					P_errMask = 0x40;
				}
			}

			{
				uint8_t a = 3;
				uint8_t b[4];
				b[0] = 0; b[1] = 1; b[2] = 2; b[3] = 3;
				uint8_t rslt = b[(ht_uint2)++ a];
				if (a == 4 && rslt == 0) {
					;
				} else {
					HtAssert(0, (uint32_t)((a << 16) | rslt));
					P_errMask = 0x20;
				}
			}

			{
				// ANSI C Section 6.1 Integral promotion
				uint64_t a = 0xf3ade68b1ull;
				uint64_t rslt = (uint64_t)((uint32_t)a << 3);
				if (rslt == 0xd6f34588ull) {
					;
				} else {
					HtAssert(0, (uint32_t)(rslt >> 16));
					P_errMask = 0x10;
				}

				a = 0xf80ull;
				rslt = (uint64_t)((uint8_t)a << 1);
				if (rslt == 0x100) {
					;
				} else {
					HtAssert(0, (uint32_t)rslt);
					P_errMask = 0x8;
				}
			}

			{
				int8_t b[4] = { 0, -5, 2, 3 };
				ht_uint2 a = 1;
				b[a++] += 4u;
				if (b[1] != -1 || a != 2) {
					HtAssert(0, (uint32_t)b[1]);
					P_errMask = 0x4;
				}
			}

			// Bug 1561
			{
				struct s1 {
					int8_t f[4];
				};
				s1 a;
				a.f[0] = 0;
				a.f[1] = -1;
				a.f[2] = -2;
				a.f[3] = -3;
				ht_uint2 b = 2;
				if (a.f[b] < 0) {
				} else {
					HtAssert(0, 0);
					P_errMask = 0x2;
				}
			}

			// Bug 1586
			{
				uint8_t a = 0xf0;
				uint8_t b = 0xc0;
				bool c = (bool)((a & b) >> 4);
				bool d = !!((a & b) >> 4);
				if (c != d) {
					HtAssert(0, 0);
					P_errMask = 0x1;
				}
			}

			if (P_errMask != 0)
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

			SendReturn_htmain(P_errMask);
		}
		break;
		default:
			assert(0);
		}
	}
}
