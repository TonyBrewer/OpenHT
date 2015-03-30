/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Htv.h"
#include "HtDesign.h"

namespace ht {

	HtStmt * HtDupStmtTree::dupStmtTree( HtStmtIter & iSrcStmt, void * pVoid )
	{
		if (iSrcStmt.asStmt()->getNodeId() == 0x17a)
			bool stop = true;

		HtStmt * pDstStmt = dupCallback( iSrcStmt, pVoid );

		if (pDstStmt)
			return pDstStmt;

		switch (iSrcStmt->getStmtClass()) {
		case Stmt::IfStmtClass:
			{
				pDstStmt = new HtStmt( *iSrcStmt );
				pDstStmt->setCond( dupStmtTree( HtStmtIter(iSrcStmt->getCond()), pVoid ));
				pDstStmt->setThen( dupStmtTree( HtStmtIter(iSrcStmt->getThen()), pVoid ));
				pDstStmt->setElse( dupStmtTree( HtStmtIter(iSrcStmt->getElse()), pVoid ));
			}
			break;
		case Stmt::CompoundAssignOperatorClass:
		case Stmt::BinaryOperatorClass:
			{
				pDstStmt = new HtExpr( iSrcStmt.asExpr() );
				pDstStmt->setLhs( dupStmtTree( HtStmtIter(iSrcStmt->getLhs()), pVoid ));
				pDstStmt->setRhs( dupStmtTree( HtStmtIter(iSrcStmt->getRhs()), pVoid ));
			}
			break;
		case Stmt::MemberExprClass:
			{
				pDstStmt = new HtExpr( iSrcStmt.asExpr() );
				pDstStmt->setBase( dupStmtTree( HtStmtIter(iSrcStmt->getBase()), pVoid ));
			}
			break;
		case Stmt::UnaryOperatorClass:
		case Stmt::ImplicitCastExprClass:
		case Stmt::CXXStaticCastExprClass:
		case Stmt::MaterializeTemporaryExprClass:
			{
				pDstStmt = new HtExpr( iSrcStmt.asExpr() );
				pDstStmt->setSubExpr( dupStmtTree( HtStmtIter(iSrcStmt->getSubExpr()), pVoid ));
			}
			break;
		case Stmt::ConditionalOperatorClass:
			{
				pDstStmt = new HtExpr( iSrcStmt.asExpr() );
				pDstStmt->setCond( dupStmtTree( HtStmtIter(iSrcStmt->getCond()), pVoid ));
				pDstStmt->setThen( dupStmtTree( HtStmtIter(iSrcStmt->getThen()), pVoid ));
				pDstStmt->setElse( dupStmtTree( HtStmtIter(iSrcStmt->getElse()), pVoid ));
			}
			break;
		case Stmt::CXXThisExprClass:
		case Stmt::IntegerLiteralClass:
		case Stmt::CXXBoolLiteralExprClass:
			pDstStmt = new HtExpr( iSrcStmt.asExpr() );
			break;
		case Stmt::DeclRefExprClass:
			{
				HtExpr * pDstExpr;
				pDstStmt = pDstExpr = new HtExpr( iSrcStmt.asExpr() );
				pDstExpr->setRefDecl( iSrcStmt.asExpr()->getRefDecl() );
			}
			break;
		case Stmt::ArraySubscriptExprClass:
			{
				pDstStmt = new HtExpr( iSrcStmt.asExpr() );
				pDstStmt->setBase( dupStmtTree( HtStmtIter(iSrcStmt->getBase()), pVoid ));
				pDstStmt->setIdx( dupStmtTree( HtStmtIter(iSrcStmt->getIdx()), pVoid ));
			}
			break;
		default:
			assert(0);
		}

		assert(pDstStmt);
		return pDstStmt;
	}
}
