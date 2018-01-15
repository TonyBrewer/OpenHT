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

void CDsnInfo::GenPersFiles()
{
	CreateDirectoryStructure();
	CleanDirectoryStructure();

	// Generate common include file
	if (!g_appArgs.IsModelOnly()) {
		GenerateCommonIncludeFile();

		GenerateHtiFiles();
		GenerateHtaFiles();

		for (size_t mifIdx = 0; mifIdx < m_mifInstList.size(); mifIdx += 1)
			GenerateMifFiles(m_mifInstList[mifIdx].m_mifId);

		for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
			CModule &mod = *m_modList[modIdx];

			if (!mod.m_bIsUsed || mod.m_bHostIntf) continue;

			GenerateModuleFiles(mod);
		}

		GenerateNgvFiles();

		GenHtMon();

		GenerateUioStubFiles();

		GenerateClockStubFiles();

		GenerateAeTopFile();
		GenerateUnitTopFile();
	}

	// needs to be after the module files are generated so the list of module files is available
	GenerateHifFiles();
}

void CDsnInfo::CreateDirectoryStructure()
{
	// Create directory structure for htl files
	string dirPath = g_appArgs.GetOutputFolder();

	string rightPath = dirPath;
	string leftPath;
	bool bDone = false;
	do {
		int pos = rightPath.find_first_of("/\\");
		if (pos <= 0) {
			bDone = true;
			pos = rightPath.size() - 1;
		}
		leftPath += rightPath.substr(0, pos + 1);
		rightPath = rightPath.substr(pos + 1);

		// create directory
		int err = _mkdir(leftPath.c_str());
		if (err != 0 && errno != EEXIST) {
			fprintf(stderr, "Could not create directory %s\n", leftPath.c_str());
			exit(1);
		}
	} while (!bDone);
}

void CDsnInfo::CleanDirectoryStructure()
{
	// Get output directory
	string dirPath = g_appArgs.GetOutputFolder();

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (dirPath.c_str())) != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			string filename = ent->d_name;
			if (filename.find(".c") != std::string::npos ||
			    (filename.find(".h") != std::string::npos &&
			     filename.find(".html") == std::string::npos)
			    ) {
				remove(filename.c_str());
			}
		}
		closedir (dir);
	} else {
		fprintf(stderr, "Could not open directory %s\n", dirPath.c_str());
		exit(1);
	}
}
