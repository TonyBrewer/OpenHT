/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#ifndef _WIN32
#include <limits.h>
#endif

#pragma once

#ifdef _WIN32
#pragma warning(disable : 4146)
#pragma warning(disable : 4018)
#else
#define GCC_VERSION (__GNUC__*10000 + __GNUC_MINOR__*100 + __GNUC_PATCHLEVEL__)
#if GCC_VERSION >= 40600
#pragma GCC diagnostic push
#endif
#if GCC_VERSION >= 40200
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif
#endif

struct CConstValue {

	enum EConstType { UINT32 = 0, UINT64 = 1, SINT32 = 2, SINT64 = 3, UNKNOWN };

	CConstValue() { m_constType = UNKNOWN; }
	CConstValue(int32_t i) { m_sint64 = i; m_constType = SINT32; }
	CConstValue(uint32_t i) { m_uint64 = i; m_constType = UINT32; }
	CConstValue(uint64_t i) { m_uint64 = i; m_constType = UINT64; }

	bool IsZero() const {
		switch (GetConstType()) {
		case SINT64: return m_sint64 == 0;
		case SINT32: return m_sint32 == 0;
		case UINT64: return m_uint64 == 0;
		case UINT32: return m_uint32 == 0;
		default: return true;
		}
	}

	bool IsNeg() const {
		switch(GetConstType()) {
			case SINT64: return m_sint64 < 0;
			case SINT32: return m_sint32 < 0;
			default: return false;
		}
	}
	bool IsPos() const {
		switch(GetConstType()) {
			case SINT64: return m_sint64 >= 0;
			case SINT32: return m_sint32 >= 0;
			default: return true;
		}
	}
	bool IsSigned() const {
		switch(GetConstType()) {
			case SINT64: return true;
			case SINT32: return true;
			default: return false;
		}
	}
	bool Is32Bit() const {
		switch(GetConstType()) {
		case SINT32: return true;
		case UINT32: return true;
		default: return false;
		}
	}

	bool IsBinary() const {	// check if value is other than zero or one
		switch (GetConstType()) {
			case SINT64:	return m_sint64 < 0 || m_sint64 > 1; break;
			case SINT32:	return m_sint32 < 0 || m_sint32 > 1; break;
			case UINT64:	return m_uint64 > 1; break;
			case UINT32:	return m_uint32 > 1; break;
			default: Assert(0);
		}
		return false;
	}
		
	int64_t GetSint64() const {
		switch (GetConstType()) {
			case SINT64:	return m_sint64; break;
			case SINT32:	return m_sint32; break;
			case UINT64:	return m_uint64; break;
			case UINT32:	return m_uint32; break;
			case UNKNOWN:	return 0; break;
			default: Assert(0);
		}
		return 0;
	}
	uint64_t GetUint64() const {
		switch (GetConstType()) {
			case SINT64:	return m_sint64; break;
			case SINT32:	return m_sint32; break;
			case UINT64:	return m_uint64; break;
			case UINT32:	return m_uint32; break;
			case UNKNOWN:	return 0; break;
			default: Assert(0);
		}
		return 0;
	}
	int32_t GetSint32() const {
		switch (GetConstType()) {
			case SINT64:	return (int32_t)m_sint64; break;
			case SINT32:	return (int32_t)m_sint32; break;
			case UINT64:	return (int32_t)m_uint64; break;
			case UINT32:	return (int32_t)m_uint32; break;
			case UNKNOWN:	return 0; break;
			default: Assert(0);
		}
		return 0;
	}
	uint32_t GetUint32() const {
		switch (GetConstType()) {
			case SINT64:	return (uint32_t)m_sint64; break;
			case SINT32:	return (uint32_t)m_sint32; break;
			case UINT64:	return (uint32_t)m_uint64; break;
			case UINT32:	return (uint32_t)m_uint32; break;
			case UNKNOWN:	return 0; break;
			default: Assert(0);
		}
		return 0;
	}

	uint64_t & SetUint64() { m_constType = UINT64; return m_uint64; }
	int64_t & SetSint64() { m_constType = SINT64; return m_sint64; }
	uint32_t & SetUint32() { m_constType = UINT32; return m_uint32; }
	int32_t & SetSint32() { m_constType = SINT32; return m_sint32; }

	uint32_t GetConstType() const { return m_constType; }

	void ConvertToSigned(CLineInfo lineInfo) {
		switch (GetConstType()) {
			case SINT64: break;
			case SINT32: break;
			case UINT64:
				if (GetUint64() & 0x8000000000000000LL)
					ErrorMsg(PARSE_ERROR, lineInfo, "Constant too large to convert to signed value");
				SetSint64() = GetUint64();
				break;
			case UINT32:
				if (GetUint32() & 0x80000000)
					ErrorMsg(PARSE_ERROR, lineInfo, "Constant too large to convert to signed value");
				SetSint32() = GetUint32();
				break;
			default: Assert(0);
		}
	}
	void ConvertToUnsigned(CLineInfo lineInfo) {
		switch (GetConstType()) {
			case SINT64:
				if (GetSint64() & 0x8000000000000000LL)
					ErrorMsg(PARSE_ERROR, lineInfo, "Can not convert negative constant to unsigned value");
				SetUint64() = GetSint64();
				break;
			case SINT32:
				if (GetSint32() & 0x80000000)
					ErrorMsg(PARSE_ERROR, lineInfo, "Can not convert negative constant to unsigned value");
				SetUint32() = GetSint32();
				break;
			case UINT64: break;
			case UINT32: break;
			default: Assert(0);
		}
	}
	CConstValue CastAsUint(int width) const {
		CConstValue rslt(0);
		uint64_t mask = width == 64 ? ~0ull : ((1ull << width)-1);
		switch (GetConstType()) {
			case SINT64:	rslt.SetUint64() = GetSint64() & mask; break;
			case SINT32:	rslt.SetUint32() = GetSint32() & mask; break;
			case UINT64:	rslt.SetUint64() = GetUint64() & mask; break;
			case UINT32:	rslt.SetUint32() = GetUint32() & mask; break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue Cast(bool isSigned, int width) const {
		CConstValue rslt(0);
		if (isSigned) {
			long long value = 0;
			switch (GetConstType()) {
				case SINT64:	value = GetSint64(); break;
				case SINT32:	value = GetSint32(); break;
				case UINT64:	value = GetUint64(); break;
				case UINT32:	value = GetUint32(); break;
				default: Assert(0);
			}
			value = (value << (64 - width)) >> (64 - width);
			if (width > 32)
				rslt.SetSint64() = value;
			else
				rslt.SetSint32() = (int32_t)value;
		} else {
			unsigned long long value = 0;
			switch (GetConstType()) {
			case SINT64:	value = GetSint64(); break;
			case SINT32:	value = GetSint32(); break;
			case UINT64:	value = GetUint64(); break;
			case UINT32:	value = GetUint32(); break;
			default: Assert(0);
			}
			uint64_t mask = width == 64 ? ~0ull : ((1ull << width) - 1);
			value &= mask;
			if (width > 32)
				rslt.SetUint64() = value;
			else
				rslt.SetUint32() = (uint32_t)value;
		}
		return rslt;
	}

#ifdef _WIN32
#pragma warning(disable : 4146)
#pragma warning(disable : 4018)
#else
#endif

	static bool IsEquivalent(const CConstValue & v1, const CConstValue & v2) {
		switch ((v1.GetConstType() << 2) | v2.GetConstType()) {
			case (UINT32 << 2) | UINT32: return v1.GetUint32() == v2.GetUint32(); break;
			case (UINT32 << 2) | UINT64: return v1.GetUint32() == v2.GetUint64(); break;
			case (UINT32 << 2) | SINT32: return v1.GetUint32() == (uint32_t)v2.GetSint32(); break;
			case (UINT32 << 2) | SINT64: return v1.GetUint32() == v2.GetSint64(); break;
			case (UINT64 << 2) | UINT32: return v1.GetUint32() == v2.GetUint32(); break;
			case (UINT64 << 2) | UINT64: return v1.GetUint32() == v2.GetUint64(); break;
			case (UINT64 << 2) | SINT32: return v1.GetUint32() == (uint32_t)v2.GetSint32(); break;
			case (UINT64 << 2) | SINT64: return v1.GetUint32() == v2.GetSint64(); break;
			case (SINT32 << 2) | UINT32: return v1.GetUint32() == v2.GetUint32(); break;
			case (SINT32 << 2) | UINT64: return v1.GetUint32() == v2.GetUint64(); break;
			case (SINT32 << 2) | SINT32: return v1.GetUint32() == (uint32_t)v2.GetSint32(); break;
			case (SINT32 << 2) | SINT64: return v1.GetUint32() == v2.GetSint64(); break;
			case (SINT64 << 2) | UINT32: return v1.GetUint32() == v2.GetUint32(); break;
			case (SINT64 << 2) | UINT64: return v1.GetUint32() == v2.GetUint64(); break;
			case (SINT64 << 2) | SINT32: return v1.GetUint32() == (uint32_t)v2.GetSint32(); break;
			case (SINT64 << 2) | SINT64: return v1.GetUint32() == v2.GetSint64(); break;
			default: Assert(0);
		}
		return false;
	}

	CConstValue operator - (void) const {
		CConstValue rslt(0);
		switch (GetConstType()) {
			case SINT64:	rslt.SetSint64() = -GetSint64(); break;
			case SINT32:	rslt.SetSint32() = -GetSint32(); break;
			case UINT64:	rslt.SetUint64() = -GetUint64(); break;
			case UINT32:	rslt.SetUint32() = -GetUint32(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator ~ (void) const {
		CConstValue rslt(0);
		switch (GetConstType()) {
			case SINT64:	rslt.SetSint64() = ~GetSint64(); break;
			case SINT32:	rslt.SetSint32() = ~GetSint32(); break;
			case UINT64:	rslt.SetUint64() = ~GetUint64(); break;
			case UINT32:	rslt.SetUint32() = ~GetUint32(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator ! (void) const {
		CConstValue rslt(0);
		switch (GetConstType()) {
			case SINT64:	rslt.SetSint64() = !GetSint64(); break;
			case SINT32:	rslt.SetSint32() = !GetSint32(); break;
			case UINT64:	rslt.SetUint64() = !GetUint64(); break;
			case UINT32:	rslt.SetUint32() = !GetUint32(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator + (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() + rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint64() = GetUint32() + rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() + rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint64() = GetUint32() + rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint64() = GetUint64() + rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint64() = GetUint64() + rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint64() = GetUint64() + rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint64() = GetUint64() + rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetUint32() = GetSint32() + rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetUint64() = GetSint32() + rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetSint32() = GetSint32() + rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetSint64() = GetSint32() + rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetUint64() = GetSint64() + rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetUint64() = GetSint64() + rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetSint64() = GetSint64() + rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetSint64() = GetSint64() + rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator - (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() - rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint64() = GetUint32() - rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() - rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint64() = GetUint32() - rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint64() = GetUint64() - rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint64() = GetUint64() - rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint64() = GetUint64() - rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint64() = GetUint64() - rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetUint32() = GetSint32() - rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetUint64() = GetSint32() - rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetSint32() = GetSint32() - rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetSint64() = GetSint32() - rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetUint64() = GetSint64() - rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetUint64() = GetSint64() - rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetSint64() = GetSint64() - rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetSint64() = GetSint64() - rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator / (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() / rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint64() = GetUint32() / rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() / rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint64() = GetUint32() / rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint64() = GetUint64() / rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint64() = GetUint64() / rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint64() = GetUint64() / rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint64() = GetUint64() / rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetUint32() = GetSint32() / rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetUint64() = GetSint32() / rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetSint32() = GetSint32() / rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetSint64() = GetSint32() / rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetUint64() = GetSint64() / rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetUint64() = GetSint64() / rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetSint64() = GetSint64() / rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetSint64() = GetSint64() / rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator % (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() % rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint64() = GetUint32() % rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() % rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint64() = GetUint32() % rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint64() = GetUint64() % rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint64() = GetUint64() % rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint64() = GetUint64() % rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint64() = GetUint64() % rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetUint32() = GetSint32() % rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetUint64() = GetSint32() % rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetSint32() = GetSint32() % rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetSint64() = GetSint32() % rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetUint64() = GetSint64() % rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetUint64() = GetSint64() % rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetSint64() = GetSint64() % rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetSint64() = GetSint64() % rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator * (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() * rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint64() = GetUint32() * rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() * rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint64() = GetUint32() * rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint64() = GetUint64() * rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint64() = GetUint64() * rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint64() = GetUint64() * rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint64() = GetUint64() * rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetUint32() = GetSint32() * rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetUint64() = GetSint32() * rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetSint32() = GetSint32() * rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetSint64() = GetSint32() * rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetUint64() = GetSint64() * rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetUint64() = GetSint64() * rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetSint64() = GetSint64() * rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetSint64() = GetSint64() * rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue XorReduce() const {
		uint64 r = 0;
		switch (GetConstType()) {
			case SINT64:	r = GetSint64(); break;
			case SINT32:	r = GetSint32(); break;
			case UINT64:	r = GetUint64(); break;
			case UINT32:	r = GetUint32(); break;
			default: Assert(0);
		}
		r = r ^ (r >> 32);
		r = r ^ (r >> 16);
		r = r ^ (r >> 8);
		r = r ^ (r >> 4);
		r = r ^ (r >> 2);
		r = r ^ (r >> 1);

		CConstValue rslt;
		rslt.SetUint64() = r;
		return rslt;
	}

	CConstValue operator | (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() | rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint64() = GetUint32() | rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() | rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint64() = GetUint32() | rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint64() = GetUint64() | rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint64() = GetUint64() | rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint64() = GetUint64() | rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint64() = GetUint64() | rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetUint32() = GetSint32() | rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetUint64() = GetSint32() | rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetSint32() = GetSint32() | rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetSint64() = GetSint32() | rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetUint64() = GetSint64() | rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetUint64() = GetSint64() | rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetSint64() = GetSint64() | rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetSint64() = GetSint64() | rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator & (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() & rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint64() = GetUint32() & rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() & rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint64() = GetUint32() & rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint64() = GetUint64() & rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint64() = GetUint64() & rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint64() = GetUint64() & rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint64() = GetUint64() & rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetUint32() = GetSint32() & rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetUint64() = GetSint32() & rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetSint32() = GetSint32() & rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetSint64() = GetSint32() & rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetUint64() = GetSint64() & rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetUint64() = GetSint64() & rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetSint64() = GetSint64() & rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetSint64() = GetSint64() & rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator ^ (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() ^ rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint64() = GetUint32() ^ rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() ^ rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint64() = GetUint32() ^ rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint64() = GetUint64() ^ rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint64() = GetUint64() ^ rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint64() = GetUint64() ^ rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint64() = GetUint64() ^ rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetUint32() = GetSint32() ^ rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetUint64() = GetSint32() ^ rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetSint32() = GetSint32() ^ rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetSint64() = GetSint32() ^ rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetUint64() = GetSint64() ^ rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetUint64() = GetSint64() ^ rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetSint64() = GetSint64() ^ rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetSint64() = GetSint64() ^ rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator << (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() << rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint32() = GetUint32() << rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() << rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint32() = GetUint32() << rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint64() = GetUint64() << rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint64() = GetUint64() << rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint64() = GetUint64() << rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint64() = GetUint64() << rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetSint32() = GetSint32() << rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetSint32() = GetSint32() << rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetSint32() = GetSint32() << rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetSint32() = GetSint32() << rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetSint64() = GetSint64() << rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetSint64() = GetSint64() << rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetSint64() = GetSint64() << rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetSint64() = GetSint64() << rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator >> (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() >> rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint32() = GetUint32() >> rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() >> rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint32() = GetUint32() >> rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint64() = GetUint64() >> rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint64() = GetUint64() >> rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint64() = GetUint64() >> rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint64() = GetUint64() >> rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetSint32() = GetSint32() >> rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetSint32() = GetSint32() >> rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetSint32() = GetSint32() >> rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetSint32() = GetSint32() >> rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetSint64() = GetSint64() >> rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetSint64() = GetSint64() >> rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetSint64() = GetSint64() >> rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetSint64() = GetSint64() >> rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator == (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() == rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint32() = GetUint32() == rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() == (uint32_t)rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint32() = GetUint32() == rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint32() = GetUint32() == rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint32() = GetUint32() == rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint32() = GetUint32() == (uint32_t)rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint32() = GetUint32() == rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() == rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetUint32() = GetUint32() == rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() == (uint32_t)rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetUint32() = GetUint32() == rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetUint32() = GetUint32() == rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetUint32() = GetUint32() == rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetUint32() = GetUint32() == (uint32_t)rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetUint32() = GetUint32() == rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator != (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() != rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint32() = GetUint32() != rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() != (uint32_t)rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint32() = GetUint32() != rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint32() = GetUint64() != rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint32() = GetUint64() != rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint32() = GetUint64() != (uint64_t)rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint32() = GetUint64() != (uint64_t)rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetUint32() = (uint32_t)GetSint32() != rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetUint32() = (uint64_t)GetSint32() != rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetUint32() = GetSint32() != rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetUint32() = GetSint32() != rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetUint32() = GetSint64() != rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetUint32() = (uint64_t)GetSint64() != rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetUint32() = GetSint64() != rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetUint32() = GetSint64() != rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	bool operator != (int64_t value) const {
		bool rslt = false;
		switch (GetConstType()) {
			case UINT32: rslt = GetUint32() != value; break;
			case UINT64: rslt = GetUint64() != (uint64_t)value; break;
			case SINT32: rslt = GetSint32() != value; break;
			case SINT64: rslt = GetSint64() != value; break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator || (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() || rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint32() = GetUint32() || rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() || rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint32() = GetUint32() || rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint32() = GetUint64() || rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint32() = GetUint64() || rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint32() = GetUint64() || rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint32() = GetUint64() || rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetUint32() = GetSint32() || rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetUint32() = GetSint32() || rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetUint32() = GetSint32() || rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetUint32() = GetSint32() || rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetUint32() = GetSint64() || rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetUint32() = GetSint64() || rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetUint32() = GetSint64() || rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetUint32() = GetSint64() || rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator && (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() && rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint32() = GetUint32() && rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() && rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint32() = GetUint32() && rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint32() = GetUint64() && rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint32() = GetUint64() && rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint32() = GetUint64() && rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint32() = GetUint64() && rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetUint32() = GetSint32() && rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetUint32() = GetSint32() && rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetUint32() = GetSint32() && rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetUint32() = GetSint32() && rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetUint32() = GetSint64() && rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetUint32() = GetSint64() && rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetUint32() = GetSint64() && rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetUint32() = GetSint64() && rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator < (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() < rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint32() = GetUint32() < rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() < (uint32_t)rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint32() = GetUint32() < rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint32() = GetUint64() < rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint32() = GetUint64() < rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint32() = GetUint64() < (uint64_t)rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint32() = GetUint64() < (uint64_t)rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetUint32() = (uint32_t)GetSint32() < rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetUint32() = (uint64_t)GetSint32() < rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetUint32() = GetSint32() < rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetUint32() = GetSint32() < rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetUint32() = GetSint64() < rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetUint32() = (uint64_t)GetSint64() < rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetUint32() = GetSint64() < rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetUint32() = GetSint64() < rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator > (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() > rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint32() = GetUint32() > rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() > (uint32_t)rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint32() = GetUint32() > rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint32() = GetUint64() > rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint32() = GetUint64() > rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint32() = GetUint64() > (uint64_t)rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint32() = GetUint64() > (uint64_t)rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetUint32() = (uint32_t)GetSint32() > rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetUint32() = (uint64_t)GetSint32() > rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetUint32() = GetSint32() > rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetUint32() = GetSint32() > rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetUint32() = GetSint64() > rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetUint32() = (uint64_t)GetSint64() > rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetUint32() = GetSint64() > rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetUint32() = GetSint64() > rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator <= (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() <= rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint32() = GetUint32() <= rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() <= (uint32_t)rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint32() = GetUint32() <= rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint32() = GetUint64() <= rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint32() = GetUint64() <= rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint32() = GetUint64() <= (uint64_t)rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint32() = GetUint64() <= (uint64_t)rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetUint32() = (uint32_t)GetSint32() <= rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetUint32() = (uint64_t)GetSint32() <= rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetUint32() = GetSint32() <= rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetUint32() = GetSint32() <= rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetUint32() = GetSint64() <= rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetUint32() = (uint64_t)GetSint64() <= rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetUint32() = GetSint64() <= rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetUint32() = GetSint64() <= rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}

	CConstValue operator >= (const CConstValue & rhs) const {
		CConstValue rslt(0);
		switch ((GetConstType() << 2) | rhs.GetConstType()) {
			case (UINT32 << 2) | UINT32: rslt.SetUint32() = GetUint32() >= rhs.GetUint32(); break;
			case (UINT32 << 2) | UINT64: rslt.SetUint32() = GetUint32() >= rhs.GetUint64(); break;
			case (UINT32 << 2) | SINT32: rslt.SetUint32() = GetUint32() >= (uint32_t)rhs.GetSint32(); break;
			case (UINT32 << 2) | SINT64: rslt.SetUint32() = GetUint32() >= rhs.GetSint64(); break;
			case (UINT64 << 2) | UINT32: rslt.SetUint32() = GetUint64() >= rhs.GetUint32(); break;
			case (UINT64 << 2) | UINT64: rslt.SetUint32() = GetUint64() >= rhs.GetUint64(); break;
			case (UINT64 << 2) | SINT32: rslt.SetUint32() = GetUint64() >= (uint64_t)rhs.GetSint32(); break;
			case (UINT64 << 2) | SINT64: rslt.SetUint32() = GetUint64() >= (uint64_t)rhs.GetSint64(); break;
			case (SINT32 << 2) | UINT32: rslt.SetUint32() = (uint32_t)GetSint32() >= rhs.GetUint32(); break;
			case (SINT32 << 2) | UINT64: rslt.SetUint32() = (uint64_t)GetSint32() >= rhs.GetUint64(); break;
			case (SINT32 << 2) | SINT32: rslt.SetUint32() = GetSint32() >= rhs.GetSint32(); break;
			case (SINT32 << 2) | SINT64: rslt.SetUint32() = GetSint32() >= rhs.GetSint64(); break;
			case (SINT64 << 2) | UINT32: rslt.SetUint32() = GetSint64() >= rhs.GetUint32(); break;
			case (SINT64 << 2) | UINT64: rslt.SetUint32() = (uint64_t)GetSint64() >= rhs.GetUint64(); break;
			case (SINT64 << 2) | SINT32: rslt.SetUint32() = GetSint64() >= rhs.GetSint32(); break;
			case (SINT64 << 2) | SINT64: rslt.SetUint32() = GetSint64() >= rhs.GetSint64(); break;
			default: Assert(0);
		}
		return rslt;
	}
#ifdef _WIN32
#pragma warning(default : 4146)
#pragma warning(default : 4018)
#else
#endif

	int GetMinWidth() const {
        // find minimum width for constant
		int minWidth;
		switch (GetConstType()) {
			case UINT32: minWidth = GetLeadingZeroCnt( GetUint32() ); break;
			case UINT64: minWidth = GetLeadingZeroCnt( GetUint64() ); break;
			case SINT32: minWidth = (IsNeg() ? GetLeadingOnesCnt( GetSint32() ) : GetLeadingZeroCnt( GetSint32() )) + 1; break;
			case SINT64: minWidth = (IsNeg() ? GetLeadingOnesCnt( GetSint64() ) : GetLeadingZeroCnt( GetSint64() )) + 1; break;
			default: minWidth = 0; Assert(0);
		}
		return minWidth;
	}

	static int GetLeadingZeroCnt(uint64_t v) {
        int hi = 64;
        int lo = 1;
        int w = 16;
        while(hi != lo) {
            if ((v & (ULLONG_MAX << w)) == 0) {
                hi = w;
                w = (hi + lo) / 2;
            } else {
                lo = w+1;
                w = (hi + lo) / 2;
            }
        }
		return w;
	}

	static int GetLeadingOnesCnt(uint64_t v) {
		return GetLeadingZeroCnt( ~v );
	}

	int GetSigWidth() const {
		return /*IsNeg() ? 64 : */GetMinWidth();
	}

private:
	EConstType	m_constType;
	union {
		uint64_t	m_uint64;
		int64_t		m_sint64;
		uint32_t	m_uint32;
		int32_t		m_sint32;
	};
};
#ifdef _WIN32
#pragma warning(default : 4146)
#pragma warning(default : 4018)
#else
#if GCC_VERSION >= 40600
#pragma GCC diagnostic pop
#endif
#if GCC_VERSION >= 40200
#pragma GCC diagnostic warning "-Wsign-compare"
#endif
#endif

