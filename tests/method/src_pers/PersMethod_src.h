#pragma once

ht_uint10 Sum(ht_uint1 cin, uint8_t c) {
    return c + cin;
}

void Inc(ht_uint4 & a) {
    a++;
}

struct A {
	ht_uint3 m_xA;
	ht_uint4 m_a[4];

	ht_uint4 GetAt(ht_uint2 ia) { return m_a[ia]; }
	ht_uint4 & GetAr(ht_uint2 ia) { return m_a[ia]; }
};

struct B {
	ht_uint5 m_xB;
	A m_b[6];

	A GetBt2(ht_uint3 ib) { return m_b[ib]; }
	A & GetBr2(ht_uint3 ib) { return m_b[ib]; }
	ht_uint4 GetBt(ht_uint3 ib, ht_uint2 ia) { return m_b[ib].GetAt(ia); }
	ht_uint4 & GetBr(ht_uint3 ib, ht_uint2 ia) { return m_b[ib].GetAr(ia); }
};

struct C {
	ht_uint3 m_ib[4];
	ht_uint2 m_ia[8];
	ht_uint2 m_xC;
	B m_c[9];

	ht_uint4 GetCt(ht_uint4 ic, ht_uint3 ib, ht_uint2 ia) { m_ib[ia++] = ib; return m_c[ic].GetBt(ib, ia-1); }
	ht_uint4 & GetCr(ht_uint4 ic, ht_uint3 ib, ht_uint2 ia) { return m_c[ic].GetBr(ib, ia); }
	ht_uint4 GetCt2(ht_uint4 ic, ht_uint3 ib, ht_uint2 ia) {
		m_ib[2] = ib;
		m_ia[5] = ia;
		ht_uint3 idx = 5;
		return m_c[ic].GetBt(m_ib[m_ib[3]], m_ia[idx]);
	}
};


class D {
public:
    ht_uint10 Sum(ht_uint1 cin, uint8_t c) {
        ht_uint9 p = m_a + c + cin;
        ht_uint10 r = p + m_b;
        return r;
    }

public:
    ht_uint7     m_a;
    ht_uint9     m_b;
};

class E {
public:
    ht_uint5     GetCt(ht_uint2 i) { return m_c[i]; }

public:
    ht_uint5     m_c[4];
};

class F {
public:
    E       GetBt(ht_uint2 i) const { return m_b[i]; }

public:
    E       m_b[4];
};

class G {
public:
	E &	GetBr(ht_uint2 i) {
		return i < 2 ? m_b[i + 1] : m_b[i - 1];
	}

	E m_b[4];
};

class H {
public:
    E GetBt(ht_uint2 i) {
		m_i = i;
		return m_b[m_i];
	}

public:
	ht_uint2	m_i;
	E			m_b[4];
};
