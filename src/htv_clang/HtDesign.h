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

#include "HtvArgs.h"

#include "HtPragma.h"
#include "HtLineInfo.h"
#include "HtStmtList.h"
#include "HtQualType.h"
#include "HtTemplateArgument.h"
#include "HtBaseSpecifier.h"
#include "HtWalkStmtTree.h"
#include "HtDupStmtTree.h"
#include "HtNode.h"
#include "HtType.h"
#include "HtDecl.h"
#include "HtStmt.h"
#include "HtExpr.h"
#include "FpgaTimingModel.h"

using namespace clang;

namespace ht {

	class HtDesign {
	private:
		typedef map<string, int>				HtNameMap_map;
		typedef pair<string, int>				HtNameMap_valuePair;
		typedef HtNameMap_map::iterator			HtNameMap_iter;
		typedef HtNameMap_map::const_iterator	HtNameMap_constIter;
		typedef pair<HtNameMap_iter, bool>		HtNameMap_insertPair;

	public:
		HtDesign() {}

		void setTopDeclList(HtDeclList_t * pDeclList) { m_pDeclList = pDeclList; }
		HtDeclList_t & getTopDeclList() { return *m_pDeclList; }
		void addDecl( HtDecl * pHtDecl ) { m_pDeclList->push_back( pHtDecl ); }

		string findUniqueName(string name);
		string findUniqueName( string name, HtLineInfo & lineInfo );

		HtDecl * findBaseDecl( HtExpr * );

		static string getSourceLine(ASTContext &Context, SourceLocation Loc);

		void setContext(ASTContext &Context) { 
			m_pASTContext = &Context; 
			m_pDiagEngine = &Context.getDiagnostics();
		}

		FpgaTimingModel * getFpgaTimingModel() {
			return m_pFpgaTimingModel;
		}

		// transforms
		void mirrorDesign(ASTContext &Context);
		void markScModuleMethodCtor(ASTContext &Context);
		void foldConstantExpr(ASTContext &Context);
		void deadCodeElimination(ASTContext &Context);
		void inlineFunctionCalls(ASTContext &Context);
		void inlineVarReferences(ASTContext &Context);
		void transformConditionalBase(ASTContext &Context);
		void adjustCompoundStmts(ASTContext &Context);
		void insertTempVarForArrayIndex(ASTContext &Context);
		void insertTempVarPrePostIncDec(ASTContext &Context);
		void arrangeCaseStmts(ASTContext &Context);
		void xformSwitchStmts(ASTContext &Context);
		void calcMemberPosAndWidth(ASTContext &Context);
		void updateRefCounts(ASTContext &Context);
		void removeUnneededTemps(ASTContext &Context);
		void unnestIfAndSwitch(ASTContext &Context);
		void propUserRegisterStages(ASTContext &Context);
		void propTimingRegisterStages(ASTContext &Context);
		void insertRegisterStages(ASTContext &Context);
		void printDesign(ASTContext &Context);
		void dumpHtvAST(ASTContext &Context);

		// sc module / ctor / method
		void setScModuleDecl( HtDecl * pScModuleDecl ) { m_pScModuleDecl = pScModuleDecl; }
		void setScCtorDecl( HtDecl * pScCtorDecl ) { m_pScCtorDecl = pScCtorDecl; }
		void addScMethodDecl( HtDecl * pScMethodDecl ) { m_scMethodDeclList.push_back( pScMethodDecl ); }

		HtDecl * getScModuleDecl() { return m_pScModuleDecl; }
		HtDeclList_t & getScModuleDeclList() { return m_pScModuleDecl->getSubDeclList(); }
		HtDeclList_t & getScMethodDeclList() { return m_scMethodDeclList; }

		void setBuiltinIntType(HtType * pIntType) { m_pBuiltinIntType = pIntType; }
		HtType * getBuiltinIntType() { return m_pBuiltinIntType; }

		void initTrsInfo();
		void initUrsInfo();
		size_t addTpGrp(string & name);

		void addPragmaTpsTpe( HtDecl * pDecl, HtPragma * pPragma);
		void addPragmaTpp( HtDecl * pDecl, HtPragma * pPragma);
		int findTpGrpIdFromName(string & name);
		HtTpg * getTpGrpFromGrpId(int grpId);
		bool isParamOutput(HtQualType qualType);
		bool isParamConst(HtQualType qualType);

		static void genAssignStmts(HtStmtIter & iInsert, HtExpr * pHtLhs, HtExpr * pHtRhs);

	private:
		ASTContext * m_pASTContext;
		DiagnosticsEngine * m_pDiagEngine;

		HtDeclList_t * m_pDeclList;	// top level declarations

		HtTpgList_t m_tpgList;

		HtDecl * m_pScModuleDecl;
		HtDecl * m_pScCtorDecl;
		HtDeclList_t m_scMethodDeclList;

		HtType * m_pBuiltinIntType;

		FpgaTimingModel * m_pFpgaTimingModel;

		HtNameMap_map m_uniqueNameMap;
	};

	// utility routines
	inline string formatStr(const char *msgStr, ...) {
		va_list marker;
		va_start( marker, msgStr );

		char buf2[256];
		vsprintf(buf2, msgStr, marker);

		return string(buf2);
	}

	void ExitIfError(clang::ASTContext &Context);
}
