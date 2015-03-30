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
	class HtExpr;
	class HtTemplateArgument;

	class HtTemplateArgList {
	public:
		HtTemplateArgList() {
			m_pFirstArg = 0;
			m_ppLastArg = & m_pFirstArg;
		}

		void append(HtTemplateArgument * pHtArg);

		HtTemplateArgument * getFirst() { return m_pFirstArg; }

	private:
		HtTemplateArgument * m_pFirstArg;
		HtTemplateArgument ** m_ppLastArg;
	};

	class HtTemplateArgument {
	public:
		HtTemplateArgument(TemplateArgument const * pArg) {
			m_kind = pArg->getKind();
			m_pNextArg = 0;
		}

		TemplateArgument::ArgKind getKind() { return m_kind; }

		void setExpr( HtExpr * pExpr ) { assert(m_expr.empty()); m_expr.push_back((HtStmt*)pExpr); }
		HtStmtList & getExpr() { return m_expr; }

		void setQualType( HtQualType & qualType ) { assert(m_qualType.getType() == 0); m_qualType = qualType; }
		HtQualType & getQualType() { return m_qualType; }

		void setIntegral( llvm::APSInt & integral ) { m_integral = integral; }
		llvm::APSInt getIntegral() { return m_integral; }

		string getIntegerLiteralAsStr() {
			return m_integral.toString(10, false);
		}

		HtTemplateArgument * getNextTemplateArg() { return m_pNextArg; }
		HtTemplateArgument ** getPNextTemplateArg() { return & m_pNextArg; }

	private:
		TemplateArgument::ArgKind m_kind;

		HtStmtList m_expr;
		HtQualType m_qualType;
		llvm::APSInt m_integral;

		HtTemplateArgument * m_pNextArg;
	};

	inline void HtTemplateArgList::append(HtTemplateArgument * pHtArg) {
		assert(pHtArg->getNextTemplateArg() == 0);
		*m_ppLastArg = pHtArg;
		m_ppLastArg = pHtArg->getPNextTemplateArg();
	}
}
