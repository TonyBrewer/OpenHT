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

#include "Htv.h"
#include "HtvArgs.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;

namespace htv {
	// This declaraion is made instead of including HtvDesign.h so that PCH is not rebuilt
	//  when any HTV/HT header is changed.
	class HtvDesign;

	class HtvAction : public ASTConsumer {

	public:
		HtvAction(CompilerInstance * pCI, string & filename);

		void HandleTranslationUnit(ASTContext &Context);

	private:
		HtvDesign * m_pHtvDesign;
		CompilerInstance * m_pCI;
		string m_fileName;
	};

	struct ActionFactory : public SourceFileCallbacks {
		virtual bool handleBeginSource(CompilerInstance &CI, StringRef Filename) {
			m_pCI = & CI;
			m_fileName = Filename.str();
			return true;
		}
		virtual void handleEndSource() {}

		ASTConsumer *newASTConsumer() {
			if (g_htvArgs.isDumpClangAST())
				return CreateASTDumper("");
			else
				return new htv::HtvAction(m_pCI, m_fileName);
		}

	private:
		CompilerInstance * m_pCI;
		string m_fileName;
	};
}
