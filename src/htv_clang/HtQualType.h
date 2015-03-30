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

	class HtType;

	class HtQualType {
	public:
		HtQualType() { m_pType = 0; m_isConst = false; }
		HtQualType( HtType * pType ) {
			m_pType = pType;
			m_isConst = false;
		}

		void setType( HtType * pType ) { m_pType = pType; }
		HtType * getType() { return m_pType; }

		void isConst(bool _isConst) { m_isConst = _isConst; }
		bool isConst() { return m_isConst; }

	private:
		HtType * m_pType;
		bool m_isConst;
	};
}
