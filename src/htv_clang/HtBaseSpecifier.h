/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

namespace ht {

	class HtBaseSpecifier;

	class HtBaseSpecifierList {
	public:
		HtBaseSpecifierList() {
			m_pFirstBaseSpecifier = 0;
			m_ppLastBaseSpecifier = & m_pFirstBaseSpecifier;
		}

		void append(HtBaseSpecifier * pHtBaseSpecifier);

		HtBaseSpecifier * getFirst() { return m_pFirstBaseSpecifier; }

	private:
		HtBaseSpecifier * m_pFirstBaseSpecifier;
		HtBaseSpecifier ** m_ppLastBaseSpecifier;
	};

	class HtBaseSpecifier {
	public:
		HtBaseSpecifier(CXXBaseSpecifier const *) {
			m_pNextBaseSpec = 0;
		}

		HtBaseSpecifier( HtBaseSpecifier * pBaseSpec ) {
			m_isVirtual = pBaseSpec->m_isVirtual;
			m_access = pBaseSpec->m_access;
			m_qualType = pBaseSpec->m_qualType;
			m_pNextBaseSpec = 0;
		}

		void isVirtual(bool is) { m_isVirtual = is; }
		bool isVirtual() { return m_isVirtual; }

		void setAccessSpec(AccessSpecifier access) { m_access = access; }
		AccessSpecifier getAccessSpec() { return m_access; }
		string getAccessSpecName() {
			switch (m_access) {
			case AS_none: return "";
			case AS_public: return "public ";
			case AS_private: return "private ";
			case AS_protected: return "protected ";
			default: assert(0);
			}
			return "";
		}

		void setQualType(HtQualType & qualType) { m_qualType = qualType; }
		HtQualType & getQualType() { return m_qualType; }

		HtBaseSpecifier * getNextBaseSpec() { return m_pNextBaseSpec; }
		HtBaseSpecifier ** getPNextBaseSpec() { return & m_pNextBaseSpec; }

	private:

		bool m_isVirtual;
		AccessSpecifier m_access;
		HtQualType m_qualType;

		HtBaseSpecifier * m_pNextBaseSpec;
	};

	inline void HtBaseSpecifierList::append(HtBaseSpecifier * pHtBaseSpecifier) {
		assert(pHtBaseSpecifier->getNextBaseSpec() == 0);
		*m_ppLastBaseSpecifier = pHtBaseSpecifier;
		m_ppLastBaseSpecifier = pHtBaseSpecifier->getPNextBaseSpec();
	}
}
