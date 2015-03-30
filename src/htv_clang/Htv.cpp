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
#include "HtvArgs.h"
#include "HtvAction.h"

using namespace htv;
using namespace clang::tooling;

int main(int argc, const char **argv) {
	g_htvArgs.Parse(argc, argv);

	FixedCompilationDatabase Compilations(".", g_htvArgs.GetFrontEndArgs());

	ClangTool Tool(Compilations, g_htvArgs.GetSourceFiles());

	ActionFactory Factory;
	return Tool.run(newFrontendActionFactory(&Factory, &Factory));
}
