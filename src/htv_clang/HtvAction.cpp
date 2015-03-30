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
#include "HtvAction.h"
#include "HtvDesign.h"

namespace htv {
	//class HtPragmaHandler : public PragmaHandler {
	//public:
	//	HtPragmaHandler() {}

	//	virtual void HandlePragma(Preprocessor &PP, PragmaIntroducerKind Introducer,
	//		Token &FirstToken) 
	//	{
	//		bool stop = true;
	//	}
	//};

	HtvAction::HtvAction(CompilerInstance * pCI, string & filename)
	{
		m_pCI = pCI;
		m_fileName = filename;
		m_pHtvDesign = new HtvDesign();

		//Preprocessor &PP = pCI->getPreprocessor();
		//PP.AddPragmaHandler(new HtPragmaHandler());
	}

	void HtvAction::HandleTranslationUnit(clang::ASTContext &Context)
	{
		m_pHtvDesign->setContext( Context );

		// Exit if clang parse errors occured
		ExitIfError(Context);

		m_pHtvDesign->mirrorDesign( Context );

		m_pHtvDesign->markScModuleMethodCtor( Context );

		m_pHtvDesign->foldConstantExpr( Context );

		m_pHtvDesign->inlineFunctionCalls( Context );

		m_pHtvDesign->adjustCompoundStmts( Context );

		m_pHtvDesign->deadCodeElimination( Context );

		m_pHtvDesign->insertTempVarPrePostIncDec( Context );

		m_pHtvDesign->insertTempVarForArrayIndex( Context );

		m_pHtvDesign->inlineVarReferences( Context );

		m_pHtvDesign->transformConditionalBase( Context );

		m_pHtvDesign->arrangeCaseStmts( Context );

		m_pHtvDesign->xformSwitchStmts( Context );

		m_pHtvDesign->removeUnneededTemps( Context );

		m_pHtvDesign->unnestIfAndSwitch( Context );

		m_pHtvDesign->propUserRegisterStages( Context );

		m_pHtvDesign->propTimingRegisterStages( Context );

		m_pHtvDesign->insertRegisterStages( Context );

		if (g_htvArgs.isDumpHtvAST())
			m_pHtvDesign->dumpHtvAST( Context );
		else
			m_pHtvDesign->printDesign( Context );
	}
}
