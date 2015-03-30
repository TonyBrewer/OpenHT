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

using namespace std;
using namespace clang;

namespace ht {
	class HtIdent;

	class HtExprIter {
	public:
		HtExprIter(HtStmtList & list) {
			m_pList = &list.getList();
			m_iter = list.begin();
		}
		bool isEol() { return m_iter == m_pList->end(); }
		HtExprIter operator++(int) { HtExprIter tmp = *this; m_iter++; return tmp; }
		HtExpr * operator* () { return m_iter->asExpr(); }
	private:
		HtStmtList_t * m_pList;
		HtStmtList_iter_t m_iter;
	};

	class HtExpr : public HtStmt {
	public:

		HtExpr(Expr const * pExpr, HtQualType qualType) : HtStmt(pExpr) {
			Init();
			m_isExpr = true;
			m_qualType = qualType;
		}

		HtExpr(Stmt::StmtClass stmtClass, HtQualType qualType, HtLineInfo & lineInfo) : HtStmt(stmtClass, lineInfo) {
			Init();
			m_isExpr = true;
			m_qualType = qualType;
		}

		HtExpr(HtStmt * pRhsStmt) : HtStmt( pRhsStmt) {

			HtExpr * pRhs = (HtExpr *)pRhsStmt;

			Init();
			m_isExpr = true;
			m_qualType = pRhs->m_qualType;

			m_pMemberDecl = pRhs->m_pMemberDecl;
			m_pConstructorDecl = pRhs->m_pConstructorDecl;
			m_pCalleeDecl = pRhs->m_pCalleeDecl;
			m_pRefDecl = pRhs->m_pRefDecl;

			m_isMemberExprArrow = pRhs->m_isMemberExprArrow;
			m_isImplicitNode = pRhs->m_isImplicitNode;
			m_isElidable = pRhs->m_isElidable;
			m_isConstExpr = pRhs->m_isConstExpr;
			m_evalResult = pRhs->m_evalResult;

			m_stringLiteralValue = pRhs->m_stringLiteralValue;

			m_binaryOpcode = pRhs->m_binaryOpcode;
		}

		void Init() {
			m_pMemberDecl = 0;
			m_pConstructorDecl = 0;
			m_pCalleeDecl = 0;
			m_pRefDecl = 0;
			m_isMemberExprArrow = false;
			m_isImplicitNode = false;
			m_isElidable = false;
			m_isConstExpr = false;
		}

		void setUnaryOperator( UnaryOperator::Opcode unaryOpcode ) { m_unaryOpcode = unaryOpcode; }
		void setBinaryOperator( BinaryOperator::Opcode binaryOpcode ) { m_binaryOpcode = binaryOpcode; }
		void setCompoundAssignOperator( CompoundAssignOperator::Opcode compoundAssignOpcode ) { m_compoundAssignOpcode = compoundAssignOpcode; }

		UnaryOperator::Opcode getUnaryOperator() { return m_unaryOpcode; }
		BinaryOperator::Opcode getBinaryOperator() { return m_binaryOpcode; }

		//void setQualType( HtQualType & qualType ) { m_qualType = qualType; }
		HtQualType & getQualType() { return m_qualType; }

		HtExpr * getLhsExpr() { return getLhs().front().asExpr(); }
		HtExpr * getRhsExpr() { return getRhs().front().asExpr(); }

		void setMemberDecl( HtDecl * pDecl ) { m_pMemberDecl = pDecl; }
		HtDecl * getMemberDecl() { return m_pMemberDecl; }

		void setCalleeDecl( HtDecl * pDecl ) { assert(m_pCalleeDecl == 0); m_pCalleeDecl = pDecl; }
		HtDecl * getCalleeDecl() { return m_pCalleeDecl; }

		void setConstructorDecl( HtDecl * pDecl ) { assert(m_pConstructorDecl == 0); m_pConstructorDecl = pDecl; }
		HtDecl * getConstructorDecl() { return m_pConstructorDecl; }

		void setRefDecl( HtDecl * pDecl ) { m_pRefDecl = pDecl; }
		HtDecl * getRefDecl() { return m_pRefDecl; }

		void isArrow(bool is) { m_isMemberExprArrow = is; }
		bool isArrow() { return m_isMemberExprArrow; }

		void setIsImplicitNode(bool isImplicit ) { m_isImplicitNode = isImplicit; }
		bool isImplicitNode() { return m_isImplicitNode; }

		void isElidable(bool is) { m_isElidable = is; }
		bool isElidable() { return m_isElidable; }

		string getOpcodeAsStr() {
			switch (m_stmtClass) {
			case Stmt::UnaryOperatorClass:
				return UnaryOperator::getOpcodeStr(m_unaryOpcode).str();
			case Stmt::BinaryOperatorClass:
				return BinaryOperator::getOpcodeStr(m_binaryOpcode).str();
			case Stmt::CompoundAssignOperatorClass:
				return CompoundAssignOperator::getOpcodeStr(m_compoundAssignOpcode).str();
			default:
				assert(0);
			}
		}

		bool getBooleanLiteralAsValue() {
			assert(m_evalResult.Val.isInt());
			return m_evalResult.Val.getInt() != 0;
		}

		string getBooleanLiteralAsStr() {
			assert(m_evalResult.Val.isInt());
			return m_evalResult.Val.getInt() != 0 ? "true" : "false";
		}

		void setStringLiteral(llvm::StringRef &stringRef) {
			m_stringLiteralValue = stringRef;
		}
		string getStringLiteralAsStr() {
			return m_stringLiteralValue.str();
		}

		void setIntegerLiteral(uint64_t v) {
			m_evalResult.Val = APValue(llvm::APSInt(llvm::APInt(32u, v)));
		}
		void setIntegerLiteral(APValue & val) {
			m_evalResult.Val = val;
		}
		string getIntegerLiteralAsStr() {
			assert(m_evalResult.Val.isInt());
			return m_evalResult.Val.getInt().toString(10, m_evalResult.Val.getInt().isSigned());
		}

		string getFloatingLiteralAsStr() {
			assert(m_evalResult.Val.isFloat());
			SmallVector<char, 32> Buffer;
			m_evalResult.Val.getFloat().toString(Buffer);
			return StringRef(Buffer.data(), Buffer.size()).str();
		}

		void addArgExpr( HtExpr * pHtExpr ) { m_argExprList.push_back(pHtExpr); }
		HtStmtList & getArgExprList() { return m_argExprList; }

		void addInitExpr( HtExpr * pHtExpr ) { m_initExprList.push_back(pHtExpr); }
		HtStmtList & getInitExprList() { return m_initExprList; }

		bool isConstExpr() { return m_isConstExpr; }
		void isConstExpr( bool is ) { m_isConstExpr = is; }

		Expr::EvalResult & getEvalResult() { return m_evalResult; }

	private:
		HtQualType m_qualType;		// for casts

		HtDecl * m_pMemberDecl;
		HtDecl * m_pConstructorDecl;
		HtDecl * m_pCalleeDecl;
		HtDecl * m_pRefDecl;

		bool m_isMemberExprArrow;	// is "." or "->" for MemberExprClass
		bool m_isImplicitNode;		// not source code associated with node
		bool m_isElidable;
		bool m_isConstExpr;

		Expr::EvalResult m_evalResult;
		llvm::StringRef m_stringLiteralValue;

		union {
			UnaryOperator::Opcode m_unaryOpcode;
			BinaryOperator::Opcode m_binaryOpcode;
			CompoundAssignOperator::Opcode m_compoundAssignOpcode;
		};

		HtStmtList m_argExprList;
		HtStmtList m_initExprList;
	};
}
