/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "CnyHt.h"
#include "DsnInfo.h"

void
CDsnInfo::GenPersBanner(CHtFile &htFile, const char *unitName, const char *dsnName, bool is_h, const char *incName)
{
	if (!htFile)
		return;

	char fileBase[256];
	sprintf(fileBase, "Pers%s%s", unitName, dsnName);

	fprintf(htFile, "/*****************************************************************************/\n");
#ifdef _WIN32
	fprintf(htFile, "// Generated with htl\n");
#else
	fprintf(htFile, "// Generated with htl %s-%s (%s)\n", VERSION, VCSREV, BLDDTE);
#endif
	fprintf(htFile, "//\n");
	fprintf(htFile, "// Hybrid Thread File - %s.%s\n", fileBase, is_h ? "h" : "cpp");
	fprintf(htFile, "//\n");
	fprintf(htFile, "/*****************************************************************************/\n");
	if (is_h) {
	    fprintf(htFile, "#pragma once\n\n");
	    fprintf(htFile, "#include \"Pers%sCommon.h\"\n\n", !g_appArgs.IsModuleUnitNamesEnabled() ? unitName : "");
	} else {
	    fprintf(htFile, "#include \"Ht.h\"\n");
		fprintf(htFile, "\n");
		fprintf(htFile, "#define PERS_%s\n", CHtString(dsnName).Upper().c_str());

		if (incName)
			fprintf(htFile, "#include \"Pers%s.h\"\n\n", incName);
		else
			fprintf(htFile, "#include \"%s.h\"\n\n", fileBase);
	}
}

void
CDsnInfo::GenerateBanner(CHtFile &htFile, const char *fileName, bool is_h)
{
	if (!htFile)
		return;

	fprintf(htFile, "/*****************************************************************************/\n");
#ifdef _WIN32
	fprintf(htFile, "// Generated with htl\n");
#else
	fprintf(htFile, "// Generated with htl %s-%s (%s)\n", VERSION, VCSREV, BLDDTE);
#endif
	fprintf(htFile, "//\n");
	fprintf(htFile, "// Hybrid Thread File - %s\n", fileName);
	fprintf(htFile, "//\n");
	fprintf(htFile, "/*****************************************************************************/\n");
	if (is_h) {
	    fprintf(htFile, "#pragma once\n\n");
	}
}
